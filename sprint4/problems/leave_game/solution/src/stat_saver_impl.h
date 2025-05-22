#pragma once

#include "db.h"
#include "model.h"

namespace model {

class StatSaverImpl : public StatSaver {
public:
    StatSaverImpl(std::shared_ptr<database::ConnectionPool> pool)
        : pool_(std::move(pool)) 
    { }

    void Save(const std::vector<SaveStat>& stats) override {
        auto conn = pool_->GetConnection();
        pqxx::work work {*conn};
        for (const auto& stat : stats) {
            work.exec_params(R"(
INSERT INTO retired_players (name, score, playtime) VALUES ($1, $2, $3);
)", stat.name, stat.scores, stat.playtime);
        }
        work.commit();
    }

private:
    std::shared_ptr<database::ConnectionPool> pool_;
};

} //model