#pragma once
#include <compare>
#include <deque>
#include <forward_list>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
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
    };

    class Pockets {
    public:
        Pockets(unsigned capacity)
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

    private:
        std::deque<Item> items_;
        unsigned capacity_;
    };

    class Dog {
    public:
        explicit Dog(std::string&& name, unsigned pockets_capacity) 
            :id_(GetNextId()), name_(std::move(name)), pockets_(pockets_capacity)
        { }

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

    private:
        const int id_;
        std::string name_;
        Position position_ = { 0., 0. };
        Speed speed_ = { 0., 0. };
        Direction direction_ = Direction::NORTH;
        Pockets pockets_;
        size_t score_ = 0;

        static int start_id_;

        static int GetNextId() {
            return start_id_++;
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

    class GameSession {
    public:
        explicit GameSession(Map* map, bool randomize_spawn);

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

        std::vector<const Dog*> GetDogs() const;

        Dog* AddDog(Dog&& dog);

        double GetSpeed() const {
            return map_->GetSpeed();
        }

        void Tick(unsigned delta, unsigned loot_count) {
            auto [new_positions, dogs_to_stop] = CalculatePositions(delta);
            CollisionActorsProvider provider;
            provider.SetGatherers(PrepareDogs(new_positions));
            auto [items, data] = PrepareCollisionItems();
            provider.SetItems(std::move(items));
            auto events = collision_detector::FindGatherEvents(provider);
            ProcessEvents(events, data);
            UpdateDogsPositions(new_positions, dogs_to_stop);
            SpawnLoot(loot_count);
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

    private:
        std::forward_list<Dog> dogs_;
        std::vector<Dog*> dogs_links_;
        Map* map_;
        std::unordered_map<Point, std::vector<const Road*>, PointHash> roads_graph_;
        bool randomize_spawn_;
        std::unordered_map<LootId, std::pair<LootType, Position>> loot_map_;
        unsigned loot_count_ = 0;
        unsigned loot_id_ = 0;

        std::pair<bool, Position> CalculateMove(Position pos, Speed speed, unsigned delta) const;

        std::pair<std::unordered_map<size_t, Position>, std::unordered_map<size_t, bool>> CalculatePositions(unsigned delta) const;

        void UpdateDogsPositions(const std::unordered_map<size_t, Position>& positions, const std::unordered_map<size_t, bool>& dogs_to_stop);

        void SpawnLoot(unsigned loot_count);

        unsigned GetNextLootId();

        std::vector<collision_detector::Gatherer> PrepareDogs(const std::unordered_map<size_t, Position>& calculated_positions) const;

        std::pair<std::vector<collision_detector::Item>, std::unordered_map<size_t, ItemData>> PrepareCollisionItems() const;

        void ProcessEvents(const std::vector<collision_detector::GatheringEvent>& events, const std::unordered_map<size_t, ItemData>& items_data);
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        explicit Game(loot_gen::LootGenerator&& loot_gen)
            : loot_generator_(std::move(loot_gen)) { }

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

        void StartSessions(bool randomize_spawn) {
            sessions_.clear();
            sessions_.reserve(maps_.size());
            for (auto& map : maps_) {
                sessions_.push_back(GameSession{ &map, randomize_spawn });
            }
        }

        void Tick(unsigned delta) {
            loot_gen::LootGenerator::TimeInterval ms_duration(delta);
            for (auto& session : sessions_) {
                session.Tick(delta, loot_generator_.Generate(ms_duration, session.GetLootCount(), session.GetDogsCount()));
            }
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        std::vector<GameSession> sessions_;
        loot_gen::LootGenerator loot_generator_;
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
        explicit Player(Token token, model::GameSession* session, model::Dog* dog);

        Token GetToken() const {
            return token_;
        }

        int GetId() const noexcept {
            return id_;
        }

        const model::GameSession* GetSession() const noexcept {
            return session_;
        }

        model::Dog* GetDog() noexcept {
            return dog_;
        }

    private:
        int id_;
        model::GameSession* session_;
        model::Dog* dog_;
        Token token_;
        static int start_id_;

        static int GetNextId() {
            return start_id_++;
        }
    };

    class Players {
    public:
        Player& AddPlayer(model::Dog&& dog, model::GameSession* session);

        Player* FindByToken(const Token& token);

    private:
        using TokenHasher = util::TaggedHasher<Token>;
        using TokensToPlayers = std::unordered_map<Token, size_t, TokenHasher>;

        std::vector<Player> players_;
        TokensToPlayers tokens_to_players_;
        TokensGen token_gen_;
    };

    class Application {
    public:
        explicit Application(model::Game&& game, bool randomize_spawn);

        const model::Map* FindMap(model::Map::Id id) const {
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
            game_.Tick(millisec);
        }

    private:
        model::Game game_;
        Players players_;
    };

}  // namespace app
