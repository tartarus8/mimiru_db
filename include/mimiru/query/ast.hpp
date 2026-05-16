#pragma once

#include "mimiru/core/types.hpp"
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <optional>

namespace mimiru::query::ast {

enum class Operator {
    Eq,
    Neq,
    Lt,
    Gt,
    Lte,
    Gte
};

struct Expr {
    virtual ~Expr() = default;
};

struct LiteralExpr : Expr {
    core::Value value;
    explicit LiteralExpr(core::Value v) : value(std::move(v)) {}
};

struct ColumnRefExpr : Expr {
    std::string column_name;
    explicit ColumnRefExpr(std::string name) : column_name(std::move(name)) {}
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    Operator op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> l, Operator o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

struct CreateDatabaseStmt {
    std::string db_name;
};
struct DropDatabaseStmt {
    std::string db_name;
};
struct UseDatabaseStmt {
    std::string db_name;
};

struct CreateTableStmt {
    std::string table_name;
    std::vector<core::Column> columns;
};
struct DropTableStmt {
    std::string table_name;
};

struct InsertStmt {
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::vector<core::Value>> values_list;
};

struct SelectTarget {
    std::string column_name;
    std::optional<std::string> alias;
};

struct SelectStmt {
    std::vector<SelectTarget> targets;
    std::string table_name;
    std::unique_ptr<Expr> where_clause;
};

using Statement = std::variant<
    CreateDatabaseStmt, DropDatabaseStmt, UseDatabaseStmt,
    CreateTableStmt, DropTableStmt, InsertStmt, SelectStmt
>;

} // namespace mimiru::query::ast
