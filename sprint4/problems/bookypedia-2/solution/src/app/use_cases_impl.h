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
        :work_(conn) {
        authors_.SetWork(&work_);
        books_.SetWork(&work_);
    }

    void Commit() override;
    domain::AuthorRepository& Authors() override;
    domain::BookRepository& Books() override;

private:
    postgres::AuthorRepositoryImpl authors_;
    postgres::BookRepositoryImpl books_;
    pqxx::work work_;
};

class UnitOfWorkFactoryImpl : public UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(pqxx::connection& conn)
        :conn_(conn) {
    }

    UnitOfWorkImpl* CreateUnit() override;

private:
    pqxx::connection& conn_;
};

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(pqxx::connection& conn)
        : factory_{conn} {
    }

    void StartWork() override;
    void EndWork() override;
    void AddAuthor(const std::string& name) override;
    void AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) override;
    std::vector<domain::Author> GetAuthors() override;
    std::vector<domain::Book> GetBooks() override;
    std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& id) override;

private:
    UnitOfWorkFactoryImpl factory_;
    std::unique_ptr<UnitOfWorkImpl> unit_;

    void AssertUnit();
};

}  // namespace app
