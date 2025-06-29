#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "model.h"

namespace json_loader {

struct LootGeneratorLiterals {
	LootGeneratorLiterals() = delete;
	constexpr static std::string_view CONFIG = "lootGeneratorConfig";
	constexpr static std::string_view PERIOD = "period";
	constexpr static std::string_view PROBABILITY = "probability";
};

struct LootLiterals {
	LootLiterals() = delete;
	constexpr static std::string_view LOOT_TYPES = "lootTypes";
};

constexpr int MILLISEC_IN_SEC = 1000;
constexpr int DEFAULT_DOG_RETIREMENT_TIME = 60000;
constexpr int DEFAULT_BAG_CAPACITY = 3;
constexpr double DEFAULT_DOG_SPEED = 1.;

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path);
std::string ReadFileToString(const std::filesystem::path& filePath);
model::Building JsonToBuilding(const json::object& obj);
model::Office JsonToOffice(const json::object& obj);
model::Road JsonToRoad(const json::object& obj);

}  // namespace json_loader
