#pragma once
#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>
#include <filesystem>

namespace http_handler {

using namespace std::literals;
namespace beast = boost::beast;
namespace json = boost::json;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace sys = boost::system;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APP_JSON = "application/json"sv;
    constexpr static std::string_view TEXT_CSS = "text/css"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    constexpr static std::string_view TEXT_JAVASCRIPT = "text/javascript"sv;
    constexpr static std::string_view APP_XML = "application/xml"sv;
    constexpr static std::string_view PNG = "image/png"sv;
    constexpr static std::string_view JPEG = "image/jpeg"sv;
    constexpr static std::string_view GIF = "image/gif"sv;
    constexpr static std::string_view BMP = "image/bmp"sv;
    constexpr static std::string_view ICO = "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view TIFF = "image/tiff"sv;
    constexpr static std::string_view SVG = "image/svg+xml"sv;
    constexpr static std::string_view MP3 = "audio/mpeg"sv;
    constexpr static std::string_view UNKNOWN = "application/octet-stream"sv;
};

struct FileExtensions {
    FileExtensions() = delete;
    constexpr static std::string_view HTML = "html"sv;
    constexpr static std::string_view HTM = "htm"sv;
    constexpr static std::string_view JSON = "json"sv;
    constexpr static std::string_view CSS = "css"sv;
    constexpr static std::string_view TXT = "txt"sv;
    constexpr static std::string_view JS = "js"sv;
    constexpr static std::string_view XML = "xml"sv;
    constexpr static std::string_view PNG = "png"sv;
    constexpr static std::string_view JPEG = "jpeg"sv;
    constexpr static std::string_view JPG = "jpg"sv;
    constexpr static std::string_view JPE = "jpe"sv;
    constexpr static std::string_view GIF = "gif"sv;
    constexpr static std::string_view BMP = "bmp"sv;
    constexpr static std::string_view ICO = "ico"sv;
    constexpr static std::string_view TIFF = "tiff"sv;
    constexpr static std::string_view TIF = "tif"sv;
    constexpr static std::string_view SVG = "svg"sv;
    constexpr static std::string_view SVGZ = "svgz"sv;
    constexpr static std::string_view MP3 = "mp3"sv;
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
    explicit RequestHandler(model::Game& game, const char* path_to_static);

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto string_target = DecodeURL(req.target());
        std::string_view target(string_target);
        auto unslashed = target.substr(1, target.length() - 1);
        switch(CheckRequest(target)) {
        case RequestType::API_MAPS:
            SendResponse(http::status::ok, json::serialize(ProcessMapsRequestBody()), req.version(), std::move(send), ContentType::APP_JSON);
            break;
        case RequestType::API_MAP:
        {
            auto request = SplitRequest(unslashed);
            std::string id_text(request[3].data(), request[3].size());
            model::Map::Id id(id_text);
            const auto* map = game_.FindMap(id);
            if (map) {
                SendResponse(http::status::ok, json::serialize(MapToJSON(map)), req.version(), std::move(send), ContentType::APP_JSON);
            }
            else {
                SendResponse(http::status::not_found, MAP_NOT_FOUND_HTTP_BODY, req.version(), std::move(send), ContentType::APP_JSON);
            }
            break;
        }
        case RequestType::FILE:
            SendFileResponseOr404(target, std::move(send), req.version());
            break;
        case RequestType::BAD_REQUEST:
            SendBadRequest(std::move(send), req.version());
            break;
        default:
            SendBadRequest(std::move(send), req.version());
            break;
        }
    }

private:
    class ExtensionToConetntTypeMapper {
    public:
        ExtensionToConetntTypeMapper();

        std::string_view operator()(std::string_view extension) const;

    private:
        std::unordered_map<std::string_view, std::string_view> map_;
    };

    model::Game& game_;
    const fs::path root_path_;
    ExtensionToConetntTypeMapper mapper_;

    enum RequestType {
        API_MAP,
        API_MAPS,
        FILE,
        BAD_REQUEST
    };

    constexpr static std::string_view BAD_REQUEST_HTTP_BODY = R"({ "code": "badRequest", "message": "Bad request" })"sv;
    constexpr static std::string_view MAP_NOT_FOUND_HTTP_BODY = R"({ "code": "mapNotFound", "message": "Map not found" })"sv;
    constexpr static std::string_view FILE_NOT_FOUND_HTTP_BODY = R"({ "code": "fileNotFound", "message": "File not found" })"sv;

    template<typename Send>
    void SendBadRequest(Send&& send, unsigned http_version) const {
        SendResponse(http::status::bad_request, BAD_REQUEST_HTTP_BODY, http_version, std::move(send), ContentType::APP_JSON);
    }

    template<typename Send>
    void SendFileResponseOr404(std::string_view path, Send&& send, unsigned http_version) const {
        http::response<http::file_body> res;
        res.version(http_version);
        res.result(http::status::ok);
        std::string full_path = root_path_.string() + path.data();
        std::size_t ext_start = path.find_last_of('.', path.size());
        if (ext_start != path.npos) {
            auto type = mapper_(path.substr(ext_start + 1, path.size() - ext_start + 1));
            res.insert(http::field::content_type, type);
        }
        else {
            full_path = root_path_.string() + "/index.html";
            res.insert(http::field::content_type, ContentType::TEXT_HTML);
        }

        http::file_body::value_type file;

        if (sys::error_code ec; file.open(full_path.data(), beast::file_mode::read, ec), ec) {
            SendResponse(http::status::not_found, FILE_NOT_FOUND_HTTP_BODY, http_version, std::move(send), ContentType::TEXT_PLAIN);
            return;
        }

        res.body() = std::move(file);
        res.prepare_payload();
        send(res);
    }

    template<typename Send>
    void SendResponse(http::status status, std::string_view body, unsigned http_version, Send&& send, std::string_view type) const {
        http::response<http::string_body> response(status, http_version);
        response.insert(http::field::content_type, type);
        response.body() = body;
        send(response);
    }

    RequestType CheckRequest(std::string_view target) const;
    json::array ProcessMapsRequestBody() const;
    std::string DecodeURL(std::string_view url) const;
};

}  // namespace http_handler