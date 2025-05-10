#pragma once

#include "../domain/author.h"
#include "../domain/book.h"

#include <string>

namespace app {

class UnitOfWork {
public:
    virtual void Commit() = 0;
    virtual domain::AuthorRepository& Authors() = 0;
    virtual domain::BookRepository& Books() = 0;
};

class UnitOfWorkFactory {
public:
    virtual UnitOfWork* CreateUnit() = 0;
};

class UseCases {
public:
    virtual void StartWork() = 0;
    virtual void EndWork() = 0;
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) = 0;
    virtual std::vector<domain::Author> GetAuthors() = 0;
    virtual std::vector<domain::Book> GetBooks() = 0;
    virtual std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& id) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
