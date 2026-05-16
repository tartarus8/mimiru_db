#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace mimiru::core {
constexpr size_t PAGE_SIZE = 4096;
using PageId = uint32_t;
constexpr PageId INVALID_PAGE_ID = 0xFFFFFFFF;

// strings are never stored in B-Tree index pages or raw table slots
// they are stored in a centralized dictionary and referenced by ID instead
using StringId = uint32_t;

#pragma pack(push, 1)

// represents the exact physical location of a record in the database file
struct RecordId {
    PageId page_id;
    uint32_t slot_id;

    bool operator==(const RecordId& other) const {
        return page_id == other.page_id && slot_id == other.slot_id;
    }
};

#pragma pack(pop)

enum class DataType : uint8_t {
    INT = 0,
    STRING = 1
};

using Value = std::variant<int32_t, std::string>;

using NullableValue = std::optional<Value>;

struct Constraints {
    bool is_not_null = false;
    bool is_indexed = false;
    bool has_default = false;
    Value default_value = 0;
};

struct Column {
    std::string name;
    DataType type;
    Constraints constraints;
};

struct TableSchema {
    std::string name;
    std::vector<Column> columns;
    PageId root_page_id = INVALID_PAGE_ID;
    PageId index_page_id = INVALID_PAGE_ID;

    int get_column_index(const std::string& col_name) const {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == col_name) {
                return static_cast<int>(i);
            }
        }

        return -1;
    }
};

struct Record {
    RecordId id;
    std::vector<NullableValue> fields;

    nlohmann::json to_json(const TableSchema& schema) const {
        nlohmann::json j;
        for (size_t i = 0; i < fields.size(); ++i) {
            const auto& col_name = schema.columns[i].name;
            if (!fields[i].has_value()) {
                j[col_name] = nullptr;
            }
            else {
                std::visit([&](auto&& arg) {
                    j[col_name] = arg;
                }, *fields[i]);
            }
        }

        return j;
    }
};

class DbError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ConstraintViolation : public DbError {
public:
    using DbError::DbError;
};

inline std::string to_string(DataType type) {
    return (type == DataType::INT) ? "INT" : "STRING";
}

} // namespace mimiru::core

namespace mimiru::index {

constexpr size_t MAX_ENTRIES = 250;

enum class NodeType : uint8_t {
    INTERNAL = 0,
    LEAF = 1
};

#pragma pack(push, 1)

struct NodeHeader {
    NodeType type;
    uint8_t is_root;
    mimiru::core::PageId parent_page_id;
    uint32_t count;
}; // exactly 10 bytes

#pragma pack(pop)

} // namespace mimiru::index
