#pragma once

#include "data/constants.h"
#include "data/hyper_config.h"

namespace simulation
{

struct HexGridPosition;
class Entity;
class World;

/* -------------------------------------------------------------------------------------------------------
 * HyperHelper
 *
 * This defines the helper functions to apply hyper to combat units
 * --------------------------------------------------------------------------------------------------------
 */
class HyperHelper
{
public:
    // Checks if entities are in range to affect hyper growth
    static bool AreInHyperRange(const Entity& first, const Entity& second, const HyperConfig& config);

    // Checks if entities are in range to affect hyper growth
    static bool
    ArePositionsInHyperRange(const HexGridPosition& first, const HexGridPosition& second, const HyperConfig& config);

    // Calculate current hyper effectiveness of combat unit
    static int CalculateHyperGrowthEffectiveness(const Entity& entity);
    static int CalculateHyperGrowthEffectiveness(const World& world, const EntityID entity_id);

    // Return false, if enemy is not in hyper range
    // Otherwise return true and contribution to hyper by given enemy
    static bool
    GetHyperContributionOfEnemyCombatUnit(const Entity& entity, const Entity& enemy_entity, int* out_contribution);
};

}  // namespace simulation
