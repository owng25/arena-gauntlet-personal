#include "stats_component.h"

#include "ecs/world.h"

namespace simulation
{
StatsData StatsComponent::GetBaseStats() const
{
    StatsData base_stats;

    // Iterate over all fields
    constexpr int start_index = static_cast<int>(StatType::kNone) + 1;
    constexpr int end_index = static_cast<int>(StatType::kNum);
    for (int stat_index = start_index; stat_index < end_index; stat_index++)
    {
        const StatType stat = static_cast<StatType>(stat_index);

        // Compute the base value
        const FixedPoint value = GetBaseValueForType(stat);

        // Set the base value
        base_stats.Set(stat, value);
    }

    return base_stats;
}

FixedPoint StatsComponent::GetAttackDamage() const
{
    assert(GetOwnerWorld());
    const auto& live_stats = GetOwnerWorld()->GetLiveStats(*GetOwnerEntity());
    const FixedPoint physical_damage = live_stats.Get(StatType::kAttackPhysicalDamage);
    const FixedPoint energy_damage = live_stats.Get(StatType::kAttackEnergyDamage);
    const FixedPoint pure_damage = live_stats.Get(StatType::kAttackPureDamage);
    return (physical_damage + energy_damage + pure_damage);
}

FixedPoint StatsComponent::GetMaxHealth() const
{
    assert(GetOwnerWorld());
    return GetOwnerWorld()->GetLiveStats(*GetOwnerEntity()).Get(StatType::kMaxHealth);
}

FixedPoint StatsComponent::GetEnergyCost() const
{
    assert(GetOwnerWorld());
    return GetOwnerWorld()->GetLiveStats(*GetOwnerEntity()).Get(StatType::kEnergyCost);
}

}  // namespace simulation
