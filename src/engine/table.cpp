#include "mimiru/engine/table.hpp"
#include <cstring>

namespace mimiru::engine {

#pragma pack(push, 1)
struct TableSuperblock {
    uint32_t magic_number;
    uint32_t first_data_page;
    uint32_t last_data_page;
    uint32_t index_root_page;
};

struct PageHeader {
    uint32_t record_count;
    uint32_t next_page;
    uint32_t free_space_offset;
};
#pragma pack(pop)

constexpr uint32_t MIMIRU_MAGIC = 0x4D494D55; // MIMU

Table::Table(core::TableSchema schema, std::unique_ptr<storage::Pager> pager)
    : schema_(schema), pager_(std::move(pager)) {

    char buffer[core::PAGE_SIZE] = {0};

    if (pager_->get_num_pages() == 0) {
        pager_->allocate_page();
        auto* sb = reinterpret_cast<TableSuperblock*>(buffer);
        sb->magic_number = MIMIRU_MAGIC;
        sb->first_data_page = core::INVALID_PAGE_ID;
        sb->last_data_page = core::INVALID_PAGE_ID;
        sb->index_root_page = core::INVALID_PAGE_ID;
        pager_->write_page(0, buffer);
    }

    pager_->read_page(0, buffer);
    auto* sb = reinterpret_cast<TableSuperblock*>(buffer);

    for (uint32_t i = 0; i < schema_.columns.size(); ++i) {
        if (schema_.columns[i].constraints.is_indexed) {
            indices_[i] = std::make_unique<index::BPlusTreeInt>(*pager_, sb->index_root_page);
            if (sb->index_root_page != indices_[i]->get_root_id()) {
                sb->index_root_page = indices_[i]->get_root_id();
                pager_->write_page(0, buffer);
            }
        }
    }
}

core::RecordId Table::insert(const std::vector<core::NullableValue>& values) {
    validate_constraints(values);

    char sb_buffer[core::PAGE_SIZE];
    pager_->read_page(0, sb_buffer);
    auto* sb = reinterpret_cast<TableSuperblock*>(sb_buffer);

    if (sb->first_data_page == core::INVALID_PAGE_ID) {
        core::PageId new_pid = pager_->allocate_page();
        sb->first_data_page = new_pid;
        sb->last_data_page = new_pid;
        pager_->write_page(0, sb_buffer);

        char new_page[core::PAGE_SIZE] = {0};
        auto* header = reinterpret_cast<PageHeader*>(new_page);
        header->free_space_offset = sizeof(PageHeader);
        header->record_count = 0;
        header->next_page = core::INVALID_PAGE_ID;
        pager_->write_page(new_pid, new_page);
    }

    core::PageId target_pid = sb->last_data_page;
    char page_buffer[core::PAGE_SIZE];
    pager_->read_page(target_pid, page_buffer);
    auto* header = reinterpret_cast<PageHeader*>(page_buffer);

    size_t record_size = schema_.columns.size() * sizeof(int32_t);

    if (header->free_space_offset + record_size > core::PAGE_SIZE) {
        core::PageId new_pid = pager_->allocate_page();

        header->next_page = new_pid;
        pager_->write_page(target_pid, page_buffer);

        sb->last_data_page = new_pid;
        pager_->write_page(0, sb_buffer);

        target_pid = new_pid;
        std::memset(page_buffer, 0, core::PAGE_SIZE);
        header = reinterpret_cast<PageHeader*>(page_buffer);
        header->free_space_offset = sizeof(PageHeader);
        header->record_count = 0;
        header->next_page = core::INVALID_PAGE_ID;
    }

    char* write_ptr = page_buffer + header->free_space_offset;
    for (size_t i = 0; i < values.size(); ++i) {
        int32_t val = std::get<int32_t>(*values[i]);
        std::memcpy(write_ptr + (i * sizeof(int32_t)), &val, sizeof(int32_t));

        if (schema_.columns[i].constraints.is_indexed) {
            uint32_t packed_record_id = (target_pid << 16) | (header->record_count & 0xFFFF);
            indices_[i]->insert(val, packed_record_id);
        }
    }

    core::RecordId rid = {target_pid, header->record_count};
    header->record_count++;
    header->free_space_offset += record_size;

    pager_->write_page(target_pid, page_buffer);
    return rid;
}

void Table::validate_constraints(const std::vector<core::NullableValue>& values) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (schema_.columns[i].constraints.is_indexed) {
            int32_t val = std::get<int32_t>(*values[i]);
            if (indices_[i]->find(val).has_value()) {
                throw core::ConstraintViolation("duplicate key in INDEXED column");
            }
        }
    }
}

} // namespace mimiru::engine
