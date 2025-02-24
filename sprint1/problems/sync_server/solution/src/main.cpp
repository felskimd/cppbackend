#ifdef WIN32
#include <sdkddkver.h>
#endif
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

StringResponse MakeStringResponse(std::string_view body, unsigned http_version,
    bool keep_alive, bool is_head_method,
    std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(http::status::ok, http_version);
    response.set(http::field::content_type, content_type);
    if (is_head_method) {
        response.body() = body;
    }
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse GetMethodNotAllowedResponse(unsigned http_version, bool keep_alive) {
    StringResponse response(http::status::method_not_allowed, http_version);
    response.set(http::field::content_type, ContentType::TEXT_HTML);
    response.set(http::field::allow, "GET, HEAD"sv);
    response.body() = "Invalid method"sv;
    response.content_length(14);
    return response;
}

StringResponse HandleRequest(StringRequest&& req) {
    std::stringstream ss;
    ss << "Hello, " << req.target().substr(1, req.target().size() - 1);
    std::cout << ss.view() << std::endl;
    switch (req.method()) {
    case http::verb::get:
        return MakeStringResponse(ss.view(), req.version(), req.keep_alive(), false);
    case http::verb::head:
        return MakeStringResponse(ss.view(), req.version(), req.keep_alive(), true);
    default:
        return GetMethodNotAllowedResponse(req.version(), req.keep_alive());
    }
}

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }
    if (ec) {
        throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }
    return req;
}

template <typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& handle_request) {
    try {
        beast::flat_buffer buffer;
        while (auto request = ReadRequest(socket, buffer)) {
            StringResponse response = handle_request(*std::move(request));
            http::write(socket, response);
            if (response.need_eof()) {
                break;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main() {
    net::io_context ioc;

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    tcp::acceptor acceptor(ioc, { address, port });
    std::cout << "Server has started..."sv << std::endl;

    while (true) {
        tcp::socket socket(ioc);
        acceptor.accept(socket);

        std::thread t(
            [](tcp::socket socket) {
                HandleConnection(socket, HandleRequest);
            },
            std::move(socket));
        t.detach();
    }
}
