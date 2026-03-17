#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <atomic>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>

#include <rtc/rtc.hpp>

// NOTE: This example uses libdatachannel (https://github.com/paullouisageneau/libdatachannel)
// for WebRTC and expects a separate WebSocket client to relay signaling messages.
// For simplicity this example uses a very small WebSocket client implementation
// based on WebSocket++ or similar libraries that you should provide when building.

using json = nlohmann::json;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::string signaling = "ws://localhost:8080/ws";
    std::string room = "testroom";
    std::string out_dir = "./frames_received";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "--signaling") && i + 1 < argc) signaling = argv[++i];
        else if ((a == "--room") && i + 1 < argc) room = argv[++i];
        else if ((a == "--out") && i + 1 < argc) out_dir = argv[++i];
    }

    fs::create_directories(out_dir);

    rtc::InitLogger(rtc::LogLevel::Info);

    // Create PeerConnection
    auto pc = std::make_shared<rtc::PeerConnection>();

    std::atomic<int> counter{0};

    pc->onDataChannel([&](std::shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel created: " << dc->label() << std::endl;

        dc->onOpen([dc]() {
            std::cout << "DataChannel open" << std::endl;
        });

        dc->onClosed([dc]() {
            std::cout << "DataChannel closed" << std::endl;
        });

        dc->onMessage([&](rtc::binary const& data) {
            int id = ++counter;
            std::string filename = (fs::path(out_dir) / ("frame_" + std::to_string(id) + ".png")).string();
            std::ofstream fh(filename, std::ios::binary);
            fh.write(reinterpret_cast<const char*>(data.data()), data.size());
            fh.close();
            std::cout << "Saved " << filename << " (" << data.size() << " bytes)" << std::endl;
        });
    });

    // When libdatachannel has a local description ready, use your signaling to send it.
    pc->onLocalDescription([&](rtc::Description const& d) {
        json m;
        m["type"] = d.typeString();
        m["sdp"] = d.sdp();
        // Send m to signaling server (e.g. via websocket): implementation-specific
        std::cout << "Local description ready (send via signaling):\n" << m.dump() << std::endl;
    });

    pc->onLocalCandidate([&](rtc::Candidate const& c) {
        json m;
        m["type"] = "ice";
        m["candidate"] = c.toJSON();
        // Send m to signaling server
        std::cout << "Local ICE candidate (send via signaling):\n" << m.dump() << std::endl;
    });

    std::cout << "Receiver ready.\n";
    std::cout << "IMPORTANT: This example prints the signaling messages (SDP/ICE) to stdout." << std::endl;
    std::cout << "You must forward those JSON messages to the sender via the /ws/{room} websocket endpoint, and relay incoming messages from the websocket into this program by calling the appropriate libdatachannel APIs (setRemoteDescription / addRemoteCandidate)." << std::endl;

    // Keep the process running while waiting for incoming frames
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
