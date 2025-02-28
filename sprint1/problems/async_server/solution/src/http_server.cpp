#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>
#include <syncstream>

namespace http_server {
    void SessionBase::Run() {
        // �������� ����� Read, ��������� executor ������� stream_.
        // ����� ������� ��� ������ �� stream_ ����� �����������, ��������� ��� executor
        net::dispatch(stream_.get_executor(),
            beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    void ReportError(beast::error_code ec, std::string_view str) {
        std::osyncstream(std::cout) << ec.what() << " Op: " << str << std::endl;
    }
}  // namespace http_server
