#include "schema.hpp"

namespace dbms::core {

    void Schema::addColumn(const ColumnDef& column) {
        if (getColumnIndex(column.name) != -1) {
            throw std::invalid_argument("column '" + column.name + "' already exists in the schema");
        }

        ColumnDef final_column = column;
        if (final_column.is_indexed) {
            final_column.is_not_null = true;
        }

        columns_.push_back(std::move(final_column));
    }

    const std::vector<ColumnDef>& Schema::getColumns() const {
        return columns_;
    }

    const ColumnDef& Schema::getColumn(size_t index) const {
        if (index >= columns_.size()) {
            throw std::out_of_range("column index is out of bounds");
        }
        return columns_[index];
    }

    int Schema::getColumnIndex(const std::string& name) const {
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (columns_[i].name == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    size_t Schema::getColumnCount() const {
        return columns_.size();
    }

} // namespace dbms::core
