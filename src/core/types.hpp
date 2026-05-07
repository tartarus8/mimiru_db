#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <vector>

namespace dbms::core {

    enum class DataType {
        Integer,
        String
    };

    using Value = std::variant<int, std::string>;

    using NullableValue = std::optional<Value>;

    // unique record id and record struct
    using RecordId = uint64_t;

    struct Record {
        RecordId id;
        std::vector<NullableValue> fields;
    };


    inline std::string to_string(DataType type) {
        switch (type) {
            case DataType::Integer: return "Integer";
            case DataType::String:  return "String";
            default:                return "Unknown";
        }
    }

} // namespace dbms::core
