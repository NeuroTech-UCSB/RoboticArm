import asyncio
import websockets

async def handle_connection(websocket):
    # This loop keeps the connection alive for this specific client
    async for message in websocket:
        print(f"Received from client: {message}")
        
        # Send a response back
        greeting = f"Server says: I got your message -> {message}"
        await websocket.send(greeting)

async def serve():
    # Start the server on localhost, port 8765
    async with websockets.serve(handle_connection, "localhost", 8765):
        print("Server started on ws://localhost:8765")
        await asyncio.Future()  # Run forever

async def test_client():
    uri = "ws://localhost:8765"
    async with websockets.connect(uri) as websocket:
        # Send a test message to the server
        test_message = "Hello, Server!"
        print(f"Client sending: {test_message}")
        await websocket.send(test_message)
        
        # Wait for a response from the server
        response = await websocket.recv()
        print(f"Client received: {response}")

async def main():
    # Start the server in the background
    server_task = asyncio.create_task(serve())
    
    # Give the server a moment to start
    await asyncio.sleep(1)
    
    # Run the test client
    await test_client()
    
    # Optionally, you can cancel the server task after testing
    server_task.cancel()

if __name__ == "__main__":
    asyncio.run(main())
