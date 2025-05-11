#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UnitOfWorkImpl::Commit() {
    work_.commit();
}

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
    auto books = books_.GetBooks();
    for (auto& book : books) {
        book.SetAuthorName(authors_.GetAuthorById(book.GetAuthor()).GetName());
    }
    return books;
}

std::vector<domain::Book> UnitOfWorkImpl::GetBooksByAuthor(const domain::AuthorId& id) {
    return books_.GetBooksByAuthor(id);
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

void UnitOfWorkImpl::DeleteAuthor(const domain::Author& author) {
    authors_.DeleteAuthor(author.GetName());
    books_.DeleteBooksOfAuthor(author.GetId());
}

void UnitOfWorkImpl::EditAuthor(const domain::Author& author) {
    authors_.Save(author);
}

std::unique_ptr<UnitOfWork> UnitOfWorkFactoryImpl::CreateUnit() {
    return std::make_unique<UnitOfWorkImpl>(conn_);
}

std::unique_ptr<UnitOfWork> UseCasesImpl::GetUnit() {
    return factory_.CreateUnit();
}

}  // namespace app
