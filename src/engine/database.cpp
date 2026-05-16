#include "mimiru/engine/database.hpp"
#include <fstream>

namespace mimiru::engine {
Database::Database(std::filesystem::path db_path) : db_path_(std::move(db_path)) {
    load_metadata();
}

void Database::create_table(const core::TableSchema& schema) {
    if (has_table(schema.name)) {
        throw core::DbError("table already exists: " + schema.name);
    }

    auto table_path = db_path_ / (schema.name + ".db");

    auto pager = std::make_unique<storage::Pager>(table_path.string());

    auto table = std::make_unique<Table>(schema, std::move(pager));

    tables_[schema.name] = std::move(table);
    save_metadata();
}

void Database::drop_table(const std::string& table_name) {
    if (!has_table(table_name)) {
        throw core::DbError("table not found: " + table_name);
    }

    tables_.erase(table_name);
    auto table_path = db_path_ / (table_name + ".db");
    std::filesystem::remove(table_path);
    save_metadata();
}

Table* Database::get_table(const std::string& name) {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        throw core::DbError("table not found: " + name);
    }

    return it->second.get();
}

bool Database::has_table(const std::string& name) const {
    return tables_.find(name) != tables_.end();
}

void Database::save_metadata() {
    // save to json here
}

void Database::load_metadata() {
    // reading for .db files here
}

} // namespace mimiru::engine
