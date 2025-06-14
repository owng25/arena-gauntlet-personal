#pragma once

#include "utility/fixed_point.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * HyperConfig
 *
 * This struct holds different bonuses applied by hyper
 * --------------------------------------------------------------------------------------------------------
 */
class HyperConfig
{
public:
    // This holds data that can scale the sub hyper
    // It can't be 0 as it's used for divide
    FixedPoint sub_hyper_scale_percentage = 100_fp;

    // The range at which we check the enemies and increase the hyper
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/160006457/Hyper+Calculation#1.-Get-a-list-of-enemies-in-range.
    int enemies_range_units = 50;
};

}  // namespace simulation
