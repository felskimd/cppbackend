#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UnitOfWorkImpl::Commit() {
    work_.commit();
    authors_.ResetWork();
    books_.ResetWork();
}

AuthorRepository& UnitOfWorkImpl::Authors() {
    return authors_;
}

BookRepository& UnitOfWorkImpl::Books() {
    return books_;
}

UnitOfWorkImpl* UnitOfWorkFactoryImpl::CreateUnit() {
    return new UnitOfWorkImpl{conn_};
}

void UseCasesImpl::StartWork() {
    if (unit_) {
        throw std::runtime_error("Work already started");
    }
    //unit_ = std::make_unique<UnitOfWork>(factory_.CreateUnit());
    unit_ = factory_.CreateUnit();
}

void UseCasesImpl::EndWork() {
    if (!unit_) {
        throw std::runtime_error("Work not started");
    }
    unit_->Commit();
    //unit_.reset();
    delete unit_;
}

void UseCasesImpl::AddAuthor(const std::string& name) {
    AssertUnit();
    unit_->Authors().Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) {
    AssertUnit();
    unit_->Books().Save({BookId::New(), author, title, publication_year});
}

std::vector<Author> UseCasesImpl::GetAuthors() {
    AssertUnit();
    return unit_->Authors().GetAuthors();
}

std::vector<Book> UseCasesImpl::GetBooks() {
    AssertUnit();
    return unit_->Books().GetBooks();
}

std::vector<Book> UseCasesImpl::GetBooksByAuthor(const domain::AuthorId& id) {
    AssertUnit();
    return unit_->Books().GetBooksByAuthor(id);
}

void UseCasesImpl::AssertUnit() {
    if (!unit_) {
        throw std::runtime_error("No unit");
    }
}

}  // namespace app
