#include <boost/json.hpp>
#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string_view>
#include <string>

using namespace std::literals;
// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

constexpr auto tag_add_book = "add_book"_zv;
constexpr auto get_books_query = "SELECT * FROM books (id, title, author, year, ISBN) ORDER BY year DESC, title ASC, author ASC, ISBN ASC;"_zv;

void InitializeDB(pqxx::connection& conn) {
    pqxx::work w(conn);
    w.exec("CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100) NOT NULL, author varchar(100) NOT NULL, year integer NOT NULL, ISBN char(13) UNIQUE);"_zv);
    w.exec("DELETE FROM books;"_zv);
    w.commit();
    conn.prepare(tag_add_book, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4);"_zv); 
}

enum Action {
    ADD_BOOK,
    ALL_BOOKS,
    EXIT
};

struct PayloadData {
    std::string title;
    std::string author;
    int year;
    std::optional<std::string> isbn;
};

std::string_view FindAndParse(std::string_view key, std::string_view query) {
    size_t key_pos = query.find(key);
    size_t key_end_pos = query.find("\"", key_pos);
    size_t value_pos = query.find("\"", key_end_pos);
    if (value_pos == query.npos) {
        return ""sv;
    }
    size_t value_end_pos = query.find("\"", value_pos);
    return query.substr(value_pos, value_end_pos - value_pos);
}

std::pair<Action, PayloadData> ParseQuery(const std::string& query) {
    //auto json = boost::json::parse(query).as_object();
    //auto action = json.at("action").as_string();
    auto action = FindAndParse("action", query);
    if (action == "add_book"sv) {
        //auto payload = json.at("payload").as_object();
        //auto payload = FindAndParse("payload", query);
        //auto title = payload.at("title").as_string();
        auto title = FindAndParse("title", query);
        //auto author = payload.at("author").as_string();
        auto author = FindAndParse("author", query);
        //auto year = payload.at("year").as_int64();
        auto year = FindAndParse("year", query);
        std::optional<std::string> isbn;
        auto isbn_parsed = FindAndParse("ISBN", query);
        if (isbn_parsed.size() != 0) {
            isbn.emplace(isbn_parsed.data(), isbn_parsed.size());
        }
        /*std::optional<std::string> isbn;
        if (!payload.at("ISBN").is_null()) {
            isbn.emplace(std::string(payload.at("ISBN").as_string().c_str()));
            
            throw std::runtime_error(std::string(title.c_str()) + " + " + std::string(author.c_str()) + " + " + std::to_string(year) + " + " + *isbn);
        }*/
        throw std::runtime_error(std::string(title.c_str()) + " + " + std::string(author.c_str()) + " + " + std::to_string(year) + " + " + *isbn);
        return { Action::ADD_BOOK, PayloadData{ std::string(title.c_str()), std::string(author.c_str()), static_cast<int>(year), isbn } };
    }
    else {
        if (action == "all_books") {
            return { Action::ALL_BOOKS, PayloadData{} };
        }
        else {
            return { Action::EXIT, PayloadData{} };
        }
    }
    throw std::exception();
}

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: connect_db <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }
        pqxx::connection conn{argv[1]};
        InitializeDB(conn);
        
        std::string query;
        while (std::getline(std::cin, query)) {
            auto [action, data] = ParseQuery(query);
            switch (action) {
                case Action::ADD_BOOK: 
                {
                    pqxx::work w(conn);
                    auto result = w.exec_prepared(tag_add_book, data.title, data.author, data.year, data.isbn ? *data.isbn : "null"_zv);
                    w.commit();
                    std::cout << "{\"result\":";
                    if (result.affected_rows() > 0) {
                        std::cout << "true";
                    }
                    else {
                        std::cout << "false";
                    }
                    std::cout << "}" << std::endl;
                    break;
                }
                case Action::ALL_BOOKS:
                {
                    pqxx::read_transaction r(conn);
                    std::cout << "[";
                    bool first = true;
                    for (auto [id, title, author, year, isbn] 
                    : r.query<int, std::string, std::string, int, std::optional<std::string>>(get_books_query)) {
                        if (!first) {
                            std::cout << ",";
                        }
                        else {
                            first = false;
                        }
                        std::cout << "{\"id\":" << id << ",\"title\":\"" << title;
                        std::cout << "\",\"author\":\"" << author << "\",\"year\":" << std::to_string(year);
                        std::cout << ",\"ISBN\":";
                        if (isbn) {
                            std::cout << "\"" << *isbn << "\"";
                        }
                        else {
                            std::cout << "null";
                        }
                        std::cout << "}";
                    }
                    std::cout << "]" << std::endl;
                    break;
                }
                case Action::EXIT:
                    return 0;
            }
        }
    } 
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}