#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(std::string_view body, unsigned http_version,
        bool keep_alive, bool is_head_method,
        std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(http::status::ok, http_version);
    response.set(http::field::content_type, content_type);
    if (!is_head_method) {
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
    switch (req.method()) {
    case http::verb::get:
        return MakeStringResponse(ss.view(), req.version(), req.keep_alive(), false);
    case http::verb::head:
        return MakeStringResponse(ss.view(), req.version(), req.keep_alive(), true);
    default:
        return GetMethodNotAllowedResponse(req.version(), req.keep_alive());
    }
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, {address, port}, [](auto&& req, auto&& sender) {
        sender(HandleRequest(std::forward<decltype(req)>(req)));
    });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
}
