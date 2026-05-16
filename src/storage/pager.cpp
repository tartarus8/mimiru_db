#include "mimiru/storage/pager.hpp"

namespace mimiru::storage {

Pager::Pager(const std::string& db_file) : filename_(db_file), num_pages_(0) {
    if (!std::filesystem::exists(filename_)) {
        std::ofstream new_file(filename_, std::ios::out | std::ios::binary);
        new_file.close();
    }

    file_.open(filename_, std::ios::in | std::ios::out | std::ios::binary);

    if (!file_.is_open()) {
        throw std::runtime_error("mimiru::storage::pager - failed to open database file: " + filename_);
    }

    auto file_size = std::filesystem::file_size(filename_);
    num_pages_ = static_cast<uint32_t>(file_size / core::PAGE_SIZE);
}

Pager::~Pager() {
    if (file_.is_open()) {
        sync();
        file_.close();
    }
}

uint32_t Pager::get_num_pages() const {
    return num_pages_;
}

void Pager::read_page(core::PageId page_id, char* destination) {
    if (page_id >= num_pages_) {
        throw std::out_of_range("mimiru::storage::pager - cannot read past end of file");
    }

    file_.seekg(page_id * core::PAGE_SIZE, std::ios::beg);
    file_.read(destination, core::PAGE_SIZE);

    if (file_.fail()) {
        throw std::runtime_error("mimiru::storage::pager - I/O error during read");
    }

    file_.clear();
}

void Pager::write_page(core::PageId page_id, const char* source) {
    if (page_id > num_pages_) {
        throw std::out_of_range("mimiru::storage::pager - cannot write to unallocated page");
    }

    file_.seekp(page_id * core::PAGE_SIZE, std::ios::beg);
    file_.write(source, core::PAGE_SIZE);

    if (file_.fail()) {
        throw std::runtime_error("mimiru::storage::pager - I/O error during write");
    }

    if (page_id == num_pages_) {
        num_pages_++;
    }
}

core::PageId Pager::allocate_page() {
    core::PageId new_page_id = num_pages_;

    char empty_page[core::PAGE_SIZE] = {0};
    write_page(new_page_id, empty_page);

    return new_page_id;
}

void Pager::sync() {
    if (file_.is_open()) {
        file_.flush();
    }
}

} // namespace mimiru::storage
