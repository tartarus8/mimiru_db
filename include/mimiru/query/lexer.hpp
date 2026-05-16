#pragma once

#include <string>
#include <vector>
#include <stdexcept>

namespace mimiru::query {

enum class TokenType {
    Keyword,
    Identifier,
    Integer,
    String,
    Symbol,
    Operator,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string value;

    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }
};

class LexerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Lexer {
public:
    explicit Lexer(std::string input);

    std::vector<Token> tokenize();

private:
    std::string input_;
    size_t pos_ = 0;

    char peek() const;
    char advance();
    bool is_at_end() const;
    void skip_whitespace();

    Token read_string();
    Token read_number();
    Token read_identifier_or_keyword();
    Token read_operator();

    bool is_keyword(const std::string& word) const;
};

} // namespace mimiru::query
