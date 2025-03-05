#include "request_handler.h"

#include <ranges>

namespace http_handler {

std::vector<std::string_view> SplitRequest(std::string_view body) {
    std::vector<std::string_view> result;
    size_t start = 0;
    size_t end = body.find("/");
    while (end != std::string_view::npos) {
        result.push_back(body.substr(start, end - start));
        start = end + 1;
        end = body.find("/", start);
    }
    result.push_back(body.substr(start, body.length() - start));
    return result;
}

json::object MapToJSON(const model::Map* map) {
    json::object obj;
    obj[std::string(ModelLiterals::ID)] = *map->GetId();
    obj[std::string(ModelLiterals::NAME)] = map->GetName();
    obj[std::string(ModelLiterals::ROADS)] = RoadsToJson(map);
    obj[std::string(ModelLiterals::BUILDINGS)] = BuildingsToJson(map);
    obj[std::string(ModelLiterals::OFFICES)] = OfficesToJson(map);
    return obj;
}

json::array RoadsToJson(const model::Map* map) {
    json::array roads;
    for (const auto& road : map->GetRoads()) {
        json::object json_road;
        json_road[std::string(ModelLiterals::START_X)] = road.GetStart().x;
        json_road[std::string(ModelLiterals::START_Y)] = road.GetStart().y;
        road.IsHorizontal() ? json_road[std::string(ModelLiterals::END_X)] = road.GetEnd().x : json_road[std::string(ModelLiterals::END_Y)] = road.GetEnd().y;
        roads.emplace_back(json_road);
    }
    return roads;
}

json::array OfficesToJson(const model::Map* map) {
    json::array offices;
    for (const auto& office : map->GetOffices()) {
        json::object json_office;
        json_office[std::string(ModelLiterals::ID)] = *office.GetId();
        json_office[std::string(ModelLiterals::POSITION_X)] = office.GetPosition().x;
        json_office[std::string(ModelLiterals::POSITION_Y)] = office.GetPosition().y;
        json_office[std::string(ModelLiterals::OFFSET_X)] = office.GetOffset().dx;
        json_office[std::string(ModelLiterals::OFFSET_Y)] = office.GetOffset().dy;
        offices.emplace_back(json_office);
    }
    return offices;
}

json::array BuildingsToJson(const model::Map* map) {
    json::array buildings;
    for (const auto& building : map->GetBuildings()) {
        json::object json_building;
        auto& bounds = building.GetBounds();
        json_building[std::string(ModelLiterals::POSITION_X)] = bounds.position.x;
        json_building[std::string(ModelLiterals::POSITION_Y)] = bounds.position.y;
        json_building[std::string(ModelLiterals::SIZE_WIDTH)] = bounds.size.width;
        json_building[std::string(ModelLiterals::SIZE_HEIGHT)] = bounds.size.height;
        buildings.emplace_back(json_building);
    }
    return buildings;
}

RequestHandler::RequestHandler(model::Game& game)
    : game_{ game } {
}

}  // namespace http_handler
