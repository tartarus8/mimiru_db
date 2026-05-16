#pragma once

#include "mimiru/query/lexer.hpp"
#include "mimiru/query/ast.hpp"

namespace mimiru::query {

class ParserError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::vector<ast::Statement> parse();

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    const Token& peek() const;
    const Token& previous() const;
    bool is_at_end() const;
    const Token& advance();

    bool match(TokenType type, const std::string& value = "");
    void consume(TokenType type, const std::string& value, const std::string& error_msg);

    ast::Statement parse_statement();
    ast::Statement parse_create();
    ast::Statement parse_drop();
    ast::Statement parse_use();
    ast::Statement parse_insert();
    ast::Statement parse_select();

    std::unique_ptr<ast::Expr> parse_expression();
    ast::Operator parse_operator();
};

} // namespace mimiru::query
