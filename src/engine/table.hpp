#pragma once
#include "../core/schema.hpp"
#include "../storage/i_storage.hpp"
#include <memory>
#include <string>
#include <stdexcept>
#include <iostream>

namespace dbms::engine {

    class Table {
    public:
        Table(std::string name, core::Schema schema, std::unique_ptr<storage::IStorageManager> storage)
            : name_(std::move(name)), schema_(std::move(schema)), storage_(std::move(storage)) {}

        void insert(std::vector<core::NullableValue> values) {
            if (values.size() != schema_.getColumnCount()) {
                throw std::invalid_argument("insert error: values count doesn't match columns count");
            }

            const auto& columns = schema_.getColumns();
            for (size_t i = 0; i < columns.size(); ++i) {
                const auto& col = columns[i];
                if (col.is_not_null && !values[i].has_value()) {
                    if (col.default_value.has_value()) {
                        values[i] = col.default_value;
                    } else {
                        throw std::invalid_argument("constraint violation: Column '" + col.name + "' cannot be NULL");
                    }
                }

                if (values[i].has_value()) {
                    bool is_int = std::holds_alternative<int>(values[i].value());
                    bool is_string = std::holds_alternative<std::string>(values[i].value());

                    if (col.type == core::DataType::Integer && !is_int) {
                        throw std::invalid_argument("type mismatch: Column '" + col.name + "' expects Integer");
                    }
                    if (col.type == core::DataType::String && !is_string) {
                        throw std::invalid_argument("type mismatch: Column '" + col.name + "' expects String");
                    }
                }
            }

            storage_->insertRecord(values);
        }

        std::vector<core::Record> selectAll() {
            return storage_->scanAll();
        }

        const std::string& getName() const { return name_; }

    private:
        std::string name_;
        core::Schema schema_;
        std::unique_ptr<storage::IStorageManager> storage_;
    };

}
