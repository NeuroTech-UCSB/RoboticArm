import os

from emotiv_robot_arm.bridge import CortexClient, parse_com_event

import asyncio
from websockets.asyncio.client import connect
import ssl
import certifi
import json
from dotenv import load_dotenv

# ===== CONFIGURATION =====
load_dotenv()
# Default values keep command line simpler for quick demos.
DEFAULT_CORTEX_URL = "wss://localhost:6868"
DEFAULT_CONF_THRESHOLD = 0.45
DEFAULT_COOLDOWN_SEC = 0.25
CLIENT_ID = os.getenv("EMOTIV_CLIENT_ID")
CLIENT_SECRET = os.getenv("EMOTIV_CLIENT_SECRET")
LICENSE_KEY = os.getenv("EMOTIV_LICENSE_KEY")

async def cortex_connect(headset_id = None, profile = None):
    ssl_context = ssl._create_unverified_context(cafile=certifi.where())

    try:
        # SSL=True because Cortex endpoint is wss://
        async with connect(DEFAULT_CORTEX_URL, ssl=ssl_context) as ws:
            cortex = CortexClient(ws, debug=True)

            token = await cortex.authorize(
                client_id=CLIENT_ID,
                client_secret=CLIENT_SECRET,
                license_key=LICENSE_KEY,
            )

            headsets = await cortex.query_headsets()
            if not headsets:
                raise RuntimeError("No headset found. Open Emotiv Launcher and connect your headset first.")

            # Use requested headset id or first available.
            headset_id = headset_id or headsets[0]["id"]
            await cortex.connect_headset(headset_id)
            # Short wait for headset connection state to settle.
            await asyncio.sleep(2.0)

            if profile:
                # Optional: load trained profile before streaming commands.
                await cortex.setup_profile(token, headset_id, profile)

            # Start active session and subscribe to "com" stream.
            session_id = await cortex.create_session(token, headset_id)
            await cortex.subscribe_com(token, session_id)

            print("[RUN] Waiting for mental commands (Ctrl+C to stop)")
            while True:
                # Receive any event (stream data, status, etc.)
                raw = await ws.recv()
                msg = json.loads(raw)
                # Extract mental command action + confidence if present.
                action, confidence = parse_com_event(msg)
                if action is None or confidence is None:
                    # Many messages are not command events; skip them.
                    continue

                print("action: ", action)
                print("confidence: ", confidence)

    except Exception as e:
        print(f"[ERROR] {e}")

def main():
    try:
        asyncio.run(cortex_connect())
    except KeyboardInterrupt:
        print("\n[SYS] Stopped")
    except Exception as exc:
        print(f"[ERROR] {exc}")
        raise


if __name__ == "__main__":
    main()
