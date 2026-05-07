#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <stdexcept>

namespace dbms::core {

    // one table column definition and table (Schema) class
    struct ColumnDef {
        std::string name;
        DataType type;

        bool is_not_null = false;
        bool is_indexed = false;

        NullableValue default_value = std::nullopt;
    };

    class Schema {
    public:
        Schema() = default;

        void addColumn(const ColumnDef& column);

        const std::vector<ColumnDef>& getColumns() const;

        const ColumnDef& getColumn(size_t index) const;

        int getColumnIndex(const std::string& name) const;

        size_t getColumnCount() const;

    private:
        std::vector<ColumnDef> columns_;
    };

} // namespace dbms::core
