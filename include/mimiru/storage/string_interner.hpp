#pragma once

#include "mimiru/core/types.hpp"
#include "mimiru/storage/pager.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <shared_mutex>

namespace mimiru::storage {

class StringInterner {
public:
    explicit StringInterner(Pager& pager);

    core::StringId intern(const std::string& str);

    std::string get(core::StringId id) const;

    void flush_to_disk();

private:
    Pager& pager_;

    mutable std::shared_mutex mutex_;

    std::unordered_map<std::string, core::StringId> string_to_id_;
    std::vector<std::string> id_to_string_;

    void load_from_disk();
};

} // namespace mimiru::storage
