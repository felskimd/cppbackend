#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

std::string RemoveExtraSpaces(const std::string& input) {
    std::istringstream iss(input);
    std::string word;
    std::string result;

    while (iss >> word) {
        if (!result.empty()) {
            result += " ";
        }
        result += word;
    }

    return result;
}

std::vector<std::string> ParseTags(std::istream& cmd_input) {
    std::string str;
    if (!std::getline(cmd_input, str) || str.empty()) {
        return {};
    }
    std::vector<std::string> result;
    boost::split(result, str, boost::is_any_of(","));

    result.erase(std::remove_if(result.begin(), result.end(),
        [](const std::string& s) { return s.empty(); }),
        result.end());

    for (auto& tag : result) {
        tag = RemoveExtraSpaces(tag);
    }

    std::sort(result.begin(), result.end());
    auto last = std::unique(result.begin(), result.end());
    result.erase(last, result.end());
    return result;
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        if (name.empty()) {
            throw std::exception();
        }
        boost::algorithm::trim(name);
        auto unit = use_cases_.GetUnit();
        unit->AddAuthor(std::move(name));
        unit->Commit();
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        auto unit = use_cases_.GetUnit();
        if (auto params = GetBookParams(unit.get(), cmd_input)) {
            unit->AddBook(domain::AuthorId::FromString(params->author_id), params->title, params->publication_year);
            AddTags(unit.get(), params->title);
        }
        unit->Commit();
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    auto unit = use_cases_.GetUnit();
    PrintVector(output_, GetAuthors(unit.get()));
    unit->Commit();
    return true;
}

bool View::ShowBooks() const {
    auto unit = use_cases_.GetUnit();
    PrintVector(output_, GetBooks(unit.get()));
    unit->Commit();
    return true;
}

bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        auto unit = use_cases_.GetUnit();
        if (auto author_id = SelectAuthor(unit.get())) {
            PrintVector(output_, GetAuthorBooks(unit.get(), *author_id));
        }
        unit->Commit();
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(app::UnitOfWork* unit, std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    auto author_id = SelectAuthor(unit);
    if (not author_id.has_value()) {
        return std::nullopt;
    }
    params.author_id = author_id.value();
    return params;
}

std::optional<std::string> View::SelectAuthor(app::UnitOfWork* unit) const {
    output_ << "Enter author name or empty line to select from list:"sv << std::endl;
    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return SelectAuthorFromList(unit);
    }
    
    boost::algorithm::trim(str);
    if (auto author = unit->GetAuthorIfExists(str)) {
        return author->GetId().ToString();
    }
    output_ << "No author found. Do you want to add "sv << str << " (y/n)?"sv << std::endl;
    std::string yes_or_no;
    if (!std::getline(input_, yes_or_no) || yes_or_no.empty()) {
        throw std::runtime_error("Parsing failed");
    }
    if (yes_or_no == "y" || yes_or_no == "Y") {
        unit->AddAuthor(str);
        return unit->GetAuthorIfExists(str).value().GetId().ToString();
    }
    throw std::runtime_error("Invalid answer");
}

std::optional<std::string> View::SelectAuthorFromList(app::UnitOfWork* unit) const {
    auto authors = GetAuthors(unit);
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel"sv << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    }
    catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors(app::UnitOfWork* unit) const {
    std::vector<detail::AuthorInfo> dst_autors;
    for (auto& author : unit->GetAuthors()) {
        dst_autors.emplace_back(std::move(author.GetId().ToString()), author.GetName());
    }
    return dst_autors;
}

std::vector<detail::BookInfo> View::GetBooks(app::UnitOfWork* unit) const {
    std::vector<detail::BookInfo> books;
    for (auto& book : unit->GetBooks()) {
        books.emplace_back(book.GetTitle(), book.GetYear());
    }
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(app::UnitOfWork* unit, const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    for (auto& book : unit->GetBooksByAuthor(domain::AuthorId::FromString(author_id))) {
        books.emplace_back(book.GetTitle(), book.GetYear());
    }
    return books;
}

void View::AddTags(app::UnitOfWork* unit, const std::string& book) const {
    output_ << "Enter tags (comma separated):" << std::endl;
    auto tags = ParseTags(input_);
    if (tags.empty()) {
        return;
    }
    auto found_book = unit->GetBookIfExists(book).value();
    unit->AddTags(found_book.GetId(), tags);
}

}  // namespace ui
