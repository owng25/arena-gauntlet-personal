#pragma once

#include <vector>

#include "data/combat_synergy_bonus.h"
#include "data/combat_unit_type_id.h"

namespace simulation
{
// Helper to keep track of the type id and the base Combat Synergy
struct SynergyOrderCombatUnit
{
    // The unique unit type id
    CombatUnitTypeID type_id{};

    // Cached value.
    // The base combat synergy, can be either a combat class or a combat affinity
    CombatSynergyBonus base_combat_synergy{};

    friend bool operator==(const SynergyOrderCombatUnit& lhs, const CombatUnitTypeID& rhs)
    {
        return lhs.type_id == rhs;
    }
    friend bool operator==(const SynergyOrderCombatUnit& lhs, const SynergyOrderCombatUnit& rhs)
    {
        return lhs.type_id == rhs.type_id;
    }
    friend bool operator!=(const SynergyOrderCombatUnit& lhs, const SynergyOrderCombatUnit& rhs)
    {
        return !(lhs == rhs);
    }
};

/* -------------------------------------------------------------------------------------------------------
 * SynergyOrder
 *
 * This struct holds unique entities and combat affinities combined in right order
 * --------------------------------------------------------------------------------------------------------
 */
struct SynergyOrder
{
    // Every Combat Unit can provide Synergy Bonuses to their team. Each has an amount of
    // synergy_stacks which are coded to the synergy. Docs:
    // - https://illuvium.atlassian.net/wiki/spaces/AB/pages/108232948/Synergy+Stack
    // - https://illuvium.atlassian.net/wiki/spaces/AB/pages/108200183/Synergy+Overlap
    int synergy_stacks = 0;

    // The combat synergy, can be either a combat class or a combat affinity
    CombatSynergyBonus combat_synergy;

    // All the combat units that either have this combat class or class affinity
    // TODO(vampy): Does this even work with weapons?
    std::vector<SynergyOrderCombatUnit> combat_units;
};

}  // namespace simulation
