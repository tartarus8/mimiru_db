#include "mimiru/query/lexer.hpp"
#include <cctype>
#include <unordered_set>
#include <algorithm>

namespace mimiru::query {

Lexer::Lexer(std::string input) : input_(std::move(input)) {}

char Lexer::peek() const {
    if (is_at_end()) {
        return '\0';
    }
    return input_[pos_];
}

char Lexer::advance() {
    return input_[pos_++];
}

bool Lexer::is_at_end() const {
    return pos_ >= input_.size();
}

void Lexer::skip_whitespace() {
    while (!is_at_end() && std::isspace(peek())) {
        advance();
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!is_at_end()) {
        skip_whitespace();
        if (is_at_end()) {
            break;
        }

        char c = peek();

        if (std::isalpha(c) || c == '_') {
            tokens.push_back(read_identifier_or_keyword());
        }
        else if (std::isdigit(c) || (c == '-' && std::isdigit(input_[pos_ + 1]))) {
            tokens.push_back(read_number());
        }
        else if (c == '"') {
            tokens.push_back(read_string());
        }
        else if (c == '=' || c == '!' || c == '<' || c == '>') {
            tokens.push_back(read_operator());
        }
        else if (std::string(";,().*").find(c) != std::string::npos) {
            tokens.push_back({TokenType::Symbol, std::string(1, advance())});
        }
        else {
            throw LexerError("unexpected character: " + std::string(1, c));
        }
    }

    tokens.push_back({TokenType::EndOfFile, ""});

    return tokens;
}

Token Lexer::read_string() {
    advance();
    std::string value;
    while (!is_at_end() && peek() != '"') {
        value += advance();
    }
    if (is_at_end()) {
        throw LexerError("unterminated string literal");
    }
    advance();

    return {
        TokenType::String,
        value
    };
}

Token Lexer::read_number() {
    std::string value;
    if (peek() == '-') {
        value += advance();
    }
    while (!is_at_end() && std::isdigit(peek())) {
        value += advance();
    }
    return {
        TokenType::Integer,
        value
    };
}

Token Lexer::read_operator() {
    char first = advance();
    if (!is_at_end() && peek() == '=') {
        advance();
        return {TokenType::Operator, std::string(1, first) + "="};
    }
    return {
        TokenType::Operator,
        std::string(1, first)
    };
}

bool Lexer::is_keyword(const std::string& word) const {
    static const std::unordered_set<std::string> keywords = {
        "CREATE", "DATABASE", "DROP", "USE", "TABLE", "INSERT", "INTO",
        "VALUE", "UPDATE", "SET", "DELETE", "FROM", "SELECT", "WHERE",
        "AS", "AND", "OR", "NOT_NULL", "INDEXED", "DEFAULT", "INT", "STRING"
    };

    return keywords.find(word) != keywords.end();
}

Token Lexer::read_identifier_or_keyword() {
    std::string word;
    bool has_upper = false;
    bool has_lower = false;

    while (!is_at_end() && (std::isalnum(peek()) || peek() == '_')) {
        char c = advance();
        word += c;
        if (std::isalpha(c)) {
            if (std::isupper(c)) {
                has_upper = true;
            }
            if (std::islower(c)) {
                has_lower = true;
            }
        }
    }

    if (has_upper && has_lower) {
        throw LexerError("mixed case not allowed in identifier/keyword: " + word);
    }

    std::string upper_word = word;
    for (char& c : upper_word) c = std::toupper(c);

    if (is_keyword(upper_word)) {
        return {TokenType::Keyword, upper_word};
    }
    return {
        TokenType::Identifier,
        word
    };
}

} // namespace mimiru::query
