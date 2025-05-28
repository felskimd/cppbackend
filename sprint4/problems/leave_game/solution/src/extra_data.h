#pragma once

#include <boost/json.hpp>
#include <unordered_map>

namespace extra {

namespace json = boost::json;

class ExtraDataGiver {
public:
	ExtraDataGiver() = delete;

	static void SetLoot(std::string id, json::array loot);

	static json::array& GetLoot(const std::string& id);
private:
	static std::unordered_map<std::string, json::array> map_id_to_loot_;
};

}