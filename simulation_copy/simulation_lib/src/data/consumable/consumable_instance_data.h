#pragma once

#include "consumable_type_id.h"
#include "utility/custom_formatter.h"

namespace simulation
{
// Instance data for a ConsumableData
class ConsumableInstanceData
{
public:
    // Unique ID
    std::string id;

    // Type ID
    ConsumableTypeID type_id;

    // Is this item considered valid
    bool IsValid() const
    {
        return type_id.IsValid();
    }

    bool operator==(const ConsumableInstanceData& other) const
    {
        return type_id == other.type_id && id == other.id;
    }
    bool operator!=(const ConsumableInstanceData& other) const
    {
        return !(*this == other);
    }
    bool operator==(const ConsumableTypeID& other_type_id) const
    {
        return type_id == other_type_id;
    }

    void FormatTo(fmt::format_context& ctx) const;
};
}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(ConsumableInstanceData, FormatTo);
