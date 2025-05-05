#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string_view>

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
    std::string_view title;
    std::string_view author;
    int year;
    std::optional<std::string_view> isbn;
};

Action ParseAction(std::string_view query) {
    size_t action_end_pos = query.find("\"action\": \""sv) + 10;
    std::string_view parsed_action = query.substr(
        action_end_pos
        , query.find('"', action_end_pos + 1) - action_end_pos
    );
    if (parsed_action == "add_book"sv) {
        return Action::ADD_BOOK;
    }
    if (parsed_action == "all_books"sv) {
        return Action::ALL_BOOKS;
    }
    if (parsed_action == "exit"sv) {
        return Action::EXIT;
    }
    throw std::exception();
}

PayloadData ParseData(std::string_view query) {
    size_t title_start = query.find("\"title\": \""sv) + 9;
    std::string_view title = query.substr(
        title_start
        , query.find('"', title_start + 1) - title_start
    );
    size_t author_start = query.find("\"author\": \""sv) + 10;
    std::string_view author = query.substr(
        author_start
        , query.find('"', author_start + 1) - author_start
    );
    size_t year_start = query.find("\"year\": "sv) + 7;
    std::string_view year_text = query.substr(
        year_start
        , query.find(',', year_start + 1) - year_start
    );
    int year = std::stoi(std::string(year_text));
    size_t isbn_start = query.find("\"ISBN\": "sv) + 7;
    std::optional<std::string_view> isbn;
    if (query.find("\"ISBN\": \""sv) != query.npos) {
        isbn.emplace(query.substr(
            isbn_start
            , query.find('"', isbn_start + 1) - isbn_start
        ));
    }
    return {title, author, year, isbn};
}

std::pair<Action, PayloadData> ParseQuery(std::string_view query) {
    auto action = ParseAction(query);
    switch(action) {
        case Action::ADD_BOOK: 
            return {action, ParseData(query)};
        case Action::ALL_BOOKS:
            return {action, PayloadData{}};
        case Action::EXIT:
            return {action, PayloadData{}};
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