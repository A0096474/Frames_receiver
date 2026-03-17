#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using json = nlohmann::json;
namespace fs = std::filesystem;

static std::string sanitize_filename(const std::string& name) {
    // Keep only the filename component and replace forbidden characters
    fs::path p{name};
    std::string base = p.filename().string();
    std::string out;
    for (char c : base) {
        // disallow path separators and common Windows forbidden chars
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            out.push_back('_');
        else if (static_cast<unsigned char>(c) < 32)
            out.push_back('_');
        else
            out.push_back(c);
    }
    if (out.empty()) out = "frame";
    return out;
}

void save_binary_message(const std::vector<char>& data, const fs::path& out_dir, const std::string& desired_name, int index) {
    fs::create_directories(out_dir);
    std::string filename;
    if (!desired_name.empty()) {
        filename = sanitize_filename(desired_name);
        // ensure extension exists
        if (!fs::path(filename).has_extension()) filename += ".png";
    } else {
        filename = "received_" + std::to_string(index) + ".png";
    }

    auto tmp = out_dir / (filename + ".part");
    auto finalp = out_dir / filename;
    std::ofstream fh(tmp, std::ios::binary);
    fh.write(data.data(), static_cast<std::streamsize>(data.size()));
    fh.flush();
    fh.close();
    std::error_code ec;
    fs::rename(tmp, finalp, ec);
    if (ec) {
        std::cerr << "Warning: rename failed: " << ec.message() << std::endl;
    }
}

int main(int argc, char** argv) {
    unsigned short port = 5000;
    std::string host = "0.0.0.0";
    fs::path out_dir = "./frames_received";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "--port") && i + 1 < argc) port = static_cast<unsigned short>(std::stoi(argv[++i]));
        else if ((a == "--host") && i + 1 < argc) host = argv[++i];
        else if ((a == "--out") && i + 1 < argc) out_dir = argv[++i];
    }

    try {
        asio::io_context ioc{1};
        tcp::endpoint endpoint{asio::ip::make_address(host), port};
        tcp::acceptor acceptor{ioc, endpoint};

        std::cout << "WebSocket C++ POC server listening on ws://" << host << ":" << port << "/frames" << std::endl;

        int connection_count = 0;
        while (true) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            ++connection_count;
            std::cout << "Accepted connection #" << connection_count << " from " << socket.remote_endpoint() << std::endl;

            try {
                websocket::stream<tcp::socket> ws{std::move(socket)};
                // Accept the websocket handshake
                ws.accept();

                beast::flat_buffer buffer;
                int frame_counter = 0;
                std::string pending_filename;

                while (true) {
                    buffer.clear();
                    ws.read(buffer);
                    bool got_text = ws.got_text();
                    if (got_text) {
                        std::string msg = beast::buffers_to_string(buffer.data());
                        std::string t = msg;
                        // simple handshake
                        if (t.rfind("HELLO", 0) == 0) {
                            ws.text(true);
                            ws.write(asio::buffer(std::string("OK")));
                            std::cout << "Handshake: replied OK" << std::endl;
                        } else if (t == "DONE") {
                            std::cout << "Received DONE, closing websocket" << std::endl;
                            ws.close(websocket::close_code::normal);
                            break;
                        } else {
                            // Try to parse JSON metadata (e.g. {"type":"frame","filename":"name.png"})
                            try {
                                auto j = json::parse(t);
                                if (j.is_object() && j.contains("type") && j["type"] == "frame" && j.contains("filename")) {
                                    pending_filename = j["filename"].get<std::string>();
                                    std::cout << "Pending filename set: " << pending_filename << std::endl;
                                } else {
                                    std::cout << "Text message: " << t << std::endl;
                                }
                            } catch (const std::exception& e) {
                                std::cout << "Text message (non-JSON): " << t << std::endl;
                            }
                        }
                        
                    } else {
                        // binary message: save to file
                        ++frame_counter;
                        auto const buffers = buffer.data();
                        std::size_t bytes = beast::buffer_bytes(buffers);
                        std::vector<char> data(bytes);
                        asio::buffer_copy(asio::buffer(data), buffers);
                        // determine output filename (use pending_filename if present)
                        std::string used_name = pending_filename;
                        save_binary_message(data, out_dir, used_name, frame_counter);
                        std::cout << "Saved frame " << frame_counter << " (" << data.size() << " bytes)" << std::endl;

                        // send ACK JSON (include filename when available)
                        json ack;
                        ack["type"] = "ack";
                        ack["index"] = frame_counter;
                        if (!used_name.empty()) ack["filename"] = used_name;
                        auto s = ack.dump();
                        ws.text(true);
                        ws.write(asio::buffer(s));

                        // clear pending filename after use
                        pending_filename.clear();
                    }
                }

            } catch (const beast::system_error& se) {
                std::cerr << "WebSocket session error: " << se.code().message() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Session exception: " << e.what() << std::endl;
            }

            std::cout << "Connection handler finished; ready for next connection." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
