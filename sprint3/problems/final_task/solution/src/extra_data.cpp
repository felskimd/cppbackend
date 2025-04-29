#include "extra_data.h"

namespace extra {

std::unordered_map<std::string, json::array> ExtraDataGiver::map_id_to_loot_{};

void ExtraDataGiver::SetLoot(std::string id, json::array loot) {
	map_id_to_loot_[id] = loot;
}

json::array& ExtraDataGiver::GetLoot(const std::string& id) {
	if (auto found = map_id_to_loot_.find(id); found != map_id_to_loot_.end()) {
		return found->second;
	}
	throw std::invalid_argument("Map id not found");
}

}