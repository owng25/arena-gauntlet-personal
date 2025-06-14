#include "utility/stats_helper.h"

#include "components/stats_component.h"
#include "ecs/world.h"

namespace simulation
{
static const std::unordered_map<StatType, FixedPoint> kDefaultStatsValues = {
    {StatType::kOmegaPowerPercentage, kDefaultOmegaPowerPercentage},
    {StatType::kOmegaDamagePercentage, kDefaultOmegaDamagePercentage},
    {StatType::kHealthGainEfficiencyPercentage, kDefaultHealthGainEfficiencyPercentage},
    {StatType::kEnergyGainEfficiencyPercentage, kDefaultEnergyGainEfficiencyPercentage},
    {StatType::kShieldGainEfficiencyPercentage, kDefaultShieldGainEfficiencyPercentage},
    {StatType::kVulnerabilityPercentage, kDefaultVulnerabilityPercentage},
    {StatType::kCritAmplificationPercentage, kDefaultCritAmplificationPercentage},
    {StatType::kCritChancePercentage, kDefaultCritChancePercentage},
    {StatType::kHitChancePercentage, kDefaultHitChancePercentage},
    {StatType::kAttackSpeed, kDefaultAttackSpeed},
};

void StatsHelper::SetDefaults(StatsData& stats_data)
{
    for (const auto& [stat_type, stat_value] : kDefaultStatsValues)
    {
        stats_data.Set(stat_type, stat_value);
    }
}

bool StatsHelper::GetDefaultStatValue(const StatType type, FixedPoint* out_value)
{
    if (!kDefaultStatsValues.contains(type))
    {
        return false;
    }

    *out_value = kDefaultStatsValues.at(type);
    return true;
}

bool StatsHelper::TimeStepOverflowStat(const Entity& entity, const StatsData& live_stats, const StatType stat_type)
{
    if (!entity.Has<StatsComponent>())
    {
        return false;
    }

    auto& stats_component = entity.Get<StatsComponent>();

    // Increase current
    const FixedPoint live_value = live_stats.Get(stat_type);
    stats_component.IncreaseOverflowForType(stat_type, live_value);

    // Overflowed
    const FixedPoint current_overflow = stats_component.GetOverflowForType(stat_type);
    if (current_overflow >= FixedPoint::FromInt(kRandomOverflowStatMax))
    {
        stats_component.DecreaseOverflowForType(stat_type, FixedPoint::FromInt(kRandomOverflowStatMax));
        return true;
    }

    // Did not overflow yet
    return false;
}

StatsData StatsHelper::CreateRandomModifierStats(World& world, const StatsData& stats)
{
    StatsData random_modifier_data;
    for (const StatType stat : kRandomCombatStatTypes)
    {
        const int roll = world.RandomRange(0, kRandomStatModifierMaxRoll + 1);
        const FixedPoint percentage = FixedPoint::FromInt(kRandomStatModifierRollIncrementPercentage * roll);

        if (stat == StatType::kAttackDamage)
        {
            // NOTE: The value for this roll affects each of the underlying values equally.
            // https://illuvium.atlassian.net/wiki/spaces/GD/pages/44204731/Random+Combat+Stat+Modifiers
            for (const StatType attack_stat_type : kAttackDamageStatTypes)
            {
                random_modifier_data.Set(attack_stat_type, percentage.AsPercentageOf(stats.Get(attack_stat_type)));
            }
        }
        else
        {
            random_modifier_data.Set(stat, percentage.AsPercentageOf(stats.Get(stat)));
        }
    }

    return random_modifier_data;
}

}  // namespace simulation
