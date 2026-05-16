#include "mimiru/engine/system_manager.hpp"
#include <fstream>

namespace mimiru::engine {

SystemManager::SystemManager(std::filesystem::path root_path)
    : root_path_(std::move(root_path)) {
    ensure_root_exists();
}

void SystemManager::ensure_root_exists() {
    if (!std::filesystem::exists(root_path_)) {
        std::filesystem::create_directories(root_path_);
    }
}

void SystemManager::create_database(const std::string& name) {
    auto db_path = root_path_ / name;
    if (std::filesystem::exists(db_path)) {
        throw core::DbError("database already exists: " + name);
    }
    std::filesystem::create_directory(db_path);
}

void SystemManager::drop_database(const std::string& name) {
    auto db_path = root_path_ / name;
    if (current_db_ && current_db_->get_name() == name) {
        current_db_.reset();
    }
    std::filesystem::remove_all(db_path);
}

void SystemManager::use_database(const std::string& name) {
    auto db_path = root_path_ / name;
    if (!std::filesystem::exists(db_path)) {
        throw core::DbError("database does not exist: " + name);
    }
    current_db_ = std::make_unique<Database>(db_path);
}

Database* SystemManager::get_current_db() {
    if (!current_db_) {
        throw core::DbError("no database selected");
    }
    return current_db_.get();
}

} // namespace mimiru::engine
