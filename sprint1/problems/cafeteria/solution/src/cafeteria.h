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
        auto sausage_strand = std::make_shared<net::strand<net::io_context::executor_type>>(net::make_strand(io_));
        auto bread_strand = std::make_shared<net::strand<net::io_context::executor_type>>(net::make_strand(io_));
        auto bread = store_.GetBread();
        auto sausage = store_.GetSausage();
        mut_.unlock();
        auto temp_result = std::make_shared<Result<HotDog>>(std::make_exception_ptr(std::runtime_error{ "No hotdog here(" }));
        auto already_done = std::make_shared<bool>(false);

        net::post(*sausage_strand, [id, handler, sausage_strand, cooker_ptr = gas_cooker_->shared_from_this(), sausage_ptr = sausage->shared_from_this(), bread_ptr1 = bread->shared_from_this(), already_done] {
            sausage_ptr->StartFry(*cooker_ptr, [id, handler, sausage_strand, sausage_ptr2 = sausage_ptr->shared_from_this(), bread_ptr2 = bread_ptr1->shared_from_this(), already_done, cooker_ptr2 = cooker_ptr->shared_from_this()] {
                auto timer = std::make_shared<Timer>(*sausage_strand, HotDog::MIN_SAUSAGE_COOK_DURATION);
                timer->async_wait([timer, id, handler, sausage_strand, already_done, bread_ptr3 = bread_ptr2->shared_from_this(), sausage_ptr3 = sausage_ptr2->shared_from_this(), cooker_ptr3 = cooker_ptr2->shared_from_this()](sys::error_code ec) {
                    if (ec) {
                        std::cerr << "Timer error: " << ec.message() << std::endl;
                        return;
                    }
                    net::post(*sausage_strand, [sausage_strand, id, handler, bread_ptr4 = bread_ptr3->shared_from_this(), sausage_ptr4 = sausage_ptr3->shared_from_this(), already_done, cooker_ptr4 = cooker_ptr3->shared_from_this()] {
                        sausage_ptr4->StopFry();
                        OnSomethingReady(sausage_ptr4, bread_ptr4, id, handler, already_done);
                    });
                });
            });
        });

        net::post(*bread_strand, [id, handler, sausage_ptr = sausage->shared_from_this(), cooker_ptr = gas_cooker_->shared_from_this(), bread_strand, bread_ptr1 = bread->shared_from_this(), already_done] {
            bread_ptr1->StartBake(*cooker_ptr, [id, handler, sausage_ptr2 = sausage_ptr->shared_from_this(), bread_ptr2 = bread_ptr1->shared_from_this(), bread_strand, already_done, cooker_ptr2 = cooker_ptr->shared_from_this()] {
                auto timer = std::make_shared<Timer>(*bread_strand, HotDog::MIN_BREAD_COOK_DURATION);
                timer->async_wait([timer, id, handler, already_done, bread_strand, bread_ptr3 = bread_ptr2->shared_from_this(), sausage_ptr3 = sausage_ptr2->shared_from_this(), cooker_ptr3 = cooker_ptr2->shared_from_this()](sys::error_code ec) {
                    if (ec) {
                        std::cerr << "Timer error: " << ec.message() << std::endl;
                        return;
                    }
                    net::post(*bread_strand, [bread_strand, id, handler, bread_ptr4 = bread_ptr3->shared_from_this(), sausage_ptr4 = sausage_ptr3->shared_from_this(), already_done, cooker_ptr4 = cooker_ptr3->shared_from_this()] {
                        bread_ptr4->StopBaking();
                        OnSomethingReady(sausage_ptr4, bread_ptr4, id, handler, already_done);
                    });
                });
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
