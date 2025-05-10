#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    AuthorRepositoryImpl() = default;

    void SetWork(pqxx::work* work);
    void ResetWork();
    void Save(const domain::Author& author) override;
    std::vector<domain::Author> GetAuthors() override;

private:
    pqxx::work* work_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    BookRepositoryImpl() = default;

    void SetWork(pqxx::work* work);
    void ResetWork();
    void Save(const domain::Book& book) override;
    std::vector<domain::Book> GetBooks() override; 
    std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& id) override;

private:
    pqxx::work* work_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

    pqxx::connection& GetConnection() {
        return connection_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_;
    BookRepositoryImpl books_;
};

}  // namespace postgres