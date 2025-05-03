#include <boost/serialization/deque.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <filesystem>
#include <fstream>

#include "model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, Position& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Speed& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.vx;
    ar& vec.vy;
}

template <typename Archive>
void serialize(Archive& ar, Item& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.id);
    ar&(obj.type);
}

template <typename Archive>
void serialize(Archive& ar, LostItem& obj, [[maybe_unused]] const unsigned version) {
    ar& (obj.id);
    ar& (obj.type);
    ar& (obj.pos);
}

}  // namespace model

namespace serialization {

class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , bag_capacity_(dog.GetPocketsCapacity())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirection())
        , score_(dog.GetScore())
        , bag_content_(dog.GetItems()) {
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{ id_, std::move(std::string(name_)), bag_capacity_ };
        dog.SetPosition(pos_);
        dog.Move(direction_, speed_.vx == 0. ? speed_.vy : speed_.vx);
        dog.AddScore(score_);
        for (const auto& item : bag_content_) {
            if (!dog.CanTakeLoot()) {
                throw std::runtime_error("Failed to put bag content");
            }
            dog.TakeLoot(item);
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    size_t id_ = 0;
    std::string name_;
    model::Position pos_ = {0, 0};
    size_t bag_capacity_ = 0;
    model::Speed speed_;
    model::Direction direction_ = model::Direction::NORTH;
    size_t score_ = 0;
    std::deque<model::Item> bag_content_;
};

class PlayerRepr {
public:
    struct PlayerData {
        size_t id;
        app::Token token;
        model::Map::Id map_id;
        model::Dog dog;
    };

    PlayerRepr() = default;

    explicit PlayerRepr(const app::Player& player) 
        : id_(player.GetId())
        , token_(player.GetToken())
        , map_id_(player.GetSession()->GetMapId())
        , dog_(DogRepr(player.GetDog())) {
    }

    [[nodiscard]] PlayerData Restore() const {
        return {id_, token_, map_id_, dog_.Restore()};
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar&* token_;
        ar&* map_id_;
        ar& dog_;
    }

private:
    size_t id_ = 0;
    app::Token token_ = app::Token{ 0u };
    model::Map::Id map_id_ = model::Map::Id{ 0u };
    DogRepr dog_;
};

class LootRepr {
public:
    LootRepr() = default;

    explicit LootRepr(const std::unordered_map<std::string, std::vector<model::LostItem>>& loot_map)
        : loot_map_(loot_map) {
    }

    [[nodiscard]] const std::unordered_map<std::string, std::vector<model::LostItem>>& Restore() const {
        return loot_map_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& loot_map_;
    }

private:
    std::unordered_map<std::string, std::vector<model::LostItem>> loot_map_;
};

void Serialize(const std::string& filename, const app::Application& app) {
    auto temp_name = filename + ".temp";
    std::ofstream file{ temp_name };
    boost::archive::text_oarchive archive{file};
    std::vector<PlayerRepr> players;
    auto loot = LootRepr{app.GetLostItems()};
    for (const auto& player : app.GetPlayers()) {
        players.emplace_back(player);
    }
    archive << players << loot;
    file.close();
    std::filesystem::rename(temp_name, filename);
}

void Deserialize(const std::string& filename, app::Application& app) {
    std::ifstream file{ filename };
    boost::archive::text_iarchive archive{ file };
    std::vector<PlayerRepr> players;
    LootRepr loot;
    archive >> players >> loot;
    for (auto& player : players) {
        auto restored = player.Restore();
        app.AddPlayer(restored.id, std::move(restored.token), std::move(restored.dog), restored.map_id);
    }
    for (const auto& [map, items] : loot.Restore()) {
        model::Map::Id map_id{ map };
        for (const auto& item : items) {
            app.AddLoot(map_id, item.id, item.type, item.pos);
        }
    }
    file.close();
}

class SerializingListener : public app::ApplicationListener {
public:
    SerializingListener(unsigned save_period, const std::string& filename, const app::Application& app)
        : save_period_(save_period)
        , filename_(filename)
        , app_(app) {
    }

    void OnTick(unsigned delta) override {
        time_since_save_ += delta;
        if (time_since_save_ >= save_period_) {
            Serialize(filename_, app_);
            time_since_save_ = 0;
        }
    }

private:
    unsigned time_since_save_ = 0;
    unsigned save_period_;
    const std::string& filename_;
    const app::Application& app_;
};

}  // namespace serialization
