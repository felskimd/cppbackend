#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UnitOfWorkImpl::Commit() {
    work_.commit();
}

//AuthorRepository& UnitOfWorkImpl::Authors() {
//    return authors_;
//}
//
//BookRepository& UnitOfWorkImpl::Books() {
//    return books_;
//}

void UnitOfWorkImpl::AddAuthor(const std::string& name) {
    authors_.Save({ AuthorId::New(), name });
}

void UnitOfWorkImpl::AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) {
    books_.Save({ BookId::New(), author, title, publication_year });
}

std::vector<domain::Author> UnitOfWorkImpl::GetAuthors() {
    return authors_.GetAuthors();
}

std::vector<domain::Book> UnitOfWorkImpl::GetBooks() {
    return books_.GetBooks();
}

std::vector<domain::Book> UnitOfWorkImpl::GetBooksByAuthor(const domain::AuthorId& id) {
    books_.GetBooksByAuthor(id);
}

std::optional<domain::Author> UnitOfWorkImpl::GetAuthorIfExists(const std::string& name) {
    return authors_.GetAuthorIfExists(name);
}

std::optional<domain::Book> UnitOfWorkImpl::GetBookIfExists(const std::string& title) {
    return books_.GetBookIfExists(title);
}

void UnitOfWorkImpl::AddTags(const domain::BookId& id, const std::vector<std::string>& tags) {
    books_.AddTags(id, tags);
}

std::unique_ptr<UnitOfWork> UnitOfWorkFactoryImpl::CreateUnit() {
    //return UnitOfWorkImpl{conn_};
    return std::make_unique<UnitOfWorkImpl>(conn_);
}

std::unique_ptr<UnitOfWork> UseCasesImpl::GetUnit() {
    return factory_.CreateUnit();
}

//void UseCasesImpl::AddAuthor(const std::string& name) {
//    AssertUnit();
//    unit_->Authors().Save({AuthorId::New(), name});
//}
//
//void UseCasesImpl::AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) {
//    AssertUnit();
//    unit_->Books().Save({BookId::New(), author, title, publication_year});
//}
//
//std::vector<Author> UseCasesImpl::GetAuthors() {
//    AssertUnit();
//    return unit_->Authors().GetAuthors();
//}
//
//std::vector<Book> UseCasesImpl::GetBooks() {
//    AssertUnit();
//    return unit_->Books().GetBooks();
//}
//
//std::vector<Book> UseCasesImpl::GetBooksByAuthor(const domain::AuthorId& id) {
//    AssertUnit();
//    return unit_->Books().GetBooksByAuthor(id);
//}
//
//void UseCasesImpl::AssertUnit() {
//    if (!unit_) {
//        throw std::runtime_error("No unit");
//    }
//}

}  // namespace app
