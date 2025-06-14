#pragma once

#include <unordered_map>

#include "data/enums.h"

namespace simulation
{
using CombatAffinityOpponentsMap = std::unordered_map<CombatAffinity, std::unordered_map<CombatAffinity, int>>;

/* -------------------------------------------------------------------------------------------------------
 * HyperData
 *
 * This struct holds the effectiveness for each combat class and combat affinity
 *
 * --------------------------------------------------------------------------------------------------------
 */
class HyperData
{
public:
    std::unique_ptr<HyperData> CreateDeepCopy() const
    {
        return std::make_unique<HyperData>(*this);
    }

    // Gets the combat affinity effectiveness from the matrix affinities
    int GetCombatAffinityEffectiveness(
        const CombatAffinity combat_affinity,
        const CombatAffinity opponent_combat_affinity) const
    {
        if (combat_affinity_opponents.contains(combat_affinity))
        {
            const auto& opponents = combat_affinity_opponents.at(combat_affinity);
            if (opponents.contains(opponent_combat_affinity))
            {
                return opponents.at(opponent_combat_affinity);
            }
        }
        return 0;
    }

    // Combat Affinity Effectiveness against different opponents
    // Key: CombatAffinity which has effectiveness for different values
    // Value: Unordered Map which holds effectiveness against different entities of the Key
    CombatAffinityOpponentsMap combat_affinity_opponents;
};

}  // namespace simulation
