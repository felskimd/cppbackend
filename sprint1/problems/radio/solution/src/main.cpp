#include <boost/asio.hpp>
#include "audio.h"

//#include <boost/asio.hpp>
#include <iostream>

using namespace std::literals;
namespace net = boost::asio;
using net::ip::udp;

static const std::string CLIENT_ARG = "client";
static const std::string SERVER_ARG = "server";
static const size_t max_buffer_size = 300000;

void StartServer(uint16_t port) {
    Player player(ma_format_u8, 1);

    boost::asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

    std::array<char, max_buffer_size> recv_buf;
    udp::endpoint remote_endpoint;

    auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);
    std::cout << "Message received" << std::endl;
    player.PlayBuffer(recv_buf.data(), recv_buf.size() / player.GetFrameSize(), 1.5s);
}

void StartClient(uint16_t port) {
    Recorder recorder(ma_format_u8, 1);

    net::io_context io_context;
    udp::socket socket(io_context, udp::v4());

    std::string str;

    std::cout << "Enter server IP to send message..." << std::endl;
    std::getline(std::cin, str);

    auto rec_result = recorder.Record(65000, 1.5s);
    std::cout << "Recording done" << std::endl;

    boost::system::error_code ec;
    auto endpoint = udp::endpoint(net::ip::make_address(str, ec), port);
    socket.send_to(net::buffer(rec_result.data.data(), rec_result.frames * recorder.GetFrameSize()), endpoint);
    std::cout << "Message sended" << std::endl;
}

int main(int argc, char** argv) {
    try {
        if (argv[1] == CLIENT_ARG) {
            StartClient(std::stoi(argv[2]));
        }
        else if (argv[1] == SERVER_ARG) {
            StartServer(std::stoi(argv[2]));
        }
        else {
            std::cout << "First argument wrong: " << argv[1] << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
