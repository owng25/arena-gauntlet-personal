#pragma once

#include "utility/fixed_point.h"

namespace simulation
{

class Entity;

/* -------------------------------------------------------------------------------------------------------
 * LevelingHelper
 *
 * This defines the helper functions to apply level stat growth to combat units
 * --------------------------------------------------------------------------------------------------------
 */
class LevelingHelper
{
public:
    // Sets all the template stats from another StatsData
    void SetBonusStats(const Entity& entity) const;

    // Calculate how much the stats should grow based on the level of the combat unit
    FixedPoint CalculateStatGrowth(const FixedPoint base_value, const int level) const;

    FixedPoint growth_rate_percentage = 5_fp;
    FixedPoint stat_scale = 1_fp;
};

}  // namespace simulation
