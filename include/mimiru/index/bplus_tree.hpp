#pragma once

#include "mimiru/storage/pager.hpp"
#include "mimiru/core/types.hpp"
#include <optional>
#include <vector>

namespace mimiru::index {
class BPlusTreeInt {
public:
    BPlusTreeInt(storage::Pager& pager, mimiru::core::PageId root_page_id);

    void insert(int key, uint32_t record_id);
    std::optional<uint32_t> find(int key);

    bool remove(int key);

    mimiru::core::PageId get_root_id() const {
        return root_id_;
    }

    std::vector<std::pair<int, uint32_t>> range(int left, int right);

private:
    storage::Pager& pager_;
    mimiru::core::PageId root_id_;

    // sized to MAX_ENTRIES + 1 to allow in-memory overflow before splitting
    struct InternalNode {
        NodeHeader header;
        int keys[MAX_ENTRIES + 1];
        mimiru::core::PageId children[MAX_ENTRIES + 2];
    };

    struct LeafNode {
        NodeHeader header;
        mimiru::core::PageId next_page_id;
        mimiru::core::PageId prev_page_id;
        int keys[MAX_ENTRIES + 1];
        uint32_t record_ids[MAX_ENTRIES + 1];
    };

    mimiru::core::PageId find_leaf(int key);

    void insert_into_parent(mimiru::core::PageId left_id, int key, mimiru::core::PageId right_id);
    void split_leaf(mimiru::core::PageId leaf_id, LeafNode* leaf);
    void split_internal(mimiru::core::PageId internal_id, InternalNode* internal);
};

} // namespace mimiru::index
