"""
Emotiv OSC -> Arduino Serial bridge

Pipeline:
EmotivBCI (OSC) -> this Python script -> Arduino -> robotic arm

Run example:
python3 Adrian_robot_control_OSC.py --serial-port /dev/cu.usbmodem1101

What this script does:
1) Listen for OSC mental-command messages from Emotiv.
2) Convert recognized actions (example: "push") into short Arduino commands (example: "F").
3) Send command to Arduino over serial and wait for ACK confirmation.

How to explain this in class:
- OSC is the "incoming brain-command message" channel.
- Python is the translator + safety filter.
- Arduino is the motor controller that physically moves the arm.

Reference links used for this file:
- python-osc dispatcher/server patterns:
  https://python-osc.readthedocs.io/en/latest/dispatcher.html
- pyserial write/read basics:
  https://pyserial.readthedocs.io/en/latest/shortintro.html
- Emotiv Cortex stream/subscription reference (general command stream context):
  https://emotiv.gitbook.io/cortex-api/data-subscription/subscribe
"""

import argparse
import threading
import time

import serial
from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import ThreadingOSCUDPServer


# ===== 1) CONFIGURATION =====
# These constants are your "default settings" so users can run the script
# without typing every option manually.
# OSC server settings:
# - IP 127.0.0.1 means "listen on this same computer only".
# - Port must match the sender's OSC target port in EmotivBCI.
DEFAULT_OSC_IP = "127.0.0.1"
DEFAULT_OSC_PORT = 5005

# Command filtering:
# - threshold: minimum confidence required to accept a mental command.
# - cooldown: avoid sending the same command too frequently.
DEFAULT_CONF_THRESHOLD = 0.45
DEFAULT_COOLDOWN_SEC = 0.25

# Serial reliability:
# - ack_timeout: how long to wait for Arduino ACK for each send.
# - retries: extra attempts after first timeout.
DEFAULT_ACK_TIMEOUT_SEC = 0.30
DEFAULT_RETRIES = 2

# OSC action -> Arduino command.
ACTION_TO_CMD = {
    # Add more mappings here as your project grows, e.g. "left": "L"
    "push": "F",  # F = move forward
}


# ===== 2) SERIAL BRIDGE (Python -> Arduino) =====
class OscToArduinoBridge:
    """
    Owns the serial connection and command delivery logic.

    Reliability features:
    - Sequence numbers so ACKs can be matched to the correct command.
    - Retry on timeout.
    - Basic latency printouts for debugging.

    In plain words:
    - Python sends a command.
    - Arduino replies "ACK" if it got it.
    - If Python does not see ACK in time, it retries.
    """

    def __init__(self, serial_port, baud, conf_threshold, cooldown, ack_timeout, retries):
        self.conf_threshold = conf_threshold
        self.cooldown = cooldown
        self.ack_timeout = ack_timeout
        self.retries = retries
        # Tracks timing to prevent repeated same commands too fast.
        self.last_sent_at = 0.0
        # last_cmd + last_sent_at are used for cooldown filtering.
        self.last_cmd = None
        self.seq = 0
        # Lock protects sequence + serial writes in case callbacks overlap.
        self.lock = threading.Lock()
        self.arduino = serial.Serial(
            serial_port,
            baud,
            timeout=0.05,
            write_timeout=0.20,
        )
        # Many Arduinos reset when serial opens; wait so it is ready.
        time.sleep(2.0)
        # Clear any boot-time noise before processing real ACKs.
        self.arduino.reset_input_buffer()
        self.arduino.reset_output_buffer()
        print(f"[Serial] Connected to {serial_port} @ {baud}")

    @staticmethod
    def _parse_ack(line):
        # Arduino ACK format: ACK,<seq>,<cmd>,<arduino_us>
        # Example: ACK,7,F,2530
        # Returns parsed values or (None, None, None) if malformed.
        parts = line.strip().split(",")
        if len(parts) != 4 or parts[0] != "ACK":
            return None, None, None
        try:
            seq = int(parts[1])
            cmd = parts[2].strip()
            arduino_us = int(parts[3])
        except ValueError:
            return None, None, None
        return seq, cmd, arduino_us

    def _send_with_ack(self, cmd):
        """
        Send one command with retries and ACK checking.

        Returns:
            True if matching ACK is received, else False.
        """
        with self.lock:
            # Every command gets a new id number (sequence number).
            self.seq += 1
            seq = self.seq
            # Command format sent to Arduino: CMD,<sequence>,<command>\n
            payload = f"CMD,{seq},{cmd}\n".encode("ascii")

            for attempt in range(1, self.retries + 2):
                # Track timing so we can print transmission/round-trip latency.
                send_start_ns = time.perf_counter_ns()
                self.arduino.write(payload)
                self.arduino.flush()
                send_end_ns = time.perf_counter_ns()

                deadline = time.monotonic() + self.ack_timeout
                # Keep checking serial input until timeout is reached.
                while time.monotonic() < deadline:
                    raw = self.arduino.readline()
                    if not raw:
                        # No line yet, keep waiting until timeout.
                        continue

                    line = raw.decode("ascii", errors="replace").strip()
                    if line.startswith("ERR,"):
                        # Arduino explicitly reported an error.
                        # We keep waiting in case a later valid ACK arrives.
                        print(f"[SERIAL] Arduino ERR: {line}")
                        continue

                    ack_seq, ack_cmd, arduino_us = self._parse_ack(line)
                    if ack_seq is None:
                        print(f"[SERIAL] Unrecognized line: {line}")
                        continue
                    if ack_seq != seq:
                        # Ignore ACK for a different command sequence.
                        # This can happen if a delayed ACK from an older command arrives.
                        continue
                    if ack_cmd != cmd:
                        # Sequence matched but command letter did not.
                        # We do not accept this as success.
                        print(f"[SERIAL] ACK cmd mismatch seq={seq} sent={cmd} ack={ack_cmd}")
                        continue

                    ack_at_ns = time.perf_counter_ns()
                    # tx_write_ms: time spent writing bytes to serial.
                    tx_write_ms = (send_end_ns - send_start_ns) / 1_000_000.0
                    # rtt_ms: send start -> ACK receive.
                    rtt_ms = (ack_at_ns - send_start_ns) / 1_000_000.0
                    print(
                        f"[LATENCY] seq={seq} cmd={cmd} tx_write_ms={tx_write_ms:.3f} "
                        f"rtt_ms={rtt_ms:.3f} arduino_us={arduino_us}"
                    )
                    return True

                print(
                    f"[SERIAL] ACK timeout seq={seq} cmd={cmd} "
                    f"attempt={attempt}/{self.retries + 1}"
                )

            print(f"[SERIAL] FAILED seq={seq} cmd={cmd} after {self.retries + 1} attempts")
            return False

    # Receives parsed OSC action + confidence and decides if we send to Arduino.
    def maybe_send(self, action, confidence):
        """
        Filters incoming action/confidence before serial send:
        - must exist in ACTION_TO_CMD
        - must pass confidence threshold
        - respects cooldown for repeated same command

        This is the main "safety gate" before movement happens.
        """
        action = action.strip()
        # Translate OSC action word into your one-letter robot command.
        cmd = ACTION_TO_CMD.get(action)
        now = time.time()

        if cmd is None:
            # Action was not mapped in ACTION_TO_CMD.
            print(f"[OSC] Unknown action '{action}' (ignored)")
            return
        if confidence < self.conf_threshold:
            # Confidence too low, skip to reduce noisy detections.
            print(f"[OSC] {action} conf={confidence:.2f} below threshold {self.conf_threshold:.2f}")
            return
        if self.last_cmd == cmd and (now - self.last_sent_at) < self.cooldown:
            # Same command repeated too soon; drop it.
            return

        # Try reliable serial send (with ACK/retry).
        ok = self._send_with_ack(cmd)
        if ok:
            # Update cooldown tracking only after successful ACK.
            self.last_cmd = cmd
            self.last_sent_at = now
            print(f"[SERIAL] sent={cmd} action={action} conf={confidence:.2f}")

    def close(self):
        if self.arduino and self.arduino.is_open:
            self.arduino.close()


# ===== 3) OSC PARSING (Emotiv -> Python) =====
def parse_com_message(address, args):
    """
    Parse Emotiv-style OSC message into (action, confidence).

    Supported message styles:
    - /com/action + [<action>, <confidence>]
    - /com/<action> + [<confidence>]

    Returns (None, None) for malformed/unsupported payloads.
    """
    if address == "/com/action":
        # Style A: address carries generic tag, payload includes action string.
        if len(args) >= 2:
            action = str(args[0]).strip().lower()
            try:
                confidence = float(args[1])
            except (TypeError, ValueError):
                # Payload field was present but not numeric.
                return None, None
            return action, confidence
        return None, None

    if address.startswith("/com/"):
        # Style B: action is encoded directly in OSC address.
        action = address.split("/")[-1].strip().lower()
        try:
            # If confidence is missing, assume 1.0.
            confidence = float(args[0]) if args else 1.0
        except (TypeError, ValueError):
            return None, None
        return action, confidence

    return None, None


# ===== 4) MAIN PROGRAM =====
def main():
    # Command-line arguments so settings can be tuned without editing code.
    parser = argparse.ArgumentParser(description="Bridge Emotiv OSC mental commands to Arduino serial.")
    parser.add_argument("--osc-ip", default=DEFAULT_OSC_IP, help=f"OSC bind IP (default: {DEFAULT_OSC_IP})")
    parser.add_argument("--osc-port", type=int, default=DEFAULT_OSC_PORT, help=f"OSC bind port (default: {DEFAULT_OSC_PORT})")
    parser.add_argument("--serial-port", required=True, help="Arduino serial port, e.g. /dev/cu.usbmodem1101")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate (default: 115200)")
    parser.add_argument("--threshold", type=float, default=DEFAULT_CONF_THRESHOLD, help="Min confidence to send command")
    parser.add_argument("--cooldown", type=float, default=DEFAULT_COOLDOWN_SEC, help="Min seconds between repeated identical commands")
    parser.add_argument("--ack-timeout", type=float, default=DEFAULT_ACK_TIMEOUT_SEC, help="ACK timeout in seconds")
    parser.add_argument("--retries", type=int, default=DEFAULT_RETRIES, help="Retries after timeout (default: 2)")
    args = parser.parse_args()

    # Initialize serial bridge with chosen settings.
    # If serial port is wrong, this is where the script will fail fast.
    bridge = OscToArduinoBridge(
        serial_port=args.serial_port,
        baud=args.baud,
        conf_threshold=args.threshold,
        cooldown=args.cooldown,
        ack_timeout=args.ack_timeout,
        retries=args.retries,
    )

    # Called every time an OSC /com/* message arrives.
    # This function is the OSC callback.
    def on_mental(address, *msg_args):
        action, confidence = parse_com_message(address, msg_args)
        if action is None or confidence is None:
            print(f"[OSC] Unhandled {address} args={msg_args}")
            return
        bridge.maybe_send(action, confidence)

    def on_unmatched(address, *msg_args):
        # Helpful when debugging: shows OSC messages outside /com/*
        print(f"[OSC] Ignored address={address} args={msg_args}")

    # Route incoming OSC addresses matching /com/* to our callback.
    dispatcher = Dispatcher()
    # Handle explicit action endpoint and wildcard action endpoints.
    dispatcher.map("/com/action", on_mental)
    dispatcher.map("/com/*", on_mental)
    # python-osc default handler for all unmatched addresses.
    dispatcher.set_default_handler(on_unmatched)

    # Start OSC UDP server loop.
    server = ThreadingOSCUDPServer((args.osc_ip, args.osc_port), dispatcher)
    print(f"[OSC] Listening on {args.osc_ip}:{args.osc_port} for /com/*")
    print(f"[MAP] {ACTION_TO_CMD}")
    # Infinite server loop: waits for incoming OSC packets.
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[SYS] Stopping bridge...")
    finally:
        bridge.close()


if __name__ == "__main__":
    main()
