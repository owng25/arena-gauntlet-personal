#pragma once

#include "encounter_mod_type_id.h"
#include "utility/custom_formatter.h"

namespace simulation
{
// Contains data about an instance of encounter mod
struct EncounterModInstanceData
{
    // Unique ID
    std::string id;

    // Type ID
    EncounterModTypeID type_id;

    // Is this item considered valid
    bool IsValid() const
    {
        return type_id.IsValid();
    }

    // Overload comparison
    bool operator==(const EncounterModInstanceData& other) const
    {
        return type_id == other.type_id && id == other.id;
    }
    bool operator!=(const EncounterModInstanceData& other) const
    {
        return !(*this == other);
    }

    void FormatTo(fmt::format_context& ctx) const;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(EncounterModInstanceData, FormatTo);
