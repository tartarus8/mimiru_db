#include <gtest/gtest.h>
#include "mimiru/query/parser.hpp"

using namespace mimiru::query;

TEST(ParserTest, ParseCreateDatabase) {
    Lexer lexer("CREATE DATABASE shop;");
    Parser parser(lexer.tokenize());
    auto statements = parser.parse();

    ASSERT_EQ(statements.size(), 1);

    EXPECT_TRUE(std::holds_alternative<ast::CreateDatabaseStmt>(statements[0]));
    auto stmt = std::get<ast::CreateDatabaseStmt>(statements[0]);
    EXPECT_EQ(stmt.db_name, "shop");
}

TEST(ParserTest, ParseCreateTable) {
    Lexer lexer("CREATE TABLE users (id INT INDEXED, name STRING NOT_NULL, age INT DEFAULT 18);");
    Parser parser(lexer.tokenize());
    auto statements = parser.parse();

    ASSERT_EQ(statements.size(), 1);
    auto stmt = std::get<ast::CreateTableStmt>(statements[0]);

    EXPECT_EQ(stmt.table_name, "users");
    ASSERT_EQ(stmt.columns.size(), 3);

    EXPECT_EQ(stmt.columns[0].name, "id");
    EXPECT_TRUE(stmt.columns[0].constraints.is_indexed);

    EXPECT_EQ(stmt.columns[1].name, "name");
    EXPECT_TRUE(stmt.columns[1].constraints.is_not_null);

    EXPECT_EQ(stmt.columns[2].name, "age");
    EXPECT_TRUE(stmt.columns[2].constraints.has_default);
    EXPECT_EQ(std::get<int32_t>(stmt.columns[2].constraints.default_value), 18);
}

TEST(ParserTest, ParseInsert) {
    Lexer lexer("INSERT INTO users (id, name) VALUE (1, \"Alice\"), (2, \"Bob\");");
    Parser parser(lexer.tokenize());
    auto statements = parser.parse();

    auto stmt = std::get<ast::InsertStmt>(statements[0]);
    EXPECT_EQ(stmt.table_name, "users");
    ASSERT_EQ(stmt.columns.size(), 2);
    ASSERT_EQ(stmt.values_list.size(), 2);

    EXPECT_EQ(std::get<int32_t>(stmt.values_list[0][0]), 1);
    EXPECT_EQ(std::get<std::string>(stmt.values_list[0][1]), "Alice");
}

TEST(ParserTest, ParseSelectWhere) {
    Lexer lexer("SELECT id, name AS client_name FROM users WHERE age >= 18;");
    Parser parser(lexer.tokenize());
    auto statements = parser.parse();

    auto& stmt = std::get<ast::SelectStmt>(statements[0]);

    EXPECT_EQ(stmt.table_name, "users");
    ASSERT_EQ(stmt.targets.size(), 2);
    EXPECT_EQ(stmt.targets[0].column_name, "id");
    EXPECT_EQ(stmt.targets[1].alias.value(), "client_name");

    ASSERT_NE(stmt.where_clause, nullptr);
    auto* binary_expr = dynamic_cast<ast::BinaryExpr*>(stmt.where_clause.get());
    ASSERT_NE(binary_expr, nullptr);
    EXPECT_EQ(binary_expr->op, ast::Operator::Gte);
}
