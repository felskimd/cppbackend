#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using Timer = net::steady_timer;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        using namespace std::chrono;

        mut_.lock();
        int id = GetId();
        auto bread = store_.GetBread();
        auto sausage = store_.GetSausage();
        mut_.unlock();
        auto already_done = std::make_shared<bool>(false);

        sausage->StartFry(*gas_cooker_, [this, id, handler, sausage_ptr = sausage->shared_from_this(), bread_ptr = bread->shared_from_this(), already_done, cooker_ptr = gas_cooker_->shared_from_this()] {
            auto timer = std::make_shared<Timer>(io_, HotDog::MIN_SAUSAGE_COOK_DURATION);
            timer->async_wait([timer, id, handler, already_done, bread_ptr2 = bread_ptr->shared_from_this(), sausage_ptr2 = sausage_ptr->shared_from_this(), cooker_ptr2 = cooker_ptr->shared_from_this()](sys::error_code ec) {
                if (ec) {
                    std::cerr << "Timer error: " << ec.message() << std::endl;
                    return;
                }
                sausage_ptr2->StopFry();
                OnSomethingReady(sausage_ptr2, bread_ptr2, id, handler, already_done);
            });
        });

        bread->StartBake(*gas_cooker_, [this, id, handler, sausage_ptr = sausage->shared_from_this(), bread_ptr = bread->shared_from_this(), already_done, cooker_ptr = gas_cooker_->shared_from_this()] {
            auto timer = std::make_shared<Timer>(io_, HotDog::MIN_BREAD_COOK_DURATION);
            timer->async_wait([timer, id, handler, already_done, bread_ptr2 = bread_ptr->shared_from_this(), sausage_ptr2 = sausage_ptr->shared_from_this(), cooker_ptr2 = cooker_ptr->shared_from_this()](sys::error_code ec) {
                if (ec) {
                    std::cerr << "Timer error: " << ec.message() << std::endl;
                    return;
                }
                bread_ptr2->StopBaking();
                OnSomethingReady(sausage_ptr2, bread_ptr2, id, handler, already_done);
            });
        });
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    int id_ = 0;
    std::mutex mut_;

    int GetId() {
        return id_++;
    }

    static void OnSomethingReady(std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread, int id, HotDogHandler handler, std::shared_ptr<bool> already_done) {
        if (sausage->IsCooked() && bread->IsCooked() && !*already_done) {
            *already_done = true;
            handler(Result<HotDog>(HotDog(id, std::move(sausage), std::move(bread))));
        }
    }
};
