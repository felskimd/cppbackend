#pragma once
#include <compare>
#include <deque>
#include <forward_list>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "collision_detector.h"
#include "tagged.h"
#include "loot_generator.h"

namespace model {

    using namespace std::literals;
    using Dimension = int;
    using Coord = Dimension;

    struct ModelLiterals {
        ModelLiterals() = delete;
        constexpr static std::string_view DEFAULT_DOG_SPEED = "defaultDogSpeed"sv;
        constexpr static std::string_view DEFAULT_BAG_CAPACITY = "defaultBagCapacity"sv;
        constexpr static std::string_view DEFAULT_DOG_RETIREMENT_TIME = "dogRetirementTime"sv;
        constexpr static std::string_view DOG_SPEED = "dogSpeed"sv;
        constexpr static std::string_view BAG_CAPACITY = "bagCapacity"sv;
        constexpr static std::string_view MAPS = "maps"sv;
        constexpr static std::string_view ID = "id"sv;
        constexpr static std::string_view NAME = "name"sv;
        constexpr static std::string_view START_X = "x0"sv;
        constexpr static std::string_view START_Y = "y0"sv;
        constexpr static std::string_view END_X = "x1"sv;
        constexpr static std::string_view END_Y = "y1"sv;
        constexpr static std::string_view POSITION_X = "x"sv;
        constexpr static std::string_view POSITION_Y = "y"sv;
        constexpr static std::string_view OFFSET_X = "offsetX"sv;
        constexpr static std::string_view OFFSET_Y = "offsetY"sv;
        constexpr static std::string_view MODEL_SIZE_WIDTH = "w"sv;
        constexpr static std::string_view MODEL_SIZE_HEIGHT = "h"sv;
        constexpr static std::string_view ROADS = "roads"sv;
        constexpr static std::string_view OFFICES = "offices"sv;
        constexpr static std::string_view BUILDINGS = "buildings"sv;
    };

    struct CollisionWidths {
        constexpr static double DOG_WIDTH = 0.6;
        constexpr static double OFFICE_WIDTH = 0.5;
        constexpr static double ITEM_WIDTH = 0.;
    };

    constexpr static unsigned MILLISECONDS_IN_SECOND = 1000;

    struct Position {
        double x, y;

        auto operator<=>(const Position&) const = default;
    };

    struct Speed {
        double vx, vy;

        auto operator<=>(const Speed&) const = default;
    };

    enum Direction {
        NORTH,
        SOUTH,
        WEST,
        EAST
    };

    struct Point {
        Coord x, y;

        bool operator==(const Point& other) const = default;
    };

    struct PointHash {
        size_t operator()(const Point& p) const {
            return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
        }
    };

    struct Size {
        Dimension width, height;
    };

    struct Rectangle {
        Point position;
        Size size;
    };

    struct Offset {
        Dimension dx, dy;
    };

    class Road {
        struct HorizontalTag {
            HorizontalTag() = default;
        };

        struct VerticalTag {
            VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{ start }
            , end_{ end_x, start.y } {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{ start }
            , end_{ start.x, end_y } {
        }

        bool IsHorizontal() const noexcept {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept {
            return start_;
        }

        Point GetEnd() const noexcept {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building {
    public:
        explicit Building(Rectangle bounds) noexcept
            : bounds_{ bounds } {
        }

        const Rectangle& GetBounds() const noexcept {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{ std::move(id) }
            , position_{ position }
            , offset_{ offset } {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        Point GetPosition() const noexcept {
            return position_;
        }

        Offset GetOffset() const noexcept {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name, unsigned pockets_size) noexcept
            : id_(std::move(id))
            , name_(std::move(name))
            , pockets_size_(pockets_size) {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        const std::string& GetName() const noexcept {
            return name_;
        }

        const Buildings& GetBuildings() const noexcept {
            return buildings_;
        }

        const Roads& GetRoads() const noexcept {
            return roads_;
        }

        const Offices& GetOffices() const noexcept {
            return offices_;
        }

        void SetSpeed(double speed) {
            speed_ = speed;
        }

        double GetSpeed() {
            return speed_;
        }

        void AddRoad(const Road& road) {
            roads_.emplace_back(road);
        }

        void AddBuilding(const Building& building) {
            buildings_.emplace_back(building);
        }

        void AddOffice(Office office);

        void SetLootTypesCount(unsigned count) {
            loot_types_count_ = count;
        }

        unsigned GetLootTypesCount() {
            return loot_types_count_;
        }

        unsigned GetPocketsSize() {
            return pockets_size_;
        }

        void SetLootValues(std::unordered_map<size_t, size_t>&& loot_scores) {
            loot_values_ = std::move(loot_scores);
        }

        size_t GetValue(size_t loot_type) const {
            return loot_values_.at(loot_type);
        }

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;
        double speed_ = 0.;
        unsigned loot_types_count_ = 0;
        unsigned pockets_size_;
        std::unordered_map<size_t, size_t> loot_values_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
    };

    struct Item {
        size_t id;
        size_t type;

        auto operator<=>(const Item& other) const = default;
    };

    struct LostItem {
        size_t id;
        size_t type;
        Position pos;

        auto operator<=>(const LostItem& other) const = default;
    };

    class Pockets {
    public:
        Pockets(size_t capacity)
            :capacity_(capacity)
        { }

        void AddItem(Item item) {
            if (!CanStoreOne()) {
                throw std::logic_error("No space");
            }
            items_.push_back(item);
        }

        bool CanStoreOne() const {
            return items_.size() < capacity_;
        }

        void Clear() {
            items_.clear();
        }

        const std::deque<Item>& GetItems() const {
            return items_;
        }

        size_t GetCapacity() const {
            return capacity_;
        }

    private:
        std::deque<Item> items_;
        size_t capacity_;
    };

    class AFKObserver {
    public:
        virtual void OnAFK(size_t) = 0;
        virtual void OnNotAFK(size_t) = 0;
    };

    class AFKSubject {
    public:
        void SetObserver(AFKObserver* obs) { 
            obs_ = obs;
        }

    protected:
        AFKObserver* obs_ = nullptr;
    };

    class Dog : public AFKSubject {
    public:
        explicit Dog(std::string&& name, size_t pockets_capacity) 
            :id_(GetNextId()), name_(std::move(name)), pockets_(pockets_capacity)
        { }

        explicit Dog(size_t id, std::string&& name, size_t pockets_capacity)
            :id_(id), name_(std::move(name)), pockets_(pockets_capacity)
        {
            ++start_id_;
        }

        bool operator==(const Dog& right) const {
            return this->id_ == right.id_;
        }

        bool operator==(const Dog* right) const {
            return this->id_ == right->id_;
        }

        int GetId() const {
            return id_;
        }

        const std::string& GetName() const {
            return name_;
        }

        Position GetPosition() const {
            return position_;
        }

        Direction GetDirection() const {
            return direction_;
        }

        Speed GetSpeed() const {
            return speed_;
        }

        void SetPosition(Position pos) {
            position_ = pos;
        }

        void Move(Direction dir, double speed) {
            direction_ = dir;
            NotifyNotAFK();
            switch (dir) {
            case Direction::NORTH:
                speed_ = { 0., -speed };
                return;
            case Direction::SOUTH:
                speed_ = { 0., speed };
                return;
            case Direction::WEST:
                speed_ = { -speed, 0. };
                return;
            case Direction::EAST:
                speed_ = { speed, 0. };
                return;
            }
        }

        void Stop() {
            speed_ = {};
            NotifyAFK();
        }

        void ResetDirection() {
            direction_ = Direction::NORTH;
        }

        bool CanTakeLoot() const {
            return pockets_.CanStoreOne();
        }

        void TakeLoot(Item item) {
            pockets_.AddItem(item);
        }

        std::deque<Item> TransferItems() {
            auto items = std::deque<Item>(pockets_.GetItems());
            pockets_.Clear();
            return items;
        }

        const std::deque<Item>& GetItems() const {
            return pockets_.GetItems();
        }

        void AddScore(size_t score) {
            score_ += score;
        }

        size_t GetScore() const {
            return score_;
        }

        size_t GetPocketsCapacity() const {
            return pockets_.GetCapacity();
        }

    private:
        const size_t id_;
        std::string name_;
        Position position_ = { 0., 0. };
        Speed speed_ = { 0., 0. };
        Direction direction_ = Direction::NORTH;
        Pockets pockets_;
        size_t score_ = 0;

        static size_t start_id_;

        static size_t GetNextId() {
            return start_id_++;
        }

        void NotifyAFK() {
            if (obs_) {
                obs_->OnAFK(id_);
            }
        }

        void NotifyNotAFK() {
            if (obs_) {
                obs_->OnNotAFK(id_);
            }
        }
    };

    class CollisionActorsProvider : public collision_detector::ItemGathererProvider {
    public:
        size_t ItemsCount() const override {
            return items_.size();
        }

        collision_detector::Item GetItem(size_t idx) const override {
            return items_.at(idx);
        }

        size_t GatherersCount() const override {
            return gatherers_.size();
        }

        collision_detector::Gatherer GetGatherer(size_t idx) const override {
            return gatherers_.at(idx);
        }

        void SetGatherers(std::vector<collision_detector::Gatherer>&& gatherers) {
            gatherers_ = std::move(gatherers);
        }

        void SetItems(std::vector<collision_detector::Item>&& items) {
            items_ = std::move(items);
        }

    private:
        std::vector<collision_detector::Gatherer> gatherers_;
        std::vector<collision_detector::Item> items_;

    };

    struct SaveStat {
        std::string name;
        int scores;
        unsigned playtime;
    };

    class StatSaver {
    public:
        virtual void Save(const std::vector<SaveStat>& stat) = 0;
    };

    class GameSession : public AFKObserver {
    public:
        explicit GameSession(Map* map, bool randomize_spawn, unsigned dog_retirement_time, std::shared_ptr<StatSaver> stat_saver);

        using LootType = unsigned;
        using LootId = unsigned;

        enum ItemType {
            OFFICE,
            LOOT
        };

        struct ItemData {
            ItemType type;
            size_t outer_id;
        };

        void OnAFK(size_t id) override {
            afk_dogs_[id] = 0;
        }

        void OnNotAFK(size_t id) override {
            if (afk_dogs_.contains(id)) {
                afk_dogs_.erase(id);
            }
        }

        std::vector<const Dog*> GetDogs() const;

        Dog* AddDog(Dog&& dog);

        double GetSpeed() const {
            return map_->GetSpeed();
        }

        std::unordered_set<size_t> Tick(unsigned delta, unsigned loot_count) {
            auto [new_positions, dogs_to_stop] = CalculatePositions(delta);
            CollisionActorsProvider provider;
            provider.SetGatherers(PrepareDogs(new_positions));
            auto [items, data] = PrepareCollisionItems();
            provider.SetItems(std::move(items));
            auto events = collision_detector::FindGatherEvents(provider);
            ProcessEvents(events, data);
            UpdateDogsPositions(new_positions, dogs_to_stop);
            SpawnLoot(loot_count);
            auto dogs_to_retire = ObserveAFKAndPlaytime(delta);
            RetireDogs(dogs_to_retire);
            return dogs_to_retire;
        }

        const std::unordered_map<LootId, std::pair<LootType, Position>>& GetLootMap() const {
            return loot_map_;
        }

        unsigned GetLootCount() const {
            return loot_count_;
        }

        unsigned GetDogsCount() const {
            return dogs_links_.size();
        }

        unsigned GetPocketsSize() {
            return map_->GetPocketsSize();
        }

        void AddLoot(LootId id, LootType type, Position pos) {
            loot_map_.insert({id, {type, pos}});
            ++loot_count_;
            ++loot_id_;
        }

        Map::Id GetMapId() const {
            return map_->GetId();
        }

    private:
        //std::unordered_map<size_t, Dog> dogs_;
        std::forward_list<Dog> dogs_;
        std::vector<Dog*> dogs_links_;
        Map* map_;
        std::unordered_map<Point, std::vector<const Road*>, PointHash> roads_graph_;
        bool randomize_spawn_;
        std::unordered_map<LootId, std::pair<LootType, Position>> loot_map_;
        std::unordered_map<size_t, unsigned> afk_dogs_;
        std::unordered_map<size_t, unsigned> dogs_playtime_;
        unsigned loot_count_ = 0;
        unsigned loot_id_ = 0;
        unsigned dog_retirement_time_;
        std::shared_ptr<model::StatSaver> stat_saver_;

        std::pair<bool, Position> CalculateMove(Position pos, Speed speed, unsigned delta) const;

        std::pair<std::unordered_map<size_t, Position>, std::unordered_map<size_t, bool>> CalculatePositions(unsigned delta) const;

        void UpdateDogsPositions(const std::unordered_map<size_t, Position>& positions, const std::unordered_map<size_t, bool>& dogs_to_stop);

        void SpawnLoot(unsigned loot_count);

        unsigned GetNextLootId();

        std::vector<collision_detector::Gatherer> PrepareDogs(const std::unordered_map<size_t, Position>& calculated_positions) const;

        std::pair<std::vector<collision_detector::Item>, std::unordered_map<size_t, ItemData>> PrepareCollisionItems() const;

        void ProcessEvents(const std::vector<collision_detector::GatheringEvent>& events, const std::unordered_map<size_t, ItemData>& items_data);

        //Returns dog ids to retire
        std::unordered_set<size_t> ObserveAFKAndPlaytime(unsigned delta);

        void RetireDogs(const std::unordered_set<size_t>& ids);
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        explicit Game(loot_gen::LootGenerator&& loot_gen, unsigned dog_retirement_time)
            : loot_generator_(std::move(loot_gen)) 
            , dog_retirement_time_(dog_retirement_time)
        { }

        void AddMap(Map map);

        const Maps& GetMaps() const noexcept {
            return maps_;
        }

        const Map* FindMap(const Map::Id& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        GameSession* FindSession(const Map::Id& id) {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &sessions_.at(it->second);
            }
            return nullptr;
        }

        void StartSessions(bool randomize_spawn, std::shared_ptr<model::StatSaver> stat_saver) {
            sessions_.clear();
            sessions_.reserve(maps_.size());
            for (auto& map : maps_) {
                sessions_.push_back(GameSession{ &map, randomize_spawn, dog_retirement_time_, stat_saver });
            }
        }

        std::vector<size_t> Tick(unsigned delta) {
            loot_gen::LootGenerator::TimeInterval ms_duration(delta);
            std::vector<size_t> retired_dogs;
            for (auto& session : sessions_) {
                auto dogs_to_retire = session.Tick(delta, loot_generator_.Generate(ms_duration, session.GetLootCount(), session.GetDogsCount()));
                for (const auto dog_id : dogs_to_retire) {
                    retired_dogs.push_back(dog_id);
                }
            }
            return retired_dogs;
        }

        void AddLoot(const Map::Id& map_id, GameSession::LootId loot_id, GameSession::LootType type, Position pos) {
            FindSession(map_id)->AddLoot(loot_id, type, pos);
        }

        std::unordered_map<std::string, std::vector<LostItem>> GetLostItems() const {
            std::unordered_map<std::string, std::vector<LostItem>> result;
            for (const auto& session : sessions_) {
                std::string map_id = *session.GetMapId();
                for (const auto& loot : session.GetLootMap()) {
                    result[map_id].emplace_back(loot.first, loot.second.first, loot.second.second);
                }
            }
            return result;
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        std::vector<GameSession> sessions_;
        loot_gen::LootGenerator loot_generator_;
        unsigned dog_retirement_time_;
    };

}  // namespace model

namespace detail {
    struct TokenTag {};
}  // namespace detail

namespace app {

    using Token = util::Tagged<std::string, detail::TokenTag>;

    class TokensGen {
    public:
        Token GetToken();

    private:
        std::random_device random_device_;
        std::mt19937_64 generator1_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        std::mt19937_64 generator2_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        // Чтобы сгенерировать токен, получите из generator1_ и generator2_
        // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
        // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
        // чтобы сделать их подбор ещё более затруднительным
    };

    class Player {
    public:
        explicit Player(Token&& token, model::GameSession* session, model::Dog* dog);
        explicit Player(size_t id, Token&& token, model::GameSession* session, model::Dog* dog);

        bool operator==(const Player& other) {
            return this->id_ == other.id_;
        }

        bool operator==(const Player* other) {
            return this->id_ == other->id_;
        }

        Token GetToken() const {
            return token_;
        }

        size_t GetId() const noexcept {
            return id_;
        }

        const model::GameSession* GetSession() const noexcept {
            return session_;
        }

        model::Dog* GetDog() noexcept {
            return dog_;
        }

        const model::Dog& GetDog() const noexcept {
            return *dog_;
        }

    private:
        size_t id_;
        model::GameSession* session_;
        model::Dog* dog_;
        Token token_;
        static size_t start_id_;

        static int GetNextId() {
            return start_id_++;
        }
    };

    class Players {
    public:
        Player& AddPlayer(model::Dog&& dog, model::GameSession* session);

        void AddPlayer(size_t id, Token&& token, model::Dog&& dog, model::GameSession* session);

        Player* FindByToken(const Token& token);

        const std::vector<Player> GetPlayers() const {
            std::vector<Player> result;
            for (const auto& [token, player] : tokens_to_players_) {
                result.push_back(player);
            }
            return result;
        }

        void RemovePlayers(const std::vector<size_t>& dogs_ids) {
            for (const auto id : dogs_ids) {
                tokens_to_players_.erase(dogs_id_to_players_.at(id)->GetToken());
                //std::erase(players_, dogs_id_to_players_.at(id));
                dogs_id_to_players_.erase(id);
            }
        }

    private:
        using TokenHasher = util::TaggedHasher<Token>;
        using TokensToPlayers = std::unordered_map<Token, Player, TokenHasher>;

        //std::forward_list<Player> players_;
        TokensToPlayers tokens_to_players_;
        TokensGen token_gen_;
        std::unordered_map<size_t, Player*> dogs_id_to_players_;
    };

    class ApplicationListener {
    public:
        virtual void OnTick(unsigned delta) = 0;
    };

    class Application {
    public:
        explicit Application(model::Game&& game, bool randomize_spawn, std::shared_ptr<model::StatSaver> stat_saver);

        const model::Map* FindMap(const model::Map::Id& id) const {
            return game_.FindMap(id);
        }

        const model::Game::Maps& GetMaps() const {
            return game_.GetMaps();
        }

        model::GameSession* FindSession(const model::Map::Id& id) {
            return game_.FindSession(id);
        }

        Player& AddPlayer(model::Dog&& dog, model::GameSession* session) {
            return players_.AddPlayer(std::move(dog), session);
        }

        Player* FindByToken(const Token& token) {
            return players_.FindByToken(token);
        }

        std::vector<const model::Dog*> GetDogs(const Player* player) const {
            return player->GetSession()->GetDogs();
        }

        void Move(Player* player, model::Direction dir) {
            player->GetDog()->Move(dir, player->GetSession()->GetSpeed());
        }

        void Stop(Player* player) {
            player->GetDog()->Stop();
        }

        void Tick(unsigned millisec) {
            try {
            auto dog_ids_to_retire = game_.Tick(millisec);
            players_.RemovePlayers(dog_ids_to_retire);
            if (listener_) {
                listener_->OnTick(millisec);
            }
            }
            catch (std::exception& ex) {
                std::cerr << ex.what() << std::endl << std::endl << std::endl << std::endl << std::endl << std::endl;
            }
        }

        void SetAppListener(std::shared_ptr<ApplicationListener> listener) {
            listener_ = listener;
        }

        void AddPlayer(size_t player_id, Token&& token, model::Dog&& dog, const model::Map::Id& map_id);

        void AddLoot(const model::Map::Id& map_id, model::GameSession::LootId loot_id, model::GameSession::LootType type, model::Position pos) {
            game_.AddLoot(map_id, loot_id, type, pos);
        }
        
        std::unordered_map<std::string, std::vector<model::LostItem>> GetLostItems() const {
            return game_.GetLostItems();
        }

        const std::vector<Player> GetPlayers() const {
            return players_.GetPlayers();
        }

    private:
        model::Game game_;
        Players players_;
        std::shared_ptr<ApplicationListener> listener_;
    };

}  // namespace app