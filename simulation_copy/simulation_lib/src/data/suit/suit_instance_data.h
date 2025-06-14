#pragma once

#include "suit_type_id.h"
#include "utility/custom_formatter.h"

namespace simulation
{
// Instance data for a CombatSuitTypeID
class CombatSuitInstanceData
{
public:
    // Is this item considered valid
    bool IsValid() const
    {
        return type_id.IsValid();
    }

    // Overload comparison
    bool operator==(const CombatSuitInstanceData& other) const
    {
        return type_id == other.type_id && id == other.id;
    }
    bool operator!=(const CombatSuitInstanceData& other) const
    {
        return !(*this == other);
    }

    void FormatTo(fmt::format_context& ctx) const;

public:
    // Unique ID
    std::string id;

    // Type ID
    CombatSuitTypeID type_id;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatSuitInstanceData, FormatTo);
