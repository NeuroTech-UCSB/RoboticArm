from cortex_bridge import run_bridge

import argparse
import asyncio

# ===== CONFIGURATION =====
# Default values keep command line simpler for quick demos.
DEFAULT_CORTEX_URL = "wss://localhost:6868"
DEFAULT_BAUD = 115200
DEFAULT_CONF_THRESHOLD = 0.45
DEFAULT_COOLDOWN_SEC = 0.25


# ===== ENV LOADING =====
def load_env():
    client_id = None
    client_secret = None
    license_key = None

    with open('data.txt', 'r', encoding='utf-8') as file:
        for line in file:
            line = line.strip()
            if line.startswith("CLIENT_ID="):
                client_id = line.split("=", 1)[1].strip()
            elif line.startswith("CLIENT_SECRET="):
                client_secret = line.split("=", 1)[1].strip()
            elif line.startswith("LICENSE_KEY="):
                license_key = line.split("=", 1)[1].strip()

    if not client_id:
        raise ValueError("CLIENT_ID not found! Did you run setup_env.py?")

    if not client_secret:
        raise ValueError("CLIENT_SECRET not found! Did you run setup_env.py?")
    
    # LICENSE_KEY is optional, so we won't raise an error if it's missing. Just print a warning.
    if not license_key:
        print("⚠️  LICENSE_KEY not found. Continuing without license key (if your app doesn't require one).")

    return client_id, client_secret, license_key

# ===== CLI ARGUMENTS =====
def build_parser():
    parser = argparse.ArgumentParser(description="Emotiv Cortex API mental command bridge to Arduino serial.")
    # Required credentials/ports are passed through command line arguments.
    parser.add_argument("--cortex-url", default=DEFAULT_CORTEX_URL, help=f"Cortex WebSocket URL (default: {DEFAULT_CORTEX_URL})")
    parser.add_argument("--headset-id", default="", help="Optional specific headset ID")
    parser.add_argument("--profile", default="", help="Optional trained mental-command profile to load")
    parser.add_argument("--serial-port", required=True, help="Arduino serial port (e.g. /dev/cu.usbmodem1101)")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help=f"Serial baud (default: {DEFAULT_BAUD})")
    parser.add_argument("--threshold", type=float, default=DEFAULT_CONF_THRESHOLD, help="Min confidence to send command")
    parser.add_argument("--cooldown", type=float, default=DEFAULT_COOLDOWN_SEC, help="Seconds between same command repeats")
    parser.add_argument("--debug", action="store_true", help="Print all JSON-RPC traffic")
    return parser


# ===== ENTRY POINT =====
def main():
    client_id, client_secret, license_key = load_env()
    args = build_parser().parse_args()
    try:
        # Run async workflow (WebSocket + stream loop) inside event loop.
        asyncio.run(run_bridge(
            serial_port=args.serial_port,
            baud=args.baud,
            threshold=args.threshold,
            cooldown=args.cooldown,
            cortex_url=args.cortex_url,
            client_id=client_id,
            client_secret=client_secret,
            license_key=license_key,
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
