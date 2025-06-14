#pragma once

#include <string>

#include "utility/custom_formatter.h"

namespace simulation
{
// Contains data about the type of suit
class CombatSuitTypeID
{
public:
    CombatSuitTypeID() = default;
    CombatSuitTypeID(const std::string& in_name, const int in_stage, const std::string& in_variation)
        : name{in_name},
          variation{in_variation},
          stage{in_stage}
    {
    }

    bool operator==(const CombatSuitTypeID& other) const
    {
        return name == other.name && stage == other.stage && variation == other.variation;
    }
    bool operator!=(const CombatSuitTypeID& other) const
    {
        return !(*this == other);
    }

    // Is this suit considered valid
    bool IsValid() const
    {
        return !name.empty();
    }

    struct HashFunction
    {
        template <typename T>
        static size_t GetHash(const T& value)
        {
            return std::hash<T>{}(value);
        }

        size_t operator()(const CombatSuitTypeID& type_id) const
        {
            const size_t name_hash = GetHash(type_id.name);
            const size_t stage_hash = GetHash(type_id.stage) << 1;
            const size_t variation_hash = GetHash(type_id.variation) << 1;
            return ((name_hash ^ stage_hash) >> 1) ^ variation_hash;
        }
    };

    void FormatTo(fmt::format_context& ctx) const;

public:
    // The name of the suit
    std::string name{};

    // The variation of the suit
    std::string variation{};

    // The stage of the suit
    int stage = 0;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatSuitTypeID, FormatTo);
