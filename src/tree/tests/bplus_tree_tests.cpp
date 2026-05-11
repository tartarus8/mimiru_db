#include "gtest/gtest.h"
#include "../include/bplus_tree.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

using Tree = BPlusTree<int, int>;

std::vector<std::pair<int, int>> flatten_range_result(
    const std::vector<std::pair<int, std::vector<int>>>& pairs) {
    std::vector<std::pair<int, int>> out;
    for (const auto& [key, values] : pairs) {
        for (int value : values) {
            out.emplace_back(key, value);
        }
    }
    std::sort(out.begin(), out.end(),
              [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
              });
    return out;
}

std::vector<int> flatten_range_values_result(const std::vector<int>& values) {
    auto out = values;
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::pair<int, int>> expected_pairs(const std::multimap<int, int>& expected, int left, int right) {
    std::vector<std::pair<int, int>> result;
    auto current = expected.lower_bound(left);
    auto end = expected.lower_bound(right);
    for (; current != end; ++current) {
        result.emplace_back(current->first, current->second);
    }
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
              });
    return result;
}

std::vector<int> expected_values(const std::multimap<int, int>& expected, int left, int right) {
    std::vector<int> result;
    auto current = expected.lower_bound(left);
    auto end = expected.lower_bound(right);
    for (; current != end; ++current) {
        result.push_back(current->second);
    }
    std::sort(result.begin(), result.end());
    return result;
}

void expect_same_as_model(Tree& tree, const std::multimap<int, int>& expected) {
    auto tree_pairs = flatten_range_result(tree.range(-1000000, 1000000));
    auto expected_pairs_all = expected_pairs(expected, -1000000, 1000000);
    EXPECT_EQ(tree_pairs, expected_pairs_all);
    auto tree_values = flatten_range_values_result(tree.range_values(-1000000, 1000000));
    auto expected_values_all = expected_values(expected, -1000000, 1000000);
    EXPECT_EQ(tree_values, expected_values_all);
}

TEST(BPlusTree, EmptyTreeReturnsEmptyRanges) {
    Tree tree(3, 3);
    EXPECT_TRUE(tree.range(0, 10).empty());
    EXPECT_TRUE(tree.range_values(0, 10).empty());
    EXPECT_TRUE(tree.range(-100, -50).empty());
    EXPECT_TRUE(tree.range_values(-100, -50).empty());
}

TEST(BPlusTree, SingleInsertUsesHalfOpenIntervals) {
    Tree tree(3, 3);
    tree.insert(10, 100);
    auto pairs = flatten_range_result(tree.range(10, 11));
    EXPECT_EQ(pairs, (std::vector<std::pair<int, int>>{{10, 100}}));
    EXPECT_EQ(tree.range_values(10, 11), (std::vector<int>{100}));
    EXPECT_TRUE(tree.range(10, 10).empty());
    EXPECT_TRUE(tree.range_values(10, 10).empty());
    EXPECT_TRUE(tree.range(11, 12).empty());
    EXPECT_TRUE(tree.range_values(11, 12).empty());
    EXPECT_TRUE(tree.range(0, 10).empty());
    EXPECT_TRUE(tree.range_values(0, 10).empty());
}

TEST(BPlusTree, UnsortedInsertionsAreReturnedInSortedKeyOrder) {
    Tree tree(3, 3);
    tree.insert(5, 50);
    tree.insert(1, 10);
    tree.insert(3, 30);
    tree.insert(2, 20);
    tree.insert(4, 40);
    auto all = flatten_range_result(tree.range(-1000000, 1000000));
    std::vector<std::pair<int, int>> expected_all = {{1,10}, {2,20}, {3,30}, {4,40}, {5,50}};
    EXPECT_EQ(all, expected_all);
    auto range23 = flatten_range_result(tree.range(2, 5));
    std::vector<std::pair<int, int>> expected23 = {{2,20}, {3,30}, {4,40}};
    EXPECT_EQ(range23, expected23);
    EXPECT_EQ(tree.range_values(2, 5), (std::vector<int>{20, 30, 40}));
}

TEST(BPlusTree, DuplicateKeysAreVisibleInRangeQueries) {
    Tree tree(3, 3);
    tree.insert(7, 701);
    tree.insert(7, 702);
    tree.insert(7, 703);
    tree.insert(6, 600);
    tree.insert(8, 800);
    auto exact_pairs = flatten_range_result(tree.range(7, 8));
    auto exact_values = tree.range_values(7, 8);
    auto around_pairs = flatten_range_result(tree.range(6, 9));
    std::sort(exact_values.begin(), exact_values.end());
    EXPECT_EQ(exact_pairs, (std::vector<std::pair<int, int>>{{7,701}, {7,702}, {7,703}}));
    EXPECT_EQ(exact_values, (std::vector<int>{701, 702, 703}));
    EXPECT_EQ(around_pairs, (std::vector<std::pair<int, int>>{{6,600}, {7,701}, {7,702}, {7,703}, {8,800}}));
}

TEST(BPlusTree, ExactDuplicateEraseWorks) {
    Tree tree(3, 3);
    tree.insert(4, 400);
    tree.insert(5, 501);
    tree.insert(5, 502);
    tree.insert(5, 503);
    tree.insert(6, 600);
    tree.delete_value(5, 502);
    auto pairs = flatten_range_result(tree.range(4, 7));
    auto values = tree.range_values(5, 6);
    EXPECT_EQ(pairs, (std::vector<std::pair<int, int>>{{4,400}, {5,501}, {5,503}, {6,600}}));
    EXPECT_EQ(values, (std::vector<int>{501, 503}));
}

TEST(BPlusTree, ManyInsertionsStayCorrect) {
    Tree tree(3, 3);
    std::vector<int> keys(200);
    std::iota(keys.begin(), keys.end(), 0);
    std::mt19937 rng(123456);
    std::shuffle(keys.begin(), keys.end(), rng);
    std::multimap<int, int> expected;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        int key = keys[i];
        int value = 1000 + key;
        tree.insert(key, value);
        expected.emplace(key, value);
        if (i % 17 == 0 || i + 1 == keys.size()) {
            expect_same_as_model(tree, expected);
        }
    }
    auto pairs = flatten_range_result(tree.range(50, 75));
    EXPECT_EQ(pairs, expected_pairs(expected, 50, 75));
    EXPECT_EQ(tree.range_values(50, 75), expected_values(expected, 50, 75));
}

TEST(BPlusTree, ManyDeletionsStayCorrect) {
    Tree tree(3, 3);
    std::multimap<int, int> expected;
    for (int key = 0; key < 256; ++key) {
        tree.insert(key, 1000 + key);
        expected.emplace(key, 1000 + key);
    }
    expect_same_as_model(tree, expected);
    for (int key = 0; key < 256; key += 2) {
        tree.delete_value(key, 1000 + key);
        auto current = expected.find(key);
        ASSERT_NE(current, expected.end());
        expected.erase(current);
    }
    for (int key = 1; key < 256; key += 2) {
        tree.delete_value(key, 1000 + key);
        auto current = expected.find(key);
        ASSERT_NE(current, expected.end());
        expected.erase(current);
    }
    expect_same_as_model(tree, expected);
    EXPECT_TRUE(tree.range(-1000000, 1000000).empty());
    EXPECT_TRUE(tree.range_values(-1000000, 1000000).empty());
}

TEST(BPlusTree, RangeRemainsHalfOpenAfterMutations) {
    Tree tree(3, 3);
    for (int key = 0; key < 10; ++key) {
        tree.insert(key, key * 10);
    }
    tree.erase(5);
    auto pairs = flatten_range_result(tree.range(3, 7));
    EXPECT_EQ(pairs, (std::vector<std::pair<int, int>>{{3,30}, {4,40}, {6,60}}));
    EXPECT_EQ(tree.range_values(3, 7), (std::vector<int>{30, 40, 60}));
    EXPECT_TRUE(tree.range(7, 7).empty());
    EXPECT_TRUE(tree.range_values(7, 7).empty());
    EXPECT_EQ(flatten_range_result(tree.range(7, 8)), (std::vector<std::pair<int, int>>{{7,70}}));
    EXPECT_EQ(tree.range_values(7, 8), (std::vector<int>{70}));
}

TEST(BPlusTree, RandomizedUpdatesMatchStdMultimapModel) {
    Tree tree(3, 3);
    std::multimap<int, int> expected;
    std::vector<bool> present(128, false);
    std::mt19937 rng(987654321);
    std::uniform_int_distribution<int> key_dist(0, 127);
    std::uniform_int_distribution<int> op_dist(0, 99);
    for (int step = 0; step < 2000; ++step) {
        int key = key_dist(rng);
        int value = 5000 + key;
        if (op_dist(rng) < 60) {
            if (!present[key]) {
                tree.insert(key, value);
                expected.emplace(key, value);
                present[key] = true;
            }
        } else {
            if (present[key]) {
                tree.delete_value(key, value);
                auto range = expected.equal_range(key);
                auto it = range.first;
                while (it != range.second && it->second != value) ++it;
                if (it != range.second) expected.erase(it);
                present[key] = false;
            }
        }
        if (step % 25 == 0) {
            expect_same_as_model(tree, expected);
            std::uniform_int_distribution<int> left_dist(0, 120);
            int left = left_dist(rng);
            std::uniform_int_distribution<int> right_dist(left + 1, 128);
            int right = right_dist(rng);
            auto tree_pairs = flatten_range_result(tree.range(left, right));
            auto exp_pairs = expected_pairs(expected, left, right);
            EXPECT_EQ(tree_pairs, exp_pairs);
            auto tree_vals = flatten_range_values_result(tree.range_values(left, right));
            auto exp_vals = expected_values(expected, left, right);
            EXPECT_EQ(tree_vals, exp_vals);
        }
    }
    expect_same_as_model(tree, expected);
}
