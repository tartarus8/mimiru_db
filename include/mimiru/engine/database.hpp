#pragma once

#include "mimiru/engine/table.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace mimiru::engine {
class Database {
public:
    explicit Database(std::filesystem::path db_path);

    void create_table(const core::TableSchema& schema);
    void drop_table(const std::string& table_name);

    Table* get_table(const std::string& name);
    bool has_table(const std::string& name) const;
    std::string get_name() const {
        return db_path_.filename().string();
    }

private:
    std::filesystem::path db_path_;
    std::unordered_map<std::string, std::unique_ptr<Table>> tables_;

    void save_metadata();
    void load_metadata();
};

} // namespace mimiru::engine
