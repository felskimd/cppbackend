#pragma once

#include "../app/use_cases.h"

#include <iosfwd>
#include <optional>
#include <string>
#include <memory>
#include <vector>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int publication_year = 0;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string title;
    int publication_year;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks() const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;

    std::optional<detail::AddBookParams> GetBookParams(app::UnitOfWork* unit, std::istream& cmd_input) const;
    std::optional<detail::AuthorInfo> SelectAuthor(app::UnitOfWork* unit) const;
    std::optional<detail::AuthorInfo> SelectAuthorFromList(app::UnitOfWork* unit) const;
    std::vector<detail::AuthorInfo> GetAuthors(app::UnitOfWork* unit) const;
    std::vector<detail::BookInfo> GetBooks(app::UnitOfWork* unit) const;
    std::vector<detail::BookInfo> GetAuthorBooks(app::UnitOfWork* unit, const std::string& author_id) const;
    void AddTags(app::UnitOfWork* unit, const std::string& book) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui