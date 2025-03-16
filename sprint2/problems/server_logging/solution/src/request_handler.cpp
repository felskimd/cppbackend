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
    obj[std::string(model::ModelLiterals::ID)] = *map->GetId();
    obj[std::string(model::ModelLiterals::NAME)] = map->GetName();
    obj[std::string(model::ModelLiterals::ROADS)] = RoadsToJson(map);
    obj[std::string(model::ModelLiterals::BUILDINGS)] = BuildingsToJson(map);
    obj[std::string(model::ModelLiterals::OFFICES)] = OfficesToJson(map);
    return obj;
}

json::array RoadsToJson(const model::Map* map) {
    json::array roads;
    for (const auto& road : map->GetRoads()) {
        json::object json_road;
        json_road[std::string(model::ModelLiterals::START_X)] = road.GetStart().x;
        json_road[std::string(model::ModelLiterals::START_Y)] = road.GetStart().y;
        road.IsHorizontal() ? json_road[std::string(model::ModelLiterals::END_X)] = road.GetEnd().x 
            : json_road[std::string(model::ModelLiterals::END_Y)] = road.GetEnd().y;
        roads.emplace_back(json_road);
    }
    return roads;
}

json::array OfficesToJson(const model::Map* map) {
    json::array offices;
    for (const auto& office : map->GetOffices()) {
        json::object json_office;
        json_office[std::string(model::ModelLiterals::ID)] = *office.GetId();
        json_office[std::string(model::ModelLiterals::POSITION_X)] = office.GetPosition().x;
        json_office[std::string(model::ModelLiterals::POSITION_Y)] = office.GetPosition().y;
        json_office[std::string(model::ModelLiterals::OFFSET_X)] = office.GetOffset().dx;
        json_office[std::string(model::ModelLiterals::OFFSET_Y)] = office.GetOffset().dy;
        offices.emplace_back(json_office);
    }
    return offices;
}

json::array BuildingsToJson(const model::Map* map) {
    json::array buildings;
    for (const auto& building : map->GetBuildings()) {
        json::object json_building;
        auto& bounds = building.GetBounds();
        json_building[std::string(model::ModelLiterals::POSITION_X)] = bounds.position.x;
        json_building[std::string(model::ModelLiterals::POSITION_Y)] = bounds.position.y;
        json_building[std::string(model::ModelLiterals::MODEL_SIZE_WIDTH)] = bounds.size.width;
        json_building[std::string(model::ModelLiterals::MODEL_SIZE_HEIGHT)] = bounds.size.height;
        buildings.emplace_back(json_building);
    }
    return buildings;
}

RequestHandler::RequestHandler(model::Game& game, const char* path_to_static)
    : game_{ game },
    root_path_(path_to_static) {
}

json::array RequestHandler::ProcessMapsRequestBody() const {
    auto maps_body = json::array();
    for (const auto& map : game_.GetMaps()) {
        json::object body;
        body[std::string(model::ModelLiterals::ID)] = *map.GetId();
        body[std::string(model::ModelLiterals::NAME)] = map.GetName();
        maps_body.emplace_back(std::move(body));
    }
    return maps_body;
}

RequestHandler::RequestType RequestHandler::CheckRequest(std::string_view target) const {
    auto request = SplitRequest(target.substr(1, target.length() - 1));
    if (request.size() > 2 && request[0] == RestApiLiterals::API && request[1] == RestApiLiterals::VERSION_1 && request[2] == RestApiLiterals::MAPS) {
        if (request.size() == 3) {
            return RequestHandler::RequestType::API_MAPS;
        }
        else if (request.size() == 4) {
            return RequestHandler::RequestType::API_MAP;
        }
        else {
            return RequestHandler::RequestType::BAD_REQUEST;
        }
    }
    if (request[0] == RestApiLiterals::API) {
        return RequestHandler::RequestType::BAD_REQUEST;
    }
    auto temp_path = root_path_;
    temp_path += target;
    auto path = fs::weakly_canonical(temp_path);
    auto canonical_root = fs::weakly_canonical(root_path_);
    for (auto b = canonical_root.begin(), p = path.begin(); b != canonical_root.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return RequestHandler::RequestType::BAD_REQUEST;
        }
    }
    return RequestHandler::RequestType::FILE;
}

RequestHandler::ExtensionToConetntTypeMapper::ExtensionToConetntTypeMapper() {
    map_[FileExtensions::HTML] = ContentType::TEXT_HTML;
    map_[FileExtensions::HTM] = ContentType::TEXT_HTML;
    map_[FileExtensions::JSON] = ContentType::APP_JSON;
    map_[FileExtensions::CSS] = ContentType::TEXT_CSS;
    map_[FileExtensions::TXT] = ContentType::TEXT_PLAIN;
    map_[FileExtensions::JS] = ContentType::TEXT_JAVASCRIPT;
    map_[FileExtensions::XML] = ContentType::APP_XML;
    map_[FileExtensions::PNG] = ContentType::PNG;
    map_[FileExtensions::JPEG] = ContentType::JPEG;
    map_[FileExtensions::JPG] = ContentType::JPEG;
    map_[FileExtensions::JPE] = ContentType::JPEG;
    map_[FileExtensions::GIF] = ContentType::GIF;
    map_[FileExtensions::BMP] = ContentType::BMP;
    map_[FileExtensions::ICO] = ContentType::ICO;
    map_[FileExtensions::TIFF] = ContentType::TIFF;
    map_[FileExtensions::TIF] = ContentType::TIFF;
    map_[FileExtensions::SVG] = ContentType::SVG;
    map_[FileExtensions::SVGZ] = ContentType::SVG;
    map_[FileExtensions::MP3] = ContentType::MP3;
}

std::string_view RequestHandler::ExtensionToConetntTypeMapper::operator()(std::string_view extension) const {
    if (map_.contains(extension)) {
        return map_.at(extension);
    }
    return ContentType::UNKNOWN;
}

std::string RequestHandler::DecodeURL(std::string_view url) const {
    std::vector<char> text;
    text.reserve(url.length());
    for (size_t i = 0; i < url.length(); ++i) {
        if (url[i] == '%') {
            if (i + 2 < url.length()) {
                char hex1 = url[i + 1];
                char hex2 = url[i + 2];

                if (isxdigit(hex1) && isxdigit(hex2)) {
                    int code = std::stoi(std::string() + hex1 + hex2, nullptr, 16);
                    text.emplace_back(static_cast<char>(code));
                    i += 2;
                }
                else {
                    text.emplace_back('%');
                }
            }
            else {
                text.emplace_back('%');
            }
        }
        else if (url[i] == '+') {
            text.emplace_back(' ');
        }
        else {
            text.emplace_back(url[i]);
        }
    }
    return std::string(text.data(), text.size());
}



}  // namespace http_handler
