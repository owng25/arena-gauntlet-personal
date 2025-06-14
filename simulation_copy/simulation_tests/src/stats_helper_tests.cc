#include "gtest/gtest.h"
#include "utility/stats_helper.h"

namespace simulation
{
TEST(StatsHelper, GetDamageAmplificationFromAttackSpeed)
{
    auto compute_amp_with_attack_speed = [](int live, int limit) -> FixedPoint
    {
        FullStatsData full_stats;
        full_stats.live.Set(StatType::kAttackSpeed, FixedPoint::FromInt(live));
        return StatsHelper::GetDamageAmplificationForAbilityType(
            AbilityType::kAttack,
            full_stats,
            FixedPoint::FromInt(limit));
    };

    ASSERT_EQ(compute_amp_with_attack_speed(0, 200), 0_fp);
    ASSERT_EQ(compute_amp_with_attack_speed(100, 200), 0_fp);
    ASSERT_EQ(compute_amp_with_attack_speed(199, 200), 0_fp);
    ASSERT_EQ(compute_amp_with_attack_speed(200, 200), 0_fp);
    ASSERT_EQ(compute_amp_with_attack_speed(250, 200), 25_fp);
    ASSERT_EQ(compute_amp_with_attack_speed(300, 200), 50_fp);
    ASSERT_EQ(compute_amp_with_attack_speed(400, 200), 100_fp);
}

TEST(StatsHelper, GetDamageAmplificationFromOmegaDamagePercentage)
{
    auto compute_amp_omega_damage_percentage = [](int live) -> FixedPoint
    {
        FullStatsData full_stats;
        full_stats.live.Set(StatType::kOmegaDamagePercentage, FixedPoint::FromInt(live));
        return StatsHelper::GetDamageAmplificationForAbilityType(AbilityType::kOmega, full_stats, 200_fp);
    };

    ASSERT_EQ(compute_amp_omega_damage_percentage(100), 0_fp);
    ASSERT_EQ(compute_amp_omega_damage_percentage(115), 15_fp);
    ASSERT_EQ(compute_amp_omega_damage_percentage(80), -20_fp);
    ASSERT_EQ(compute_amp_omega_damage_percentage(200), 100_fp);
}

}  // namespace simulation
