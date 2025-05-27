#pragma once

#include "model.h"

#include <condition_variable>
#include <pqxx/connection>
#include <pqxx/pqxx>
#include <pqxx/transaction>
#include <mutex>
#include <vector>

namespace database {

    class ConnectionPool {
        using PoolType = ConnectionPool;
        using ConnectionPtr = std::shared_ptr<pqxx::connection>;

    public:
        class ConnectionWrapper {
        public:
            ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
                : conn_{ std::move(conn) }
                , pool_{ &pool } {
            }

            ConnectionWrapper(const ConnectionWrapper&) = delete;
            ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

            ConnectionWrapper(ConnectionWrapper&&) = default;
            ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

            pqxx::connection& operator*() const& noexcept {
                return *conn_;
            }
            pqxx::connection& operator*() const&& = delete;

            pqxx::connection* operator->() const& noexcept {
                return conn_.get();
            }

            ~ConnectionWrapper() {
                if (conn_) {
                    pool_->ReturnConnection(std::move(conn_));
                }
            }

        private:
            std::shared_ptr<pqxx::connection> conn_;
            PoolType* pool_;
        };

        template <typename ConnectionFactory>
        ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
            pool_.reserve(capacity);
            for (size_t i = 0; i < capacity; ++i) {
                pool_.emplace_back(connection_factory());
            }
        }

        ConnectionWrapper GetConnection() {
            std::unique_lock lock{ mutex_ };
            // Ѕлокируем текущий поток и ждЄм, пока cond_var_ не получит уведомление и не освободитс€
            // хот€ бы одно соединение
            cond_var_.wait(lock, [this] {
                return used_connections_ < pool_.size();
                });
            // ѕосле выхода из цикла ожидани€ мьютекс остаЄтс€ захваченным

            return { std::move(pool_[used_connections_++]), *this };
        }

    private:
        void ReturnConnection(ConnectionPtr&& conn) {
            // ¬озвращаем соединение обратно в пул
            {
                std::lock_guard lock{ mutex_ };
                assert(used_connections_ != 0);
                pool_[--used_connections_] = std::move(conn);
            }
            // ”ведомл€ем один из ожидающих потоков об изменении состо€ни€ пула
            cond_var_.notify_one();
        }

        std::mutex mutex_;
        std::condition_variable cond_var_;
        std::vector<ConnectionPtr> pool_;
        size_t used_connections_ = 0;
    };

    void InitializeDB(std::shared_ptr<ConnectionPool> pool);

    struct StatInfo {
        std::string name;
        int score;
        double playtime;
    };

    class StatProvider {
    public:
        StatProvider(std::shared_ptr<ConnectionPool> pool) 
            : pool_(std::move(pool))
        { }

        std::vector<StatInfo> GetStats(int start_id, int max_items) const;

    private:
        std::shared_ptr<ConnectionPool> pool_;
    };
}

//stat provider