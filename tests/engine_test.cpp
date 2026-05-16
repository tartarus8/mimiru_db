#include <gtest/gtest.h>
#include <filesystem>
#include "mimiru/engine/system_manager.hpp"

using namespace mimiru::core;
using namespace mimiru::engine;

class EngineTest : public ::testing::Test {
protected:
    const std::string root_dir = "mimiru_data";

    void SetUp() override {
        std::filesystem::remove_all(root_dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(root_dir);
    }
};

TEST_F(EngineTest, FullHierarchyTest) {
    SystemManager sys(root_dir);

    sys.create_database("shop");
    EXPECT_NO_THROW(sys.use_database("shop"));

    Database* db = sys.get_current_db();
    ASSERT_NE(db, nullptr);

    TableSchema schema;
    schema.name = "products";

    Column c1{"id", DataType::INT, {true, true, false, 0}};
    Column c2{"price", DataType::INT, {false, false, false, 0}};
    schema.columns = {c1, c2};

    db->create_table(schema);
    EXPECT_TRUE(db->has_table("products"));

    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(root_dir) / "shop" / "products.db"));

    Table* table = db->get_table("products");

    std::vector<NullableValue> row1 = {101, 500};
    std::vector<NullableValue> row2 = {102, 750};

    EXPECT_NO_THROW(table->insert(row1));
    EXPECT_NO_THROW(table->insert(row2));

    std::vector<NullableValue> duplicate_row = {101, 999};
    EXPECT_THROW(table->insert(duplicate_row), ConstraintViolation);
}

TEST_F(EngineTest, DropDatabaseCleanup) {
    SystemManager sys(root_dir);
    sys.create_database("temp_db");
    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(root_dir) / "temp_db"));

    sys.drop_database("temp_db");
    EXPECT_FALSE(std::filesystem::exists(std::filesystem::path(root_dir) / "temp_db"));
}

TEST_F(EngineTest, PaginationStressTest) {
    SystemManager sys(root_dir);
    sys.create_database("heavy_db");
    sys.use_database("heavy_db");

    Database* db = sys.get_current_db();

    TableSchema schema;
    schema.name = "logs";
    schema.columns = {
        {"id", DataType::INT, {true, true, false, 0}},
        {"value", DataType::INT, {false, false, false, 0}}
    };

    db->create_table(schema);
    Table* table = db->get_table("logs");

    const int NUM_RECORDS = 2000;

    for (int i = 0; i < NUM_RECORDS; ++i) {
        std::vector<NullableValue> row = {i, i * 10};
        EXPECT_NO_THROW(table->insert(row));
    }

    auto table_file = std::filesystem::path(root_dir) / "heavy_db" / "logs.db";
    auto file_size = std::filesystem::file_size(table_file);

    EXPECT_GE(file_size, 5 * PAGE_SIZE);

    std::cout << "[ INFO     ] Pagination successful! File grew to: "
              << file_size / 1024 << " KB" << std::endl;
}
