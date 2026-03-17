# C++ WebRTC PNG Receiver (libdatachannel)

This small project shows how to receive PNG files sent over a WebRTC DataChannel and save them to disk using C++ and libdatachannel.

IMPORTANT: This example intentionally keeps signaling separate. The program prints the SDP/ICE JSON objects to stdout and expects you to relay those messages to the existing signaling server (`/ws/{room}`) and forward incoming signaling messages from the server into this program. See the notes below for wiring options.

Dependencies (recommended via vcpkg):

```powershell
vcpkg install libdatachannel nlohmann-json websocketpp openssl asio
```

Build (example):

```powershell
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

Run:

```powershell
.\receiver.exe --signaling ws://YOUR_SERVER:8080/ws --room myroom --out .\frames_received
```

How to wire signaling (options):
- Quick manual test: run the receiver and the sender; copy the JSON printed by the receiver to the sender's websocket (or use the signaling server web UI) and vice-versa.
- Better: extend this program to include a WebSocket client and automatically connect to the same `/ws/{room}` endpoint as the sender. Use `websocketpp`, `ixwebsocket` or another client library to relay text JSON messages between the server and the libdatachannel API (`setRemoteDescription` and `addRemoteCandidate`).

Notes:
- `libdatachannel` gives you DataChannel callbacks; the example writes binary frames to `--out` directory as `frame_1.png`, `frame_2.png`, ...
- For Internet connections where direct P2P fails, configure TURN servers on both peers.
