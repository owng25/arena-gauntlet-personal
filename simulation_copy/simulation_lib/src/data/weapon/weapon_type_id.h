#pragma once

#include <string>

#include "data/enums.h"
#include "utility/custom_formatter.h"

namespace simulation
{
// Unique identifier for weapon type
class CombatWeaponTypeID
{
public:
    CombatWeaponTypeID() = default;

    CombatWeaponTypeID(const std::string& in_name, const int in_stage, const CombatAffinity in_combat_affinity)
        : name(in_name),
          stage(in_stage),
          combat_affinity{in_combat_affinity}
    {
    }

    bool operator==(const CombatWeaponTypeID& other) const
    {
        return name == other.name && stage == other.stage && combat_affinity == other.combat_affinity &&
               variation == other.variation;
    }
    bool operator!=(const CombatWeaponTypeID& other) const
    {
        return !(*this == other);
    }

    struct HashFunction
    {
        template <typename T>
        static size_t GetHash(const T& value)
        {
            return std::hash<T>{}(value);
        }

        size_t operator()(const CombatWeaponTypeID& type_id) const
        {
            const size_t name_hash = GetHash(type_id.name);
            const size_t stage_hash = GetHash(type_id.stage);
            const size_t affinity_hash = GetHash(type_id.combat_affinity);
            const size_t variation_hash = GetHash(type_id.variation);
            return ((((name_hash ^ (stage_hash << 1)) >> 1) ^ affinity_hash) << 1) ^ variation_hash;
        }
    };

    // Is this item considered valid
    bool IsValid() const
    {
        return !name.empty();
    }

    void FormatTo(fmt::format_context& ctx) const;

public:
    // The name of the item
    std::string name{};

    // The stage of the item
    int stage = 0;

    // The combat affinity, can be none
    CombatAffinity combat_affinity = CombatAffinity::kNone;

    // Item variation
    std::string variation{};
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatWeaponTypeID, FormatTo);
