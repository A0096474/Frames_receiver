# Frames_receiver

Small collection of examples and utilities to receive image frames over the network and save them to disk.

Contents
- `receiver_cpp/` — C++ examples and helper programs (see `receiver_cpp/README.md` for full build/run instructions).

Quickstart
- Prerequisites: `cmake`, a C++17-capable compiler (MSVC/GCC/Clang), and optionally `vcpkg` to install dependencies such as `boost` and `nlohmann-json`.
- To build the C++ examples, follow the instructions in `receiver_cpp/README.md`.

Example: run the WebSocket receiver that saves binary frames

1. Build the project (from `receiver_cpp`):

```powershell
cmake -S receiver_cpp -B receiver_cpp/build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build receiver_cpp/build --config Release
```

2. Run the WebSocket receiver (binary may be `ws_receiver.exe` or `ws_main.exe` depending on the build):

```powershell
.\receiver_cpp\build\Release\ws_receiver.exe --out .\frames_received --host 0.0.0.0 --port 5000
```

Where to look next
- Full C++ example and details: `receiver_cpp/README.md`

License
- No license selected. Add a `LICENSE` file if you wish to make this project public with a specific license.
