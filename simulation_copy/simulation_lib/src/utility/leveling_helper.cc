#include "leveling_helper.h"

#include "components/stats_component.h"
#include "ecs/entity.h"
#include "ecs/world.h"
#include "math.h"

namespace simulation
{
void LevelingHelper::SetBonusStats(const Entity& entity) const
{
    auto& stats_component = entity.Get<StatsComponent>();
    const int level = stats_component.GetLevel();

    // Check that the level is not below kMinLevel
    if (level < kMinLevel)
    {
        return;
    }

    StatsData level_bonus_stats;

    // Iterate over the stat types and apply the level bonus to each
    for (const StatType stat : kLevelGrowthStatTypes)
    {
        const FixedPoint base_value = stats_component.GetBaseValueForTypeBeforeLeveling(stat);
        const FixedPoint value_with_bonus = CalculateStatGrowth(base_value, level);

        // NOTE: the growth value already contains the base_value, we must subtract from it the base
        // to see the actual increase for the bonus
        const FixedPoint bonus_value = value_with_bonus - base_value;
        level_bonus_stats.Set(stat, bonus_value);
    }

    stats_component.SetLevelBonusStats(level_bonus_stats);
}

FixedPoint LevelingHelper::CalculateStatGrowth(const FixedPoint base_value, const int level) const
{
    // Follows this formula here:
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44204724/Illuvial+Combat+Stat+Growth.
    // Actual Stat = round(Value at level 1 * (1 + ((Current Level - 1 )/(Max Level - 1)) * Level Bonus)).

    // Current Level is in a predefined range
    const auto current_level_fp = FixedPoint::FromInt(std::clamp(level, kMinLevel, kMaxLevel));

    // Value at level 1 = base_value
    // Level Bonus = growth_rate_percentage
    FixedPoint bonus =
        ((base_value * growth_rate_percentage * (current_level_fp - 1_fp)) / ((kMaxLevelFP - 1_fp) * kMaxPercentageFP));

    // bonus = Math::Round(bonus / stat_scale) * stat_scale;
    bonus = Math::Round(bonus);

    return base_value + bonus;
}

}  // namespace simulation
