#pragma once
#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>

namespace http_handler {

using namespace std::literals;
namespace beast = boost::beast;
namespace json = boost::json;
namespace http = beast::http;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APP_JSON = "application/json"sv;
};

struct ModelLiterals {
    ModelLiterals() = delete;
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
    constexpr static std::string_view SIZE_WIDTH = "w"sv;
    constexpr static std::string_view SIZE_HEIGHT = "h"sv;
    constexpr static std::string_view ROADS = "roads"sv;
    constexpr static std::string_view OFFICES = "offices"sv;
    constexpr static std::string_view BUILDINGS = "buildings"sv;
};

struct RestApiLiterals {
    RestApiLiterals() = delete;
    constexpr static std::string_view API = "api"sv;
    constexpr static std::string_view VERSION_1 = "v1"sv;
    constexpr static std::string_view MAPS = "maps"sv;
};

std::vector<std::string_view> SplitRequest(std::string_view body);

json::object MapToJSON(const model::Map* map);
json::array RoadsToJson(const model::Map* map);
json::array OfficesToJson(const model::Map* map);
json::array BuildingsToJson(const model::Map* map);

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game);

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto request = SplitRequest(std::string_view(req.target().substr(1, req.target().size() - 1)));
        if (request[0] == RestApiLiterals::API && request[1] == RestApiLiterals::VERSION_1 && request[2] == RestApiLiterals::MAPS) {
            if (request.size() == 3) {
                auto maps_body = json::array();
                for (const auto& map : game_.GetMaps()) {
                    json::object body;
                    body[std::string(ModelLiterals::ID)] = *map.GetId();
                    body[std::string(ModelLiterals::NAME)] = map.GetName();
                    maps_body.emplace_back(std::move(body));
                }
                SendResponse(http::status::ok, json::serialize(maps_body), req.version(), std::move(send));
            }
            else if (request.size() == 4) {
                std::string id_text(request[3].data(), request[3].size());
                model::Map::Id id(id_text);
                const auto* map = game_.FindMap(id);
                if (map) {
                    SendResponse(http::status::ok, json::serialize(MapToJSON(map)), req.version(), std::move(send));
                }
                else {
                    SendResponse(http::status::not_found, MAP_NOT_FOUND_HTTP_BODY, req.version(), std::move(send));
                }
            }
            else {
                SendBadRequest(std::move(send), req.version());
            }
        }
        else {
            SendBadRequest(std::move(send), req.version());
        }
    }

private:
    model::Game& game_;

    constexpr static std::string_view BAD_REQUEST_HTTP_BODY = R"({ "code": "badRequest", "message": "Bad request" })"sv;
    constexpr static std::string_view MAP_NOT_FOUND_HTTP_BODY = R"({ "code": "mapNotFound", "message": "Map not found" })"sv;

    template<typename Send>
    void SendBadRequest(Send&& send, unsigned http_version) {
        SendResponse(http::status::bad_request, BAD_REQUEST_HTTP_BODY, http_version, std::move(send));
    }

    template<typename Send>
    void SendResponse(http::status status, std::string_view body, unsigned http_version, Send&& send) {
        http::response<http::string_body> response(status, http_version);
        response.set(http::field::content_type, ContentType::APP_JSON);
        response.body() = body;
        send(response);
    }
};

}  // namespace http_handler