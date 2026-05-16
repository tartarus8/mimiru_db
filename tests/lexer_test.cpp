#include <gtest/gtest.h>
#include "mimiru/query/lexer.hpp"

using namespace mimiru::query;

TEST(LexerTest, BasicTokens) {
    Lexer lexer("SELECT * FROM users;");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].type, TokenType::Keyword);
    EXPECT_EQ(tokens[0].value, "SELECT");

    EXPECT_EQ(tokens[1].type, TokenType::Symbol);
    EXPECT_EQ(tokens[1].value, "*");

    EXPECT_EQ(tokens[2].type, TokenType::Keyword);
    EXPECT_EQ(tokens[2].value, "FROM");

    EXPECT_EQ(tokens[3].type, TokenType::Identifier);
    EXPECT_EQ(tokens[3].value, "users");

    EXPECT_EQ(tokens[4].type, TokenType::Symbol);
    EXPECT_EQ(tokens[4].value, ";");
}

TEST(LexerTest, MixedCaseRejection) {
    Lexer lexer1("SeLeCt * FROM users;");
    EXPECT_THROW(lexer1.tokenize(), LexerError);

    Lexer lexer2("select * from users;");
    EXPECT_NO_THROW({
        auto t = lexer2.tokenize();
        EXPECT_EQ(t[0].value, "SELECT");
    });
}

TEST(LexerTest, StringsAndNumbers) {
    Lexer lexer("INSERT INTO table VALUE (-42, \"Hello World\");");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[5].type, TokenType::Integer);
    EXPECT_EQ(tokens[5].value, "-42");

    EXPECT_EQ(tokens[7].type, TokenType::String);
    EXPECT_EQ(tokens[7].value, "Hello World");
}

TEST(LexerTest, Operators) {
    Lexer lexer("age >= 18 != 20");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[1].type, TokenType::Operator);
    EXPECT_EQ(tokens[1].value, ">=");

    EXPECT_EQ(tokens[3].type, TokenType::Operator);
    EXPECT_EQ(tokens[3].value, "!=");
}
