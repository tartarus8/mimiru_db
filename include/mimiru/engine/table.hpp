#pragma once

#include "mimiru/core/types.hpp"
#include "mimiru/storage/pager.hpp"
#include "mimiru/index/bplus_tree.hpp"
#include <memory>
#include <vector>

namespace mimiru::engine {
class Table {
public:
    Table(core::TableSchema schema, std::unique_ptr<storage::Pager> pager);

    core::RecordId insert(const std::vector<core::NullableValue>& values);
    std::vector<core::Record> select_all();
    std::optional<core::Record> select_by_id(core::RecordId rid);
    void delete_record(core::RecordId rid);

    const core::TableSchema& get_schema() const {
        return schema_;
    }

private:
    core::TableSchema schema_;
    std::unique_ptr<storage::Pager> pager_;

    std::unordered_map<uint32_t, std::unique_ptr<index::BPlusTreeInt>> indices_;

    core::PageId get_free_page(size_t record_size);

    void validate_constraints(const std::vector<core::NullableValue>& values);
};

} // namespace mimiru::engine
