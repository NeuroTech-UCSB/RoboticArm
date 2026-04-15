from .bridge import run_bridge

import argparse
import asyncio
from dotenv import load_dotenv
import os


# ===== CONFIGURATION =====
load_dotenv()
DEFAULT_CORTEX_URL = "wss://localhost:6868"
DEFAULT_BAUD = 115200
DEFAULT_CONF_THRESHOLD = 0.45
DEFAULT_COOLDOWN_SEC = 0.25
CLIENT_ID = os.getenv("EMOTIV_CLIENT_ID")
CLIENT_SECRET = os.getenv("EMOTIV_CLIENT_SECRET")
LICENSE_KEY = os.getenv("EMOTIV_LICENSE_KEY")

# ===== CLI ARGUMENTS =====
def build_parser():
    parser = argparse.ArgumentParser(description="Emotiv Cortex API mental command bridge to Arduino serial.")
    # Required credentials/ports are passed through command line arguments.
    parser.add_argument("--cortex-url", default=DEFAULT_CORTEX_URL, help=f"Cortex WebSocket URL (default: {DEFAULT_CORTEX_URL})")
    parser.add_argument("--headset-id", default="", help="Optional specific headset ID")
    parser.add_argument("--profile", default="", help="Optional trained mental-command profile to load")
    parser.add_argument("--serial-port", required=True, help="Arduino serial port (e.g. /dev/cu.usbmodem1101 or COM5)")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help=f"Serial baud (default: {DEFAULT_BAUD})")
    parser.add_argument("--threshold", type=float, default=DEFAULT_CONF_THRESHOLD, help="Min confidence to send command")
    parser.add_argument("--cooldown", type=float, default=DEFAULT_COOLDOWN_SEC, help="Seconds between same command repeats")
    parser.add_argument("--debug", action="store_true", help="Print all JSON-RPC traffic")
    return parser


# ===== ENTRY POINT =====
def main():
    args = build_parser().parse_args()
    try:
        # Run async workflow (WebSocket + stream loop) inside event loop.
        asyncio.run(run_bridge(
            serial_port=args.serial_port,
            baud=args.baud,
            threshold=args.threshold,
            cooldown=args.cooldown,
            cortex_url=args.cortex_url,
            client_id=CLIENT_ID,
            client_secret=CLIENT_SECRET,
            license_key=LICENSE_KEY,
            headset_id=args.headset_id,
            profile=args.profile,
            debug=args.debug
        ))
    except KeyboardInterrupt:
        print("\n[SYS] Stopped")
    except Exception as exc:
        print(f"[ERROR] {exc}")
        raise


if __name__ == "__main__":
    main()
