#include "extra_data.h"
#include "json_loader.h"
#include "loot_generator.h"

#include <fstream>

namespace json_loader {

loot_gen::LootGenerator CreateLootGenerator(json::object& json_config) {
    double period, probability;
    if (auto config = json_config.find(std::string(LootGeneratorLiterals::CONFIG)); config != json_config.end()) {
        auto& config_object = config->value().as_object();
        if (auto json_period = config_object.find(std::string(LootGeneratorLiterals::PERIOD)); json_period != config_object.end()) {
            period = json_period->value().as_double();
        }
        else {
            throw std::runtime_error("Incorrect loot generator options");
        }
        if (auto json_probability = config_object.find(std::string(LootGeneratorLiterals::PROBABILITY)); json_probability != config_object.end()) {
            probability = json_probability->value().as_double();
        }
        else {
            throw std::runtime_error("Incorrect loot generator options");
        }
    }
    else {
        throw std::runtime_error("Incorrect loot generator options");
    }
    return loot_gen::LootGenerator(std::chrono::milliseconds(static_cast<int>(period * 1000)), probability);
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    auto json_config = json::parse(ReadFileToString(json_path));
    auto obj = json_config.as_object();
    double default_speed;
    model::Game game{std::move(CreateLootGenerator(obj))};
    if (auto speed = obj.find(std::string(model::ModelLiterals::DEFAULT_DOG_SPEED)); speed != obj.end()) {
        default_speed = speed->value().as_double();
    }
    else {
        default_speed = 1.;
    }
    for (const auto& parsed_map : obj.at(std::string(model::ModelLiterals::MAPS)).as_array()) {
        json::object object_map = parsed_map.as_object();
        auto parsed_id = object_map.at(std::string(model::ModelLiterals::ID)).as_string().c_str();
        auto loot_array = object_map.at(std::string(LootLiterals::LOOT_TYPES)).as_array();
        if (loot_array.size() == 0) {
            throw std::invalid_argument("Incorrect config file");
        }
        extra::ExtraDataGiver::SetLoot(std::string(parsed_id), loot_array);
        model::Map model_map(model::Map::Id(parsed_id), object_map.at(std::string(model::ModelLiterals::NAME)).as_string().c_str());
        model_map.SetLootTypesCount(loot_array.size());
        if (auto speed = object_map.find(std::string(model::ModelLiterals::DOG_SPEED)); speed != obj.end()) {
            if (speed->value().is_double()) {
                model_map.SetSpeed(speed->value().get_double());
            }
            else {
                model_map.SetSpeed(default_speed);
            }
        }
        else {
            model_map.SetSpeed(default_speed);
        }
        for (const auto& building : object_map.at(std::string(model::ModelLiterals::BUILDINGS)).as_array()) {
            model_map.AddBuilding(JsonToBuilding(building.as_object()));
        }
        for (const auto& road : object_map.at(std::string(model::ModelLiterals::ROADS)).as_array()) {
            model_map.AddRoad(JsonToRoad(road.as_object()));
        }
        for (const auto& office : object_map.at(std::string(model::ModelLiterals::OFFICES)).as_array()) {
            model_map.AddOffice(JsonToOffice(office.as_object()));
        }
        game.AddMap(model_map);
    }
    return game;
}

std::string ReadFileToString(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл: " + filePath.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

model::Building JsonToBuilding(const json::object& obj) {
    model::Rectangle rect{ { obj.at(std::string(model::ModelLiterals::POSITION_X)).to_number<int>()
                , obj.at(std::string(model::ModelLiterals::POSITION_Y)).to_number<int>() }
        , { obj.at(std::string(model::ModelLiterals::MODEL_SIZE_WIDTH)).to_number<int>()
                , obj.at(std::string(model::ModelLiterals::MODEL_SIZE_HEIGHT)).to_number<int>() } };
    return model::Building(rect);
}

model::Office JsonToOffice(const json::object& obj) {
    
    return model::Office(model::Office::Id(obj.at(std::string(model::ModelLiterals::ID)).as_string().c_str())
        , { obj.at(std::string(model::ModelLiterals::POSITION_X)).to_number<int>()
                , obj.at(std::string(model::ModelLiterals::POSITION_Y)).to_number<int>() }
        , {obj.at(std::string(model::ModelLiterals::OFFSET_X)).to_number<int>()
                , obj.at(std::string(model::ModelLiterals::OFFSET_Y)).to_number<int>() });
}

model::Road JsonToRoad(const json::object& obj) {
    if (obj.contains(std::string(model::ModelLiterals::END_X))) {
        return model::Road(model::Road::HORIZONTAL
            , { obj.at(std::string(model::ModelLiterals::START_X)).to_number<int>()
                    , obj.at(std::string(model::ModelLiterals::START_Y)).to_number<int>() }
            , obj.at(std::string(model::ModelLiterals::END_X)).to_number<int>());
    }
    return model::Road(model::Road::VERTICAL
        , { obj.at(std::string(model::ModelLiterals::START_X)).to_number<int>()
                , obj.at(std::string(model::ModelLiterals::START_Y)).to_number<int>() }
        , obj.at(std::string(model::ModelLiterals::END_Y)).to_number<int>());
}

}  // namespace json_loader
