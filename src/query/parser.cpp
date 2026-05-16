#include "mimiru/query/parser.hpp"

namespace mimiru::query {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token& Parser::peek() const {
    return tokens_[pos_];
}
const Token& Parser::previous() const {
    return tokens_[pos_ - 1];
}
bool Parser::is_at_end() const {
    return peek().type == TokenType::EndOfFile;
}

const Token& Parser::advance() {
    if (!is_at_end()) {
        pos_++;
    }

    return previous();
}

bool Parser::match(TokenType type, const std::string& value) {
    if (is_at_end()) {
        return false;
    }
    if (peek().type != type) {
        return false;
    }
    if (!value.empty() && peek().value != value) {
        return false;
    }
    advance();

    return true;
}

void Parser::consume(TokenType type, const std::string& value, const std::string& error_msg) {
    if (match(type, value)) {
        return;
    }
    throw ParserError(error_msg + " found: " + peek().value);
}

std::vector<ast::Statement> Parser::parse() {
    std::vector<ast::Statement> statements;
    while (!is_at_end()) {
        statements.push_back(parse_statement());
        consume(TokenType::Symbol, ";", "expected ';' after statement");
    }

    return statements;
}

ast::Statement Parser::parse_statement() {
    if (match(TokenType::Keyword, "CREATE")) {
        return parse_create();
    }
    if (match(TokenType::Keyword, "DROP")) {
        return parse_drop();
    }
    if (match(TokenType::Keyword, "USE")) {
        return parse_use();
    }
    if (match(TokenType::Keyword, "INSERT")) {
        return parse_insert();
    }
    if (match(TokenType::Keyword, "SELECT")) {
        return parse_select();
    }

    throw ParserError("unknown statement starting with: " + peek().value);
}

ast::Statement Parser::parse_create() {
    if (match(TokenType::Keyword, "DATABASE")) {
        std::string db_name = advance().value;

        return ast::CreateDatabaseStmt{db_name};
    }

    if (match(TokenType::Keyword, "TABLE")) {
        ast::CreateTableStmt stmt;
        stmt.table_name = advance().value;
        consume(TokenType::Symbol, "(", "expected '(' after table name");

        do {
            core::Column col;
            col.name = advance().value;

            std::string type_str = advance().value;
            col.type = (type_str == "INT") ? core::DataType::INT : core::DataType::STRING;

            while (peek().type == TokenType::Keyword &&
                  (peek().value == "NOT_NULL" || peek().value == "INDEXED" || peek().value == "DEFAULT")) {
                std::string mod = advance().value;
                if (mod == "NOT_NULL") {
                    col.constraints.is_not_null = true;
                }
                if (mod == "INDEXED") {
                    col.constraints.is_indexed = true;
                    col.constraints.is_not_null = true;
                }
                if (mod == "DEFAULT") {
                    col.constraints.has_default = true;
                    if (peek().type == TokenType::Integer) {
                        col.constraints.default_value = std::stoi(advance().value);
                    }
                    else if (peek().type == TokenType::String) {
                        col.constraints.default_value = advance().value;
                    }
                }
            }
            stmt.columns.push_back(col);
        } while (match(TokenType::Symbol, ","));

        consume(TokenType::Symbol, ")", "expected ')' after columns");

        return stmt;
    }
    throw ParserError("unexpected CREATE target");
}

ast::Statement Parser::parse_drop() {
    if (match(TokenType::Keyword, "DATABASE")) {
        return ast::DropDatabaseStmt{advance().value};
    }
    if (match(TokenType::Keyword, "TABLE")) {
        return ast::DropTableStmt{advance().value};
    }
    throw ParserError("Unexpected DROP target.");
}

ast::Statement Parser::parse_use() {
    return ast::UseDatabaseStmt{advance().value};
}

ast::Statement Parser::parse_insert() {
    consume(TokenType::Keyword, "INTO", "expected INTO after INSERT");
    ast::InsertStmt stmt;
    stmt.table_name = advance().value;

    consume(TokenType::Symbol, "(", "expected '(' for columns");
    do {
        stmt.columns.push_back(advance().value);
    }
    while (match(TokenType::Symbol, ","));
    consume(TokenType::Symbol, ")", "expected ')' after columns");

    consume(TokenType::Keyword, "VALUE", "expected VALUE");
    do {
        consume(TokenType::Symbol, "(", "expected '(' for values");
        std::vector<core::Value> row;
        do {
            if (peek().type == TokenType::Integer) {
                row.push_back(std::stoi(advance().value));
            }
            else if (peek().type == TokenType::String) {
                row.push_back(advance().value);
            }
            else {
                throw ParserError("expected literal value");
            }
        }
        while (match(TokenType::Symbol, ","));
        consume(TokenType::Symbol, ")", "expected ')' after values");
        stmt.values_list.push_back(row);
    }
    while (match(TokenType::Symbol, ","));

    return stmt;
}

ast::Statement Parser::parse_select() {
    ast::SelectStmt stmt;

    if (match(TokenType::Symbol, "*")) {
        stmt.targets.push_back({"*", std::nullopt});
    }
    else {
        do {
            ast::SelectTarget target;
            target.column_name = advance().value;
            if (match(TokenType::Keyword, "AS")) {
                target.alias = advance().value;
            }
            stmt.targets.push_back(target);
        }
        while (match(TokenType::Symbol, ","));
    }

    consume(TokenType::Keyword, "FROM", "expected FROM");
    stmt.table_name = advance().value;

    if (match(TokenType::Keyword, "WHERE")) {
        stmt.where_clause = parse_expression();
    }

    return stmt;
}

ast::Operator Parser::parse_operator() {
    std::string op = advance().value;
    if (op == "==") {
        return ast::Operator::Eq;
    }
    if (op == "!=") {
        return ast::Operator::Neq;
    }
    if (op == "<") {
        return ast::Operator::Lt;
    }
    if (op == ">") {
        return ast::Operator::Gt;
    }
    if (op == "<=") {
        return ast::Operator::Lte;
    }
    if (op == ">=") {
        return ast::Operator::Gte;
    }
    throw ParserError("unknown operator: " + op);
}

std::unique_ptr<ast::Expr> Parser::parse_expression() {
    auto left = std::make_unique<ast::ColumnRefExpr>(advance().value);
    ast::Operator op = parse_operator();

    std::unique_ptr<ast::Expr> right;
    if (peek().type == TokenType::Integer) {
        right = std::make_unique<ast::LiteralExpr>(std::stoi(advance().value));
    }
    else if (peek().type == TokenType::String) {
        right = std::make_unique<ast::LiteralExpr>(advance().value);
    }
    else {
        throw ParserError("expected literal on right side of condition");
    }

    return std::make_unique<ast::BinaryExpr>(std::move(left), op, std::move(right));
}

} // namespace mimiru::query
