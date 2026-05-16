#pragma once

#include "mimiru/engine/database.hpp"
#include <filesystem>
#include <optional>

namespace mimiru::engine {

class SystemManager {
public:
    explicit SystemManager(std::filesystem::path root_path);

    void create_database(const std::string& name);
    void drop_database(const std::string& name);
    void use_database(const std::string& name);

    Database* get_current_db();
    bool has_database(const std::string& name) const;

private:
    std::filesystem::path root_path_;
    std::unique_ptr<Database> current_db_;
    void ensure_root_exists();
};

} // namespace mimiru::engine
