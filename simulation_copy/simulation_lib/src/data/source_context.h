#pragma once

#include <vector>

#include "data/combat_synergy_bonus.h"
#include "data/enums.h"
#include "utility/custom_formatter.h"
#include "utility/vector_helper.h"

namespace simulation
{
// Struct that describes the source of an ability/effect/entity
struct SourceContextData
{
    bool Has(const SourceContextType type) const
    {
        return VectorHelper::ContainsValue(sources, type);
    }

    void Add(const SourceContextType type)
    {
        if (!Has(type))
        {
            sources.push_back(type);
        }
    }

    // Is this context empty?
    bool IsEmpty() const
    {
        return sources.empty();
    }

    void FormatTo(fmt::format_context& ctx) const;

    // The original combat unit sender ability type
    // NOTE: Only used for non combat units
    AbilityType combat_unit_ability_type = AbilityType::kNone;

    // Is this from a specific weapon type?
    WeaponType from_weapon_type = WeaponType::kNone;

    // The name of the last ability that caused this
    std::string ability_name;

    // The name of the last skill that caused this
    std::string skill_name;

    // From what synergy type is this ability type
    // NOTE: Only used if Has(SourceContextType::kSynergy) is true
    CombatSynergyBonus combat_synergy;

    // The history of all the contexts
    std::vector<SourceContextType> sources;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(SourceContextData, FormatTo);
