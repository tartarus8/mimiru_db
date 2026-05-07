#pragma once
#include "i_storage.hpp"
#include <unordered_map>

namespace dbms::storage {

    class MemoryStorage : public IStorageManager {
    public:
        core::RecordId insertRecord(const std::vector<core::NullableValue>& fields) override {
            core::RecordId id = next_id_++;
            data_[id] = {id, fields};
            return id;
        }

        std::vector<core::Record> scanAll() override {
            std::vector<core::Record> records;
            for (const auto& [id, record] : data_) {
                records.push_back(record);
            }
            return records;
        }

    private:
        core::RecordId next_id_ = 1;
        std::unordered_map<core::RecordId, core::Record> data_;
    };

}
