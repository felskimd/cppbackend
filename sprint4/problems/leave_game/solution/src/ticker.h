#pragma once

#include <boost/asio.hpp>

namespace ticker {

namespace net = boost::asio;
namespace sys = boost::system;

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
        : strand_{ strand }
        , period_{ period }
        , handler_{ std::move(handler) } {
    }

    void Start() {
        net::dispatch(strand_, [self = shared_from_this(), last_tick = &last_tick_] {
            *last_tick = Clock::now();
            self->ScheduleTick();
            });
    }

private:
    void ScheduleTick() {
        if (!strand_.running_in_this_thread()) {
            throw std::logic_error("Wrong ticker logic");
        }
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
            });
    }

    void OnTick(sys::error_code ec) {
        using namespace std::chrono;
        if (!strand_.running_in_this_thread()) {
            throw std::logic_error("Wrong ticker logic");
        }

        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            try {
                handler_(delta);
            }
            catch (std::exception& ex) {
                std::cerr << "Auto tick exception: " << ex.what() << std::endl;
                throw ex;
            }
            ScheduleTick();
        }
    }

    using Clock = std::chrono::steady_clock;

    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{ strand_ };
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};

} //ticker