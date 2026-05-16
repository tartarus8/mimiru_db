#pragma once

#include "mimiru/core/types.hpp"
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>

namespace mimiru::storage {
class Pager {
public:
    explicit Pager(const std::string& db_file);

    ~Pager();

    Pager(const Pager&) = delete;
    Pager& operator=(const Pager&) = delete;

    uint32_t get_num_pages() const;

    void read_page(core::PageId page_id, char* destination);
    void write_page(core::PageId page_id, const char* source);

    core::PageId allocate_page();

    void sync();

private:
    std::fstream file_;
    std::string filename_;
    uint32_t num_pages_;
};

} // namespace mimiru::storage
