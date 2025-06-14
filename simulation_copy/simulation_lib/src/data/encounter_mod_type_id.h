#pragma once

#include <functional>  // for std::hash
#include <string>

#include "utility/custom_formatter.h"

namespace simulation
{
// Contains data about the type of encounter mod
class EncounterModTypeID
{
public:
    std::string name{};

    int stage = 0;

    bool operator==(const EncounterModTypeID& other) const
    {
        return name == other.name && stage == other.stage;
    }
    bool operator!=(const EncounterModTypeID& other) const
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
        size_t operator()(const EncounterModTypeID& type_id) const
        {
            const size_t name_hash = std::hash<std::string>{}(type_id.name);
            const size_t stage_hash = std::hash<int>{}(type_id.stage) << 1;
            return name_hash ^ stage_hash;
        }
    };

    void FormatTo(fmt::format_context& ctx) const;
};
}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(EncounterModTypeID, FormatTo);
