#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename Key, typename RecordId = int, typename Compare = std::less<Key>>
class BPlusTree {
public:
    using value_type = std::pair<Key, std::vector<RecordId>>;
    explicit BPlusTree(
        std::size_t leaf_max_keys = 32,
        std::size_t node_max_keys = 32,
        const Compare& compare = Compare()
    ) : leaf_max_keys_(leaf_max_keys), node_max_keys_(node_max_keys), compare_(compare),
        root_(), keys_count_(0), values_count_(0) {
        if (leaf_max_keys_ < 2) {
            throw std::invalid_argument("leaf_max_keys < 2");
        }
        if (node_max_keys_ < 2) {
            throw std::invalid_argument("node_max_keys < 2");
        }
    }

    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return keys_count_ == 0;
    }

    std::size_t keys_count() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return keys_count_;
    }

    std::size_t values_count() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return values_count_;
    }

    std::size_t leaf_max_keys() const {
        return leaf_max_keys_;
    }

    std::size_t node_max_keys() const {
        return node_max_keys_;
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        root_.reset();
        keys_count_ = 0;
        values_count_ = 0;
    }

    bool contains(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        const Leaf* leaf = find_leaf(key);
        if (leaf == nullptr) {
            return false;
        }
        std::size_t index = find_lower_index(leaf->keys, key);
        if (index >= leaf->keys.size()) {
            return false;
        }
        return equal(leaf->keys[index], key);
    }

    std::vector<RecordId> find(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<RecordId> found;
        const Leaf* leaf = find_leaf(key);
        if (leaf == nullptr) {
            return found;
        }
        std::size_t index = find_lower_index(leaf->keys, key);
        if (index >= leaf->keys.size()) {
            return found;
        }
        if (!equal(leaf->keys[index], key)) {
            return found;
        }
        found = leaf->values[index];
        return found;
    }

    std::vector<value_type> range(const Key& left, const Key& right) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<value_type> found;
        if (!less(left, right)) {
            return found;
        }
        const Leaf* leaf = find_leaf(left);
        if (leaf == nullptr) {
            return found;
        }
        std::size_t i = find_lower_index(leaf->keys, left);
        while (leaf != nullptr) {
            for (; i < leaf->keys.size(); ++i) {
                if (!less(leaf->keys[i], right)) {
                    return found;
                }
                found.push_back(value_type(leaf->keys[i], leaf->values[i]));
            }
            leaf = leaf->next;
            i = 0;
        }
        return found;
    }

    std::vector<RecordId> range_values(const Key& left, const Key& right) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<RecordId> values;
        if (!less(left, right)) {
            return values;
        }
        const Leaf* leaf = find_leaf(left);
        if (leaf == nullptr) {
            return values;
        }
        std::size_t i = find_lower_index(leaf->keys, left);
        while (leaf != nullptr) {
            for (; i < leaf->keys.size(); ++i) {
                if (!less(leaf->keys[i], right)) {
                    return values;
                }
                for (std::size_t j = 0; j < leaf->values[i].size(); ++j) {
                    values.push_back(leaf->values[i][j]);
                }
            }
            leaf = leaf->next;
            i = 0;
        }
        return values;
    }

    bool insert(const Key& key, const RecordId& record_id) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        if (root_.get() == nullptr) {
            std::unique_ptr<Leaf> root_leaf(new Leaf());
            root_leaf->keys.push_back(key);
            root_leaf->values.push_back(std::vector<RecordId>(1, record_id));
            root_ = std::unique_ptr<Node>(root_leaf.release());
            ++keys_count_;
            ++values_count_;
            return true;
        }
        Leaf* leaf = find_leaf(key);
        std::size_t index = find_lower_index(leaf->keys, key);
        if (index < leaf->keys.size() && equal(leaf->keys[index], key)) {
            if (contains_record_id(leaf->values[index], record_id)) {
                return false;
            }
            leaf->values[index].push_back(record_id);
            ++values_count_;
            return true;
        }
        leaf->keys.insert(leaf->keys.begin() + index, key);
        leaf->values.insert(leaf->values.begin() + index, std::vector<RecordId>(1, record_id));
        ++keys_count_;
        ++values_count_;
        if (leaf->keys.size() > leaf_max_keys_) {
            split_leaf(leaf);
            return true;
        }
        update_all_parents(leaf->parent);
        return true;
    }

    bool erase(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        Leaf* leaf = find_leaf(key);
        if (leaf == nullptr) {
            return false;
        }
        std::size_t index = find_lower_index(leaf->keys, key);
        if (index >= leaf->keys.size()) {
            return false;
        }
        if (!equal(leaf->keys[index], key)) {
            return false;
        }
        values_count_ -= leaf->values[index].size();
        --keys_count_;
        leaf->keys.erase(leaf->keys.begin() + index);
        leaf->values.erase(leaf->values.begin() + index);
        if (leaf == root_.get()) {
            if (leaf->keys.empty()) {
                root_.reset();
            }
            return true;
        }
        rebalance_leaf(leaf);
        return true;
    }

    bool delete_value(const Key& key, const RecordId& record_id) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        Leaf* leaf = find_leaf(key);
        if (leaf == nullptr) {
            return false;
        }
        std::size_t key_index = find_lower_index(leaf->keys, key);
        if (key_index >= leaf->keys.size()) {
            return false;
        }
        if (!equal(leaf->keys[key_index], key)) {
            return false;
        }
        std::vector<RecordId>& values = leaf->values[key_index];
        std::size_t value_index = find_record_index(values, record_id);
        if (value_index >= values.size()) {
            return false;
        }
        values.erase(values.begin() + value_index);
        --values_count_;
        if (!values.empty()) {
            update_all_parents(leaf->parent);
            return true;
        }
        leaf->keys.erase(leaf->keys.begin() + key_index);
        leaf->values.erase(leaf->values.begin() + key_index);
        --keys_count_;
        if (leaf == root_.get()) {
            if (leaf->keys.empty()) {
                root_.reset();
            }
            return true;
        }
        rebalance_leaf(leaf);
        return true;
    }

private:
    struct IndexNode;

    struct Node {
        bool is_leaf;
        IndexNode* parent;

        explicit Node(bool leaf) : is_leaf(leaf), parent(nullptr) {}
        virtual ~Node() = default;
    };

    struct Leaf : Node {
        std::vector<Key> keys;
        std::vector<std::vector<RecordId>> values;
        Leaf* next;
        Leaf* prev;

        Leaf() : Node(true), next(nullptr), prev(nullptr) {}
    };

    struct IndexNode : Node {
        std::vector<Key> keys;
        std::vector<std::unique_ptr<Node>> children;

        IndexNode() : Node(false) {}
    };

    bool less(const Key& left, const Key& right) const {
        return compare_(left, right);
    }

    bool equal(const Key& left, const Key& right) const {
        return !compare_(left, right) && !compare_(right, left);
    }

    std::size_t find_lower_index(const std::vector<Key>& keys, const Key& key) const {
        std::size_t index = 0;
        std::size_t remain = keys.size();
        while (remain > 0) {
            std::size_t half = remain / 2;
            std::size_t middle = index + half;
            if (less(keys[middle], key)) {
                index = middle + 1;
                remain -= half + 1;
            } else {
                remain = half;
            }
        }
        return index;
    }

    std::size_t find_upper_index(const std::vector<Key>& keys, const Key& key) const {
        std::size_t index = 0;
        std::size_t remain = keys.size();
        while (remain > 0) {
            std::size_t half = remain / 2;
            std::size_t middle = index + half;
            if (!less(key, keys[middle])) {
                index = middle + 1;
                remain -= half + 1;
            } else {
                remain = half;
            }
        }
        return index;
    }

    bool contains_record_id(const std::vector<RecordId>& records, const RecordId& record_id) const {
        for (std::size_t i = 0; i < records.size(); ++i) {
            if (records[i] == record_id) {
                return true;
            }
        }
        return false;
    }

    std::size_t find_record_index(const std::vector<RecordId>& records, const RecordId& record_id) const {
        for (std::size_t i = 0; i < records.size(); ++i) {
            if (records[i] == record_id) {
                return i;
            }
        }
        return records.size();
    }

    const Leaf* find_leaf(const Key& key) const {
        const Node* current = root_.get();
        while (current != nullptr && !current->is_leaf) {
            const IndexNode* node = static_cast<const IndexNode*>(current);
            std::size_t child_index = find_upper_index(node->keys, key);
            current = node->children[child_index].get();
        }
        return static_cast<const Leaf*>(current);
    }

    Leaf* find_leaf(const Key& key) {
        const BPlusTree* tree = this;
        const Leaf* leaf = tree->find_leaf(key);
        return const_cast<Leaf*>(leaf);
    }

    const Key& find_min_key(const Node* node) const {
        const Node* current = node;
        while (!current->is_leaf) {
            const IndexNode* node = static_cast<const IndexNode*>(current);
            current = node->children.front().get();
        }
        const Leaf* leaf = static_cast<const Leaf*>(current);
        if (leaf->keys.empty()) {
            throw std::logic_error("leaf->keys is empty");
        }
        return leaf->keys.front();
    }

    void update_keys(IndexNode* node) {
        node->keys.clear();
        if (node->children.size() < 2) {
            return;
        }
        for (std::size_t i = 1; i < node->children.size(); ++i) {
            node->keys.push_back(find_min_key(node->children[i].get()));
        }
    }

    void update_all_parents(IndexNode* node) {
        IndexNode* current = node;
        while (current != nullptr) {
            update_keys(current);
            current = current->parent;
        }
    }

    std::size_t find_child_index(const IndexNode* parent, const Node* child) const {
        for (std::size_t i = 0; i < parent->children.size(); ++i) {
            if (parent->children[i].get() == child) {
                return i;
            }
        }
        throw std::logic_error("child is not found");
    }

    std::size_t min_leaf_keys() const {
        return (leaf_max_keys_ + 1) / 2;
    }

    std::size_t min_node_children() const {
        return (node_max_keys_ + 2) / 2;
    }

    void insert_right_sibling(Node* left_child, const Key& split, std::unique_ptr<Node> right_child) {
        if (left_child == root_.get()) {
            std::unique_ptr<IndexNode> new_root(new IndexNode());
            std::unique_ptr<Node> old_root = std::move(root_);
            old_root->parent = new_root.get();
            right_child->parent = new_root.get();
            new_root->children.push_back(std::move(old_root));
            new_root->children.push_back(std::move(right_child));
            new_root->keys.push_back(split);
            root_ = std::unique_ptr<Node>(new_root.release());
            return;
        }
        IndexNode* parent = left_child->parent;
        std::size_t left_index = find_child_index(parent, left_child);
        right_child->parent = parent;
        parent->children.insert(parent->children.begin() + left_index + 1, std::move(right_child));
        parent->keys.insert(parent->keys.begin() + left_index, split);
        if (parent->keys.size() > node_max_keys_) {
            split_node(parent);
        } else {
            update_all_parents(parent);
        }
    }

    void split_leaf(Leaf* leaf) {
        std::unique_ptr<Leaf> right_leaf(new Leaf());
        std::size_t keys_count = leaf->keys.size();
        std::size_t split_index = keys_count / 2;
        for (std::size_t i = split_index; i < keys_count; ++i) {
            right_leaf->keys.push_back(leaf->keys[i]);
            right_leaf->values.push_back(std::move(leaf->values[i]));
        }
        leaf->keys.erase(leaf->keys.begin() + split_index, leaf->keys.end());
        leaf->values.erase(leaf->values.begin() + split_index, leaf->values.end());
        right_leaf->next = leaf->next;
        right_leaf->prev = leaf;
        if (right_leaf->next != nullptr) {
            right_leaf->next->prev = right_leaf.get();
        }
        leaf->next = right_leaf.get();
        Key split = right_leaf->keys.front();
        insert_right_sibling(leaf, split, std::unique_ptr<Node>(right_leaf.release()));
    }

    void split_node(IndexNode* node) {
        std::unique_ptr<IndexNode> right_node(new IndexNode());
        std::size_t children_count = node->children.size();
        std::size_t left_children_count = (children_count + 1) / 2;
        for (std::size_t i = left_children_count; i < children_count; ++i) {
            right_node->children.push_back(std::move(node->children[i]));
            right_node->children.back()->parent = right_node.get();
        }
        node->children.erase(node->children.begin() + left_children_count, node->children.end());
        update_keys(node);
        update_keys(right_node.get());
        Key split = find_min_key(right_node->children.front().get());
        insert_right_sibling(node, split, std::unique_ptr<Node>(right_node.release()));
    }

    void rebalance_leaf(Leaf* leaf) {
        if (leaf == root_.get()) {
            if (leaf->keys.empty()) {
                root_.reset();
            }
            return;
        }
        if (leaf->keys.size() >= min_leaf_keys()) {
            update_all_parents(leaf->parent);
            return;
        }
        IndexNode* parent = leaf->parent;
        std::size_t index = find_child_index(parent, leaf);
        if (index > 0) {
            Leaf* left_leaf = static_cast<Leaf*>(parent->children[index - 1].get());
            if (left_leaf->keys.size() > min_leaf_keys()) {
                Key left_leaf_key = left_leaf->keys.back();
                std::vector<RecordId> left_leaf_values;
                left_leaf_values.swap(left_leaf->values.back());
                left_leaf->keys.pop_back();
                left_leaf->values.pop_back();
                leaf->keys.insert(leaf->keys.begin(), left_leaf_key);
                leaf->values.insert(leaf->values.begin(), std::vector<RecordId>());
                leaf->values.front().swap(left_leaf_values);
                update_all_parents(parent);
                return;
            }
        }
        if (index + 1 < parent->children.size()) {
            Leaf* right_leaf = static_cast<Leaf*>(parent->children[index + 1].get());
            if (right_leaf->keys.size() > min_leaf_keys()) {
                Key right_leaf_key = right_leaf->keys.front();
                std::vector<RecordId> right_leaf_values;
                right_leaf_values.swap(right_leaf->values.front());
                right_leaf->keys.erase(right_leaf->keys.begin());
                right_leaf->values.erase(right_leaf->values.begin());
                leaf->keys.push_back(right_leaf_key);
                leaf->values.push_back(std::vector<RecordId>());
                leaf->values.back().swap(right_leaf_values);
                update_all_parents(parent);
                return;
            }
        }
        if (index > 0) {
            Leaf* left_leaf = static_cast<Leaf*>(parent->children[index - 1].get());
            for (std::size_t i = 0; i < leaf->keys.size(); ++i) {
                left_leaf->keys.push_back(leaf->keys[i]);
                left_leaf->values.push_back(std::move(leaf->values[i]));
            }
            left_leaf->next = leaf->next;
            if (leaf->next != nullptr) {
                leaf->next->prev = left_leaf;
            }
            parent->children.erase(parent->children.begin() + index);
            update_keys(parent);
            rebalance_node(parent);
            return;
        }
        Leaf* right_sibling = static_cast<Leaf*>(parent->children[index + 1].get());
        for (std::size_t i = 0; i < right_sibling->keys.size(); ++i) {
            leaf->keys.push_back(right_sibling->keys[i]);
            leaf->values.push_back(std::move(right_sibling->values[i]));
        }
        leaf->next = right_sibling->next;
        if (right_sibling->next != nullptr) {
            right_sibling->next->prev = leaf;
        }
        parent->children.erase(parent->children.begin() + index + 1);
        update_keys(parent);
        rebalance_node(parent);
    }

    void rebalance_node(IndexNode* node) {
        if (node == root_.get()) {
            if (node->children.empty()) {
                root_.reset();
                return;
            }
            if (node->children.size() == 1) {
                std::unique_ptr<Node> new_root = std::move(node->children.front());
                new_root->parent = nullptr;
                root_ = std::move(new_root);
                return;
            }
            update_keys(node);
            return;
        }
        if (node->children.size() >= min_node_children()) {
            update_keys(node);
            update_all_parents(node->parent);
            return;
        }
        IndexNode* parent = node->parent;
        std::size_t index = find_child_index(parent, node);
        if (index > 0) {
            IndexNode* left_node = static_cast<IndexNode*>(parent->children[index - 1].get());
            if (left_node->children.size() > min_node_children()) {
                std::unique_ptr<Node> left_node_child = std::move(left_node->children.back());
                left_node->children.pop_back();
                left_node_child->parent = node;
                node->children.insert(node->children.begin(), std::move(left_node_child));
                update_keys(left_node);
                update_keys(node);
                update_all_parents(parent);
                return;
            }
        }
        if (index + 1 < parent->children.size()) {
            IndexNode* right_node = static_cast<IndexNode*>(parent->children[index + 1].get());
            if (right_node->children.size() > min_node_children()) {
                std::unique_ptr<Node> right_node_child = std::move(right_node->children.front());
                right_node->children.erase(right_node->children.begin());
                right_node_child->parent = node;
                node->children.push_back(std::move(right_node_child));
                update_keys(right_node);
                update_keys(node);
                update_all_parents(parent);
                return;
            }
        }
        if (index > 0) {
            IndexNode* left_node = static_cast<IndexNode*>(parent->children[index - 1].get());
            for (std::size_t i = 0; i < node->children.size(); ++i) {
                left_node->children.push_back(std::move(node->children[i]));
                left_node->children.back()->parent = left_node;
            }
            update_keys(left_node);
            parent->children.erase(parent->children.begin() + index);
            update_keys(parent);
            rebalance_node(parent);
            return;
        }
        IndexNode* right_sibling = static_cast<IndexNode*>(parent->children[index + 1].get());
        for (std::size_t i = 0; i < right_sibling->children.size(); ++i) {
            node->children.push_back(std::move(right_sibling->children[i]));
            node->children.back()->parent = node;
        }
        update_keys(node);
        parent->children.erase(parent->children.begin() + index + 1);
        update_keys(parent);
        rebalance_node(parent);
    }

private:
    std::size_t leaf_max_keys_;
    std::size_t node_max_keys_;
    Compare compare_;
    mutable std::shared_mutex mutex_;
    std::unique_ptr<Node> root_;
    std::size_t keys_count_;
    std::size_t values_count_;
};
