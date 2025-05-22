#include "db.h"

namespace database {
    void InitializeDB(ConnectionPool& pool) {
        auto conn = pool.GetConnection();
        pqxx::work work{ *conn };
        work.exec(R"(
CREATE TABLE IF NOT EXISTS retired_players (
    id SERIAL PRIMARY KEY,
    name varchar(100) NOT NULL,
    score int NOT NULL,
    playtime double precision NOT NULL
);
)");
        work.exec(R"(
CREATE INDEX score_idx ON retired_players (score);
)");
        work.exec(R"(
CREATE INDEX playtime_idx ON retired_players (playtime);
)");
        work.exec(R"(
CREATE INDEX name_idx ON retired_players (name);
)");
        work.commit();
    }

    std::vector<StatInfo> StatProvider::GetStats(int start_id, int max_items) const {
        auto conn = pool_->GetConnection();
        pqxx::work work{ *conn };
        std::vector<StatInfo> stats;
        pqxx::result result = work.exec_params(R"(
SELECT name, score, playtime FROM retired_players
ORDER BY score DESC, playtime ASC, name ASC
LIMIT $1 OFFSET $2;
)", max_items, start_id);
        work.commit();
        for (const auto& row : result) {
            auto [name, score, playtime] = row.as<std::string, int, double>();
            stats.emplace_back(name, score, playtime);
        }
        return stats;
    }
}