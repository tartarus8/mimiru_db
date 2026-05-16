#include "mimiru/index/bplus_tree.hpp"
#include <cstring>
#include <stdexcept>

namespace mimiru::index {
BPlusTreeInt::BPlusTreeInt(storage::Pager& pager, core::PageId root_id)
    : pager_(pager), root_id_(root_id) {

    if (root_id_ == core::INVALID_PAGE_ID || root_id_ >= pager_.get_num_pages()) {
        root_id_ = pager_.allocate_page();

        char buffer[core::PAGE_SIZE] = {0};
        auto* node = reinterpret_cast<LeafNode*>(buffer);
        node->header.type = NodeType::LEAF;
        node->header.is_root = 1;
        node->header.parent_page_id = core::INVALID_PAGE_ID;
        node->header.count = 0;
        node->next_page_id = core::INVALID_PAGE_ID;
        node->prev_page_id = core::INVALID_PAGE_ID;

        pager_.write_page(root_id_, buffer);
    }
}

core::PageId BPlusTreeInt::find_leaf(int key) {
    core::PageId current_id = root_id_;
    char buffer[core::PAGE_SIZE];

    while (true) {
        pager_.read_page(current_id, buffer);
        auto* header = reinterpret_cast<NodeHeader*>(buffer);

        if (header->type == NodeType::LEAF) {
            return current_id;
        }

        auto* internal = reinterpret_cast<InternalNode*>(buffer);
        uint32_t i = 0;
        while (i < header->count && key >= internal->keys[i]) {
            i++;
        }
        current_id = internal->children[i];
    }
}

std::optional<uint32_t> BPlusTreeInt::find(int key) {
    core::PageId leaf_id = find_leaf(key);
    char buffer[core::PAGE_SIZE];
    pager_.read_page(leaf_id, buffer);

    auto* leaf = reinterpret_cast<LeafNode*>(buffer);
    for (uint32_t i = 0; i < leaf->header.count; ++i) {
        if (leaf->keys[i] == key) {
            return leaf->record_ids[i];
        }
    }

    return std::nullopt;
}

void BPlusTreeInt::insert(int key, uint32_t record_id) {
    core::PageId leaf_id = find_leaf(key);
    char buffer[core::PAGE_SIZE];
    pager_.read_page(leaf_id, buffer);
    auto* leaf = reinterpret_cast<LeafNode*>(buffer);

    uint32_t idx = 0;
    while (idx < leaf->header.count && leaf->keys[idx] < key) {
        idx++;
    }

    if (idx < leaf->header.count && leaf->keys[idx] == key) {
        leaf->record_ids[idx] = record_id;
        pager_.write_page(leaf_id, buffer);

        return;
    }

    for (uint32_t i = leaf->header.count; i > idx; --i) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->record_ids[i] = leaf->record_ids[i - 1];
    }
    leaf->keys[idx] = key;
    leaf->record_ids[idx] = record_id;
    leaf->header.count++;

    if (leaf->header.count > MAX_ENTRIES) {
        split_leaf(leaf_id, leaf);
    }
    else {
        pager_.write_page(leaf_id, buffer);
    }
}

void BPlusTreeInt::split_leaf(core::PageId leaf_id, LeafNode* leaf) {
    core::PageId new_leaf_id = pager_.allocate_page();
    char new_buffer[core::PAGE_SIZE] = {0};
    auto* new_leaf = reinterpret_cast<LeafNode*>(new_buffer);

    new_leaf->header.type = NodeType::LEAF;
    new_leaf->header.is_root = 0;
    new_leaf->header.parent_page_id = leaf->header.parent_page_id;

    uint32_t split_idx = leaf->header.count / 2;
    uint32_t move_count = leaf->header.count - split_idx;

    for (uint32_t i = 0; i < move_count; ++i) {
        new_leaf->keys[i] = leaf->keys[split_idx + i];
        new_leaf->record_ids[i] = leaf->record_ids[split_idx + i];
    }

    leaf->header.count = split_idx;
    new_leaf->header.count = move_count;

    new_leaf->next_page_id = leaf->next_page_id;
    new_leaf->prev_page_id = leaf_id;
    leaf->next_page_id = new_leaf_id;

    int up_key = new_leaf->keys[0];

    pager_.write_page(new_leaf_id, new_buffer);
    pager_.write_page(leaf_id, reinterpret_cast<char*>(leaf));

    if (new_leaf->next_page_id != core::INVALID_PAGE_ID) {
        char next_buffer[core::PAGE_SIZE];
        pager_.read_page(new_leaf->next_page_id, next_buffer);
        auto* next_node = reinterpret_cast<LeafNode*>(next_buffer);
        next_node->prev_page_id = new_leaf_id;
        pager_.write_page(new_leaf->next_page_id, next_buffer);
    }

    insert_into_parent(leaf_id, up_key, new_leaf_id);
}

void BPlusTreeInt::insert_into_parent(core::PageId left_id, int key, core::PageId right_id) {
    char left_buffer[core::PAGE_SIZE];
    pager_.read_page(left_id, left_buffer);
    auto* left_node = reinterpret_cast<NodeHeader*>(left_buffer);

    if (left_node->parent_page_id == core::INVALID_PAGE_ID) {
        core::PageId new_root_id = pager_.allocate_page();
        char root_buffer[core::PAGE_SIZE] = {0};
        auto* new_root = reinterpret_cast<InternalNode*>(root_buffer);

        new_root->header.type = NodeType::INTERNAL;
        new_root->header.is_root = 1;
        new_root->header.parent_page_id = core::INVALID_PAGE_ID;
        new_root->header.count = 1;

        new_root->keys[0] = key;
        new_root->children[0] = left_id;
        new_root->children[1] = right_id;

        left_node->parent_page_id = new_root_id;
        left_node->is_root = 0;

        char right_buffer[core::PAGE_SIZE];
        pager_.read_page(right_id, right_buffer);
        auto* right_node = reinterpret_cast<NodeHeader*>(right_buffer);
        right_node->parent_page_id = new_root_id;
        right_node->is_root = 0;

        pager_.write_page(left_id, left_buffer);
        pager_.write_page(right_id, right_buffer);
        pager_.write_page(new_root_id, root_buffer);

        root_id_ = new_root_id;

        return;
    }

    core::PageId parent_id = left_node->parent_page_id;
    char parent_buffer[core::PAGE_SIZE];
    pager_.read_page(parent_id, parent_buffer);
    auto* parent = reinterpret_cast<InternalNode*>(parent_buffer);

    uint32_t idx = 0;
    while (idx < parent->header.count && parent->keys[idx] < key) {
        idx++;
    }

    for (uint32_t i = parent->header.count; i > idx; --i) {
        parent->keys[i] = parent->keys[i - 1];
        parent->children[i + 1] = parent->children[i];
    }

    parent->keys[idx] = key;
    parent->children[idx + 1] = right_id;
    parent->header.count++;

    if (parent->header.count > MAX_ENTRIES) {
        split_internal(parent_id, parent);
    }
    else {
        pager_.write_page(parent_id, parent_buffer);
    }
}

void BPlusTreeInt::split_internal(core::PageId internal_id, InternalNode* internal) {
    core::PageId new_internal_id = pager_.allocate_page();
    char new_buffer[core::PAGE_SIZE] = {0};
    auto* new_internal = reinterpret_cast<InternalNode*>(new_buffer);

    new_internal->header.type = NodeType::INTERNAL;
    new_internal->header.is_root = 0;
    new_internal->header.parent_page_id = internal->header.parent_page_id;

    uint32_t split_idx = internal->header.count / 2;
    int up_key = internal->keys[split_idx];

    uint32_t move_count = internal->header.count - split_idx - 1;

    for (uint32_t i = 0; i < move_count; ++i) {
        new_internal->keys[i] = internal->keys[split_idx + 1 + i];
        new_internal->children[i] = internal->children[split_idx + 1 + i];
    }
    new_internal->children[move_count] = internal->children[internal->header.count];

    internal->header.count = split_idx;
    new_internal->header.count = move_count;

    pager_.write_page(internal_id, reinterpret_cast<char*>(internal));
    pager_.write_page(new_internal_id, new_buffer);

    for (uint32_t i = 0; i <= new_internal->header.count; ++i) {
        char child_buffer[core::PAGE_SIZE];
        pager_.read_page(new_internal->children[i], child_buffer);
        auto* child_header = reinterpret_cast<NodeHeader*>(child_buffer);
        child_header->parent_page_id = new_internal_id;
        pager_.write_page(new_internal->children[i], child_buffer);
    }

    insert_into_parent(internal_id, up_key, new_internal_id);
}

bool BPlusTreeInt::remove(int key) {
    core::PageId leaf_id = find_leaf(key);
    char buffer[core::PAGE_SIZE];
    pager_.read_page(leaf_id, buffer);
    auto* leaf = reinterpret_cast<LeafNode*>(buffer);

    for (uint32_t i = 0; i < leaf->header.count; ++i) {
        if (leaf->keys[i] == key) {
            for (uint32_t j = i; j < leaf->header.count - 1; ++j) {
                leaf->keys[j] = leaf->keys[j + 1];
                leaf->record_ids[j] = leaf->record_ids[j + 1];
            }
            leaf->header.count--;
            pager_.write_page(leaf_id, buffer);

            return true;
        }
    }

    return false;
}

std::vector<std::pair<int, uint32_t>> BPlusTreeInt::range(int left, int right) {
    std::vector<std::pair<int, uint32_t>> result;
    if (left >= right) return result;

    core::PageId current_id = find_leaf(left);
    char buffer[core::PAGE_SIZE];

    while (current_id != core::INVALID_PAGE_ID) {
        pager_.read_page(current_id, buffer);
        auto* leaf = reinterpret_cast<LeafNode*>(buffer);

        for (uint32_t i = 0; i < leaf->header.count; ++i) {
            if (leaf->keys[i] >= right) {
                return result;
            }
            if (leaf->keys[i] >= left) {
                result.emplace_back(leaf->keys[i], leaf->record_ids[i]);
            }
        }
        current_id = leaf->next_page_id;
    }

    return result;
}

} // namespace mimiru::index
