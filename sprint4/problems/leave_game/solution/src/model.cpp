#include "model.h"

#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace model {
    using namespace std::literals;

    static const double MAX_DELTA = 0.4;

    void Map::AddOffice(Office office) {
        if (warehouse_id_to_index_.contains(office.GetId())) {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        try {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        }
        catch (std::exception& ex) {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }

    void Game::AddMap(Map map) {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        }
        else {
            try {
                maps_.emplace_back(std::move(map));
            }
            catch (std::exception& ex) {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    std::vector<const Dog*> GameSession::GetDogs() const {
        std::vector<const Dog*> result;
        for (const auto& [id, dog] : dogs_) {
            result.emplace_back(&dog);
        }
        return result;
    }

    Dog* GameSession::AddDog(Dog&& dog) {
        auto roads = map_->GetRoads();
        if (randomize_spawn_) {
            std::default_random_engine generator(roads.size());
            std::uniform_int_distribution rand_road{ 0, static_cast<int>(roads.size()) - 1 };
            auto& road = roads[rand_road(generator)];
            if (road.IsHorizontal()) {
                std::uniform_real_distribution rand_pos{ static_cast<double>(road.GetStart().x), static_cast<double>(road.GetEnd().x) };
                dog.SetPosition({ rand_pos(generator), static_cast<double>(road.GetStart().y) });
            }
            else {
                std::uniform_real_distribution rand_pos{ static_cast<double>(road.GetStart().y), static_cast<double>(road.GetEnd().y) };
                dog.SetPosition({ static_cast<double>(road.GetStart().x), rand_pos(generator) });
            }
        }
        else {
            const auto& road_start = roads[0].GetStart();
            dog.SetPosition({ static_cast<double>(road_start.x), static_cast<double>(road_start.y) });
        }
        dog.SetObserver(this);
        dog.ResetDirection();
        dog.Stop();
        auto id = dog.GetId();
        dogs_.insert({id, std::move(dog)});
        dogs_playtime_[id];
        return &dogs_.at(id);
    }

    size_t Dog::start_id_ = 0;

    GameSession::GameSession(Map* map, bool randomize_spawn, unsigned dog_retirement_time, std::shared_ptr<model::StatSaver> stat_saver)
        : map_(map)
        , randomize_spawn_(randomize_spawn)
        , dog_retirement_time_(dog_retirement_time)
        , stat_saver_(stat_saver)
    {
        for (const auto& road : map_->GetRoads()) {
            auto start = road.GetStart();
            auto end = road.GetEnd();
            if (road.IsHorizontal()) {
                int start_x, end_x;
                if (start.x < end.x) {
                    start_x = start.x;
                    end_x = end.x;
                }
                else {
                    start_x = end.x;
                    end_x = start.x;
                }
                for (; start_x <= end_x; ++start_x) {
                    roads_graph_[{start_x, start.y}].push_back(&road);
                }
            }
            else {
                int start_y, end_y;
                if (start.y < end.y) {
                    start_y = start.y;
                    end_y = end.y;
                }
                else {
                    start_y = end.y;
                    end_y = start.y;
                }
                for (; start_y <= end_y; ++start_y) {
                    roads_graph_[{start.x, start_y}].push_back(&road);
                }
            }
        }
    }

    bool InBounds(const Road* road, Position pos) {
        auto start_point = road->GetStart();
        auto end_point = road->GetEnd();
        double max_x;
        double min_x;
        double max_y;
        double min_y;
        if (road->IsHorizontal()) {
            if (start_point.x < end_point.x) {
                min_x = static_cast<double>(start_point.x) - MAX_DELTA;
                max_x = static_cast<double>(end_point.x) + MAX_DELTA;
            }
            else {
                min_x = static_cast<double>(end_point.x) - MAX_DELTA;
                max_x = static_cast<double>(start_point.x) + MAX_DELTA;
            }
            min_y = static_cast<double>(start_point.y) - MAX_DELTA;
            max_y = static_cast<double>(start_point.y) + MAX_DELTA;
        }
        else {
            if (start_point.y < end_point.y) {
                min_y = static_cast<double>(start_point.y) - MAX_DELTA;
                max_y = static_cast<double>(end_point.y) + MAX_DELTA;
            }
            else {
                min_y = static_cast<double>(end_point.y) - MAX_DELTA;
                max_y = static_cast<double>(start_point.y) + MAX_DELTA;
            }
            min_x = static_cast<double>(start_point.x) - MAX_DELTA;
            max_x = static_cast<double>(start_point.x) + MAX_DELTA;
        }
        if (pos.x <= max_x && pos.x >= min_x && pos.y <= max_y && pos.y >= min_y) {
            return true;
        }
        return false;
    }

    double GetMaxPossible(const Road* road, Direction dir) {
        switch (dir) {
        case Direction::NORTH:
            return static_cast<double>(std::min(road->GetEnd().y, road->GetStart().y)) - MAX_DELTA;
        case Direction::SOUTH:
            return static_cast<double>(std::max(road->GetEnd().y, road->GetStart().y)) + MAX_DELTA;
        case Direction::EAST:
            return static_cast<double>(std::max(road->GetEnd().x, road->GetStart().x)) + MAX_DELTA;
        case Direction::WEST:
            return static_cast<double>(std::min(road->GetEnd().x, road->GetStart().x)) - MAX_DELTA;
        }
        return 0.;
    }

    std::pair<bool, Position> GameSession::CalculateMove(Position pos, Speed speed, unsigned delta) const {
        double in_seconds = static_cast<double>(delta) / MILLISECONDS_IN_SECOND;
        Position end_pos = { pos.x + speed.vx * in_seconds, pos.y + speed.vy * in_seconds };
        Point rounded = { static_cast<int>(std::round(pos.x)), static_cast<int>(std::round(pos.y)) };
        for (const auto& road : roads_graph_.at(rounded)) {
            if (InBounds(road, end_pos)) {
                return { false, end_pos };
            }
            if (road->IsHorizontal() && speed.vy == 0.) {
                if (speed.vx > 0) {
                    return { true, {GetMaxPossible(road, Direction::EAST), pos.y} };
                }
                return { true, {GetMaxPossible(road, Direction::WEST), pos.y} };
            }
            if (road->IsVertical() && speed.vx == 0.) {
                if (speed.vy > 0) {
                    return { true, {pos.x, GetMaxPossible(road, Direction::SOUTH)} };
                }
                return { true, {pos.x, GetMaxPossible(road, Direction::NORTH)} };
            }
        }
        if (speed.vx == 0) {
            end_pos = { pos.x, speed.vy > 0 ? std::round(pos.y) + MAX_DELTA : std::round(pos.y) - MAX_DELTA };
            return { true, end_pos };
        }
        end_pos = { speed.vx > 0 ? std::round(pos.x) + MAX_DELTA : std::round(pos.x) - MAX_DELTA, pos.y };
        return { true, end_pos };
    }

    std::pair<std::unordered_map<size_t, Position>, std::vector<size_t>> GameSession::CalculatePositions(unsigned delta) const {
        std::unordered_map<size_t, Position> new_positions;
        std::vector<size_t> dogs_to_stop;
        for (auto& [id, dog] : dogs_) {
            if (dog.GetSpeed() != Speed{}) {
                auto [stop, new_pos] = CalculateMove(dog.GetPosition(), dog.GetSpeed(), delta);
                new_positions[id] = new_pos;
                if (stop) {
                    dogs_to_stop.push_back(id);
                }
            }
        }
        return {new_positions, dogs_to_stop};
    }

    std::pair<std::vector<collision_detector::Gatherer>, std::unordered_map<size_t, size_t>> GameSession::PrepareDogs(const std::unordered_map<size_t, Position>& calculated_positions) const {
        std::vector<collision_detector::Gatherer> gatherers;
        std::unordered_map<size_t, size_t> gatherer_id_to_dog_id;
        size_t i = 0;
        for (const auto [id, pos] : calculated_positions) {
            auto temp_start = dogs_.at(id).GetPosition();
            geom::Point2D start{ temp_start.x, temp_start.y };
            auto temp_end = calculated_positions.at(id);
            geom::Point2D end{ temp_end.x, temp_end.y };
            gatherers.emplace_back(start, end, CollisionWidths::DOG_WIDTH / 2);
            gatherer_id_to_dog_id[i] = id;
            ++i;
        }
        return { gatherers, gatherer_id_to_dog_id };
    }

    void GameSession::UpdateDogsPositions(const std::unordered_map<size_t, Position>& positions, const std::vector<size_t>& dogs_to_stop) {
        for (const auto [id, pos] : positions) {
            dogs_.at(id).SetPosition(pos);
        }
        for (const auto id : dogs_to_stop) {
            dogs_.at(id).Stop();
        }
    }

    std::pair<std::vector<collision_detector::Item>, std::unordered_map<size_t, GameSession::ItemData>> GameSession::PrepareCollisionItems() const {
        std::vector<collision_detector::Item> items;
        std::unordered_map<size_t, GameSession::ItemData> item_data;
        size_t i = 0;
        for (const auto& office : map_->GetOffices()) {
            auto pos = office.GetPosition();
            geom::Point2D position;
            position.x = static_cast<double>(pos.x);
            position.y = static_cast<double>(pos.y);
            collision_detector::Item item{ position, CollisionWidths::OFFICE_WIDTH / 2 };
            items.emplace_back(std::move(item));
            item_data[i] = {ItemType::OFFICE, i};
            ++i;
        }
        for (const auto& [id, data] : loot_map_) {
            geom::Point2D position{ data.second.x, data.second.y };
            collision_detector::Item item{ position, CollisionWidths::ITEM_WIDTH };
            items.emplace_back(std::move(item));
            item_data[i] = {ItemType::LOOT, id};
            ++i;
        }
        return {items, item_data};
    }

    void GameSession::ProcessEvents(const std::vector<collision_detector::GatheringEvent>& events
                                , const std::unordered_map<size_t, ItemData>& items_data
                                , const std::unordered_map<size_t, size_t>& gatherer_id_to_dog_id) {
        std::unordered_set<size_t> taken_loot;
        for (const auto& event : events) {
            auto& item_data = items_data.at(event.item_id);
            auto& dog = dogs_.at(gatherer_id_to_dog_id.at(event.gatherer_id));
            if (item_data.type == GameSession::ItemType::LOOT) {
                if (!taken_loot.contains(item_data.outer_id) && dog.CanTakeLoot()) {
                    dog.TakeLoot({ item_data.outer_id, loot_map_.at(item_data.outer_id).first });
                    taken_loot.insert(item_data.outer_id);
                    loot_map_.erase(item_data.outer_id);
                }
            }
            else {
                auto items = dog.TransferItems();
                for (const auto& item : items) {
                    dog.AddScore(map_->GetValue(item.type));
                }
            }
        }
    }

    void GameSession::SpawnLoot(unsigned loot_count) {
        auto roads = map_->GetRoads();
        std::default_random_engine generator(roads.size());
        std::uniform_int_distribution rand_road{ 0, static_cast<int>(roads.size()) - 1 };
        std::uniform_int_distribution rand_type{ 0, static_cast<int>(map_->GetLootTypesCount() - 1) };
        for (unsigned i = 0; i < loot_count; ++i) {
            auto& road = roads[rand_road(generator)];
            std::uniform_real_distribution deviation{-MAX_DELTA, MAX_DELTA};
            Position loot_pos;
            if (road.IsHorizontal()) {
                double max = static_cast<double>(std::max(road.GetStart().x, road.GetEnd().x));
                double min = static_cast<double>(std::min(road.GetStart().x, road.GetEnd().x));
                std::uniform_real_distribution rand_pos{ min, max };
                loot_pos = { rand_pos(generator), road.GetStart().y + deviation(generator)};
            }
            else {
                double max = static_cast<double>(std::max(road.GetStart().y, road.GetEnd().y));
                double min = static_cast<double>(std::min(road.GetStart().y, road.GetEnd().y));
                std::uniform_real_distribution rand_pos{ min, max };
                loot_pos = { road.GetStart().x + deviation(generator), rand_pos(generator) };
            }
            loot_map_[GetNextLootId()] = {rand_type(generator), loot_pos};
        }
        loot_count_ += loot_count;
    }

    unsigned GameSession::GetNextLootId() {
        return loot_id_++;
    }

    std::unordered_set<size_t> GameSession::ObserveAFKAndPlaytime(unsigned delta) {
        std::unordered_set<size_t> result;
        for (auto& [id, time] : afk_dogs_) {
            time += delta;
            if (time >= dog_retirement_time_) {
                result.insert(id);
            }
        }
        for (const auto id : result) {
            afk_dogs_.erase(id);
        }

        for (auto& [id, time] : dogs_playtime_) {
            time += delta;
        }

        return result;
    }

    void GameSession::RetireDogs(const std::unordered_set<size_t>& ids) {
        std::vector<SaveStat> stats;
        for (const auto id : ids) {
            auto& dog = dogs_.at(id);
            stats.emplace_back(dog.GetName(), dog.GetScore(), dogs_playtime_.at(dog.GetId()));
            dogs_.erase(id);
        }
        if (stats.size() != 0) {
            stat_saver_->Save(stats);
        }
    }

}  // namespace model

namespace app {

    Token TokensGen::GetToken() {
        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << generator1_() << std::setw(16) << generator2_();
        return Token(ss.str());
    }

    Player::Player(Token&& token, model::GameSession* session, model::Dog* dog)
        : id_(GetNextId()), session_(session), dog_(dog), token_(std::move(token)) {
    }

    Player::Player(size_t id, Token&& token, model::GameSession* session, model::Dog* dog) 
        : id_(id), session_(session), dog_(dog), token_(std::move(token)) {
            ++start_id_;
    }

    Player& Players::AddPlayer(model::Dog&& dog, model::GameSession* session) {
        auto* doggy = session->AddDog(std::move(dog));
        auto token = token_gen_.GetToken();
        while (tokens_to_players_.contains(token)) {
            token = token_gen_.GetToken();
        }
        auto token_copy = token;
        Player player(std::move(token_copy), session, doggy);
        tokens_to_players_.emplace(token, std::move(player));
        dogs_id_to_players_[doggy->GetId()] = &tokens_to_players_.at(token);
        return tokens_to_players_.at(token);
    }

    void Players::AddPlayer(size_t id, Token&& token, model::Dog&& dog, model::GameSession* session) {
        auto* doggy = session->AddDog(std::move(dog));
        auto token_copy = token;
        Player player(id, std::move(token_copy), session, doggy);
        tokens_to_players_.emplace(std::move(token), std::move(player));
        dogs_id_to_players_[doggy->GetId()] = &tokens_to_players_.at(token);
    }

    Player* Players::FindByToken(const Token& token) {
        auto player = tokens_to_players_.find(token);
        if (auto player = tokens_to_players_.find(token); player != tokens_to_players_.end()) {
            return &player->second;
        }
        return nullptr;
    }

    size_t Player::start_id_ = 0;

    Application::Application(model::Game&& game, bool randomize_spawn, std::shared_ptr<model::StatSaver> stat_saver)
        :game_(std::move(game))
    {
        game_.StartSessions(randomize_spawn, stat_saver);
    }

    void Application::AddPlayer(size_t player_id, Token&& token, model::Dog&& dog, const model::Map::Id& map_id) {
        auto* session = FindSession(map_id);
        players_.AddPlayer(player_id, std::move(token), std::move(dog), session);
    }

}  // namespace app