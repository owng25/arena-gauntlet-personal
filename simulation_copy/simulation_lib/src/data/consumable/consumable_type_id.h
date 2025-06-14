#pragma once

#include "utility/custom_formatter.h"

namespace simulation
{
// Contains data about the type of consumable
class ConsumableTypeID
{
public:
    ConsumableTypeID() = default;
    ConsumableTypeID(const std::string& in_name, const int in_stage) : name{in_name}, stage{in_stage} {}

    // The name of the item
    std::string name{};

    // The stage of the item
    int stage = 0;

    bool operator==(const ConsumableTypeID& other) const
    {
        return name == other.name && stage == other.stage;
    }
    bool operator!=(const ConsumableTypeID& other) const
    {
        return !(*this == other);
    }

    // Is this item considered valid
    bool IsValid() const
    {
        return !name.empty();
    }

    struct HashFunction
    {
        size_t operator()(const ConsumableTypeID& type_id) const
        {
            const size_t name_hash = std::hash<std::string>{}(type_id.name);
            const size_t stage_hash = std::hash<int>{}(type_id.stage) << 1;
            return name_hash ^ stage_hash;
        }
    };

    void FormatTo(fmt::format_context& ctx) const;
};
}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(ConsumableTypeID, FormatTo);
