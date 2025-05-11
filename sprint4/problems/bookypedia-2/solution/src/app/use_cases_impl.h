#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "../postgres/postgres.h"
#include "use_cases.h"

#include <pqxx/pqxx>

namespace app {

class UnitOfWorkImpl : public UnitOfWork {
public:
    explicit UnitOfWorkImpl(pqxx::connection& conn)
        :work_{conn},
        authors_{work_},
        books_{work_}{
    }

    void Commit() override;
    //void Abort();
    void AddAuthor(const std::string& name) override;
    void AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) override;
    std::vector<domain::Author> GetAuthors() override;
    std::vector<domain::Book> GetBooks() override;
    std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& id) override;
    std::optional<domain::Author> GetAuthorIfExists(const std::string& name) override;
    std::optional<domain::Book> GetBookIfExists(const std::string& title) override;
    void AddTags(const domain::BookId& id, const std::vector<std::string>& tags) override;
    void DeleteAuthor(const domain::Author& author) override;
    void EditAuthor(const domain::Author& author) override;

private:
    pqxx::work work_;
    postgres::AuthorRepositoryImpl authors_;
    postgres::BookRepositoryImpl books_;
};

class UnitOfWorkFactoryImpl : public UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(pqxx::connection& conn)
        :conn_(conn) {
    }

    std::unique_ptr<UnitOfWork> CreateUnit() override;

private:
    pqxx::connection& conn_;
};

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(pqxx::connection& conn)
        : factory_{conn} {
    }

    std::unique_ptr<UnitOfWork> GetUnit() override;
    /*void AddAuthor(const std::string& name) override;
    void AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) override;
    std::vector<domain::Author> GetAuthors() override;
    std::vector<domain::Book> GetBooks() override;
    std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& id) override;*/

private:
    UnitOfWorkFactoryImpl factory_;
    std::unique_ptr<UnitOfWorkImpl> unit_;

    //void AssertUnit();
};

}  // namespace app
