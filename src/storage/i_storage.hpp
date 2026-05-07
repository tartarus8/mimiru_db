#pragma once
#include "../core/types.hpp"
#include <vector>

namespace dbms::storage {

    class IStorageManager {
    public:
        virtual ~IStorageManager() = default;

        // insert returns generated id, virtual orr offset
        virtual core::RecordId insertRecord(const std::vector<core::NullableValue>& fields) = 0;

        virtual std::vector<core::Record> scanAll() = 0;
    };

}
