#include <gtest/gtest.h>
#include <filesystem>
#include <map>
#include <random>
#include <numeric>
#include <algorithm>

#include "mimiru/index/bplus_tree.hpp"
#include "mimiru/storage/pager.hpp"
#include "mimiru/core/types.hpp"

namespace {
using namespace mimiru::index;
using namespace mimiru::storage;
using namespace mimiru::core;

class BPlusTreeTest : public ::testing::Test {
protected:
    const std::string test_db = "test_tree.db";
    std::unique_ptr<Pager> pager;
    std::unique_ptr<BPlusTreeInt> tree;

    void SetUp() override {
        std::filesystem::remove(test_db);
        pager = std::make_unique<Pager>(test_db);
        tree = std::make_unique<BPlusTreeInt>(*pager, INVALID_PAGE_ID);
    }

    void TearDown() override {
        tree.reset();
        pager.reset();
        std::filesystem::remove(test_db);
    }

    std::vector<std::pair<int, uint32_t>> expected_pairs(const std::map<int, uint32_t>& expected, int left, int right) {
        std::vector<std::pair<int, uint32_t>> result;
        auto current = expected.lower_bound(left);
        auto end = expected.lower_bound(right);
        for (; current != end; ++current) {
            result.emplace_back(current->first, current->second);
        }
        return result;
    }

    void expect_same_as_model(const std::map<int, uint32_t>& expected) {
        auto tree_pairs = tree->range(-1000000, 1000000);
        auto expected_pairs_all = expected_pairs(expected, -1000000, 1000000);
        EXPECT_EQ(tree_pairs, expected_pairs_all);
    }
};

TEST_F(BPlusTreeTest, EmptyTreeReturnsEmptyRanges) {
    EXPECT_TRUE(tree->range(0, 10).empty());
    EXPECT_TRUE(tree->range(-100, -50).empty());
}

TEST_F(BPlusTreeTest, SingleInsertUsesHalfOpenIntervals) {
    tree->insert(10, 100);

    auto pairs = tree->range(10, 11);
    ASSERT_EQ(pairs.size(), 1);
    EXPECT_EQ(pairs[0], std::make_pair(10, 100u));

    EXPECT_TRUE(tree->range(10, 10).empty());
    EXPECT_TRUE(tree->range(11, 12).empty());
    EXPECT_TRUE(tree->range(0, 10).empty());
}

TEST_F(BPlusTreeTest, UnsortedInsertionsAreReturnedInSortedKeyOrder) {
    tree->insert(5, 50);
    tree->insert(1, 10);
    tree->insert(3, 30);
    tree->insert(2, 20);
    tree->insert(4, 40);

    auto all = tree->range(-1000000, 1000000);
    std::vector<std::pair<int, uint32_t>> expected_all = {
        {1,10}, {2,20}, {3,30}, {4,40}, {5,50}
    };
    EXPECT_EQ(all, expected_all);

    auto range25 = tree->range(2, 5);
    std::vector<std::pair<int, uint32_t>> expected25 = {
        {2,20}, {3,30}, {4,40}
    };
    EXPECT_EQ(range25, expected25);
}

TEST_F(BPlusTreeTest, DuplicateKeysOverwriteExistingValues) {
    tree->insert(7, 701);
    tree->insert(7, 702);

    auto exact_pairs = tree->range(7, 8);
    ASSERT_EQ(exact_pairs.size(), 1);
    EXPECT_EQ(exact_pairs[0].second, 702u);
}

TEST_F(BPlusTreeTest, ErasureWorks) {
    tree->insert(4, 400);
    tree->insert(5, 500);
    tree->insert(6, 600);

    tree->remove(5);

    auto pairs = tree->range(4, 7);
    std::vector<std::pair<int, uint32_t>> expected = {{4,400}, {6,600}};
    EXPECT_EQ(pairs, expected);
    EXPECT_FALSE(tree->find(5).has_value());
}

TEST_F(BPlusTreeTest, ManyInsertionsTriggerSplits) {
    std::vector<int> keys(2000);
    std::iota(keys.begin(), keys.end(), 0);
    std::mt19937 rng(123456);
    std::shuffle(keys.begin(), keys.end(), rng);

    std::map<int, uint32_t> expected;

    for (std::size_t i = 0; i < keys.size(); ++i) {
        int key = keys[i];
        uint32_t value = 1000 + key;
        tree->insert(key, value);
        expected[key] = value;

        if (i % 250 == 0 || i + 1 == keys.size()) {
            expect_same_as_model(expected);
        }
    }

    auto pairs = tree->range(500, 550);
    EXPECT_EQ(pairs, expected_pairs(expected, 500, 550));
}

TEST_F(BPlusTreeTest, RandomizedUpdatesMatchStdMapModel) {
    std::map<int, uint32_t> expected;
    std::mt19937 rng(987654321);
    std::uniform_int_distribution<int> key_dist(0, 5000);
    std::uniform_int_distribution<int> op_dist(0, 99);

    for (int step = 0; step < 5000; ++step) {
        int key = key_dist(rng);
        uint32_t value = 5000 + key;

        if (op_dist(rng) < 70) {
            tree->insert(key, value);
            expected[key] = value;
        }
        else {
            if (expected.find(key) != expected.end()) {
                tree->remove(key);
                expected.erase(key);
            }
        }

        if (step % 500 == 0) {
            expect_same_as_model(expected);
        }
    }
    expect_same_as_model(expected);
}

} // namespace
