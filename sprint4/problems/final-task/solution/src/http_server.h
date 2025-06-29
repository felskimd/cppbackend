#pragma once
#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace http_server {

    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;

    using namespace std::literals;

    void ReportError(beast::error_code ec, std::string_view where);

    class SessionBase {
    public:
        // Запрещаем копирование и присваивание объектов SessionBase и его наследников
        SessionBase(const SessionBase&) = delete;
        SessionBase& operator=(const SessionBase&) = delete;

        void Run();

    protected:
        beast::net::ip::address address_;
        using HttpRequest = http::request<http::string_body>;

        ~SessionBase() = default;

        explicit SessionBase(tcp::socket&& socket);

        template <typename Body, typename Fields>
        void Write(http::response<Body, Fields>&& response) {
            // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
            auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

            auto self = GetSharedThis();
            http::async_write(stream_, *safe_response,
                [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                    self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                });
        }

    private:
        // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        HttpRequest request_;

        void Read();

        void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);

        void Close();

        void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);

        // Обработку запроса делегируем подклассу
        virtual void HandleRequest(HttpRequest&& request) = 0;

        virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
    };

    template <typename RequestHandler>
    class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
    public:
        template <typename Handler>
        Session(tcp::socket&& socket, Handler&& request_handler)
            : SessionBase(std::move(socket))
            , request_handler_(std::forward<Handler>(request_handler)) {
        }

    private:
        RequestHandler request_handler_;

        void HandleRequest(HttpRequest&& request) override {
            // Захватываем умный указатель на текущий объект Session в лямбде,
            // чтобы продлить время жизни сессии до вызова лямбды.
            // Используется generic-лямбда функция, способная принять response произвольного типа
            request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
                self->Write(std::move(response));
                }, address_);
        }

        std::shared_ptr<SessionBase> GetSharedThis() override {
            return this->shared_from_this();
        }
    };

    template <typename RequestHandler>
    class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
    public:
        template <typename Handler>
        Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
            : ioc_(ioc)
            // Обработчики асинхронных операций acceptor_ будут вызываться в своём strand
            , acceptor_(net::make_strand(ioc))
            , request_handler_(std::forward<Handler>(request_handler)) {
            // Открываем acceptor, используя протокол (IPv4 или IPv6), указанный в endpoint
            acceptor_.open(endpoint.protocol());

            // После закрытия TCP-соединения сокет некоторое время может считаться занятым,
            // чтобы компьютеры могли обменяться завершающими пакетами данных.
            // Однако это может помешать повторно открыть сокет в полузакрытом состоянии.
            // Флаг reuse_address разрешает открыть сокет, когда он "наполовину закрыт"
            acceptor_.set_option(net::socket_base::reuse_address(true));
            // Привязываем acceptor к адресу и порту endpoint
            acceptor_.bind(endpoint);
            // Переводим acceptor в состояние, в котором он способен принимать новые соединения
            // Благодаря этому новые подключения будут помещаться в очередь ожидающих соединений
            acceptor_.listen(net::socket_base::max_listen_connections);
        }

        void Run() {
            DoAccept();
        }

    private:
        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        RequestHandler request_handler_;

        void AsyncRunSession(tcp::socket&& socket) {
            std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
        }

        void DoAccept() {
            acceptor_.async_accept(
                // Передаём последовательный исполнитель, в котором будут вызываться обработчики
                // асинхронных операций сокета
                net::make_strand(ioc_),
                // С помощью bind_front_handler создаём обработчик, привязанный к методу OnAccept
                // текущего объекта.
                // Так как Listener — шаблонный класс, нужно подсказать компилятору, что
                // shared_from_this — метод класса, а не свободная функция.
                // Для этого вызываем его, используя this
                // Этот вызов bind_front_handler аналогичен
                // namespace ph = std::placeholders;
                // std::bind(&Listener::OnAccept, this->shared_from_this(), ph::_1, ph::_2)
                beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
        }

        void OnAccept(sys::error_code ec, tcp::socket socket) {
            using namespace std::literals;

            if (ec) {
                return ReportError(ec, "accept"sv);
            }

            // Асинхронно обрабатываем сессию
            AsyncRunSession(std::move(socket));

            // Принимаем новое соединение
            DoAccept();
        }
    };

    template <typename RequestHandler>
    void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
        using MyListener = Listener<std::decay_t<RequestHandler>>;

        std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
    }

}  // namespace http_server
