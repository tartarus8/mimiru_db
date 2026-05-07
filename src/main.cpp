// minimal working product, subject to change
#include "core/schema.hpp"
#include "storage/file_storage.hpp"
#include "engine/table.hpp"
#include <iostream>
#include <memory>

using namespace dbms;

int main() {
    try {
        // creating schema
        core::Schema schema;
        schema.addColumn({"id", core::DataType::Integer, true, true, std::nullopt}); // INT, NOT NULL, INDEXED
        schema.addColumn({"username", core::DataType::String, true, false, std::nullopt}); // STRING, NOT NULL
        schema.addColumn({"age", core::DataType::Integer, false, false, std::nullopt}); // INT, NULLABLE

        // initializing storage, for now in memory
        auto storage = std::make_unique<storage::MemoryStorage>();
        engine::Table users_table("users", std::move(schema), std::move(storage));

        std::cout << "Table '" << users_table.getName() << "' created successfully!\n";

        // testing basic insert
        users_table.insert({ 1, std::string("Alice"), 25 });
        users_table.insert({ 2, std::string("Bob"), std::nullopt }); // age = NULL - ОК
        std::cout << "Valid records inserted.\n";

        // testing null insert
        try {
            users_table.insert({ 3, std::nullopt, 30 });
        } catch (const std::exception& e) {
            std::cout << "[Expected Error Caught]: " << e.what() << "\n";
        }

        // testing wrong type insert
        try {
            users_table.insert({ 4, std::string("Charlie"), std::string("Thirty") });
        } catch (const std::exception& e) {
            std::cout << "[Expected Error Caught]: " << e.what() << "\n";
        }

        // result
        auto records = users_table.selectAll();
        std::cout << "\n--- Table Contents ---\n";
        for (const auto& rec : records) {
            std::cout << "Record ID: " << rec.id << " | ";
            for (const auto& field : rec.fields) {
                if (field.has_value()) {
                    if (std::holds_alternative<int>(field.value()))
                        std::cout << std::get<int>(field.value()) << "\t";
                    else
                        std::cout << std::get<std::string>(field.value()) << "\t";
                } else {
                    std::cout << "NULL\t";
                }
            }
            std::cout << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "fatal Error: " << e.what() << "\n";
    }

    return 0;
}
