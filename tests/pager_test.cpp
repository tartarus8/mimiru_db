#include <gtest/gtest.h>
#include <filesystem>
#include <algorithm>
#include <vector>

#include "mimiru/storage/pager.hpp"
#include "mimiru/core/types.hpp"

namespace {

using namespace mimiru::storage;
using namespace mimiru::core;

class PagerTest : public ::testing::Test {
protected:
    const std::string test_db = "test_unit.db";

    void SetUp() override {
        std::filesystem::remove(test_db);
    }

    void TearDown() override {
        std::filesystem::remove(test_db);
    }
};

TEST_F(PagerTest, ReadWriteConsistency) {
    Pager pager(test_db);

    std::vector<char> write_data_A(PAGE_SIZE, 'A');
    std::vector<char> write_data_B(PAGE_SIZE, 'B');
    std::vector<char> read_buffer(PAGE_SIZE, 0);

    pager.allocate_page();
    pager.allocate_page();
    pager.allocate_page();
    pager.allocate_page();

    pager.write_page(0, write_data_A.data());
    pager.write_page(3, write_data_B.data());

    pager.read_page(0, read_buffer.data());
    EXPECT_EQ(read_buffer[0], 'A');
    EXPECT_EQ(read_buffer[PAGE_SIZE - 1], 'A');

    pager.read_page(3, read_buffer.data());
    EXPECT_EQ(read_buffer[0], 'B');
    EXPECT_EQ(read_buffer[PAGE_SIZE - 1], 'B');
}

TEST_F(PagerTest, PersistenceTest) {
    {
        Pager pager(test_db);
        pager.allocate_page();
        std::vector<char> write_data(PAGE_SIZE, 'Z');
        pager.write_page(0, write_data.data());
        pager.sync();
    }

    Pager pager_reopened(test_db);
    std::vector<char> read_buffer(PAGE_SIZE, 0);

    ASSERT_EQ(pager_reopened.get_num_pages(), 1);
    pager_reopened.read_page(0, read_buffer.data());

    EXPECT_EQ(read_buffer[0], 'Z');
}

TEST_F(PagerTest, BoundsChecking) {
    Pager pager(test_db);
    std::vector<char> buffer(PAGE_SIZE, 0);

    EXPECT_THROW(pager.read_page(99, buffer.data()), std::out_of_range);

    EXPECT_THROW(pager.write_page(5, buffer.data()), std::out_of_range);
}

} // namespace
