
#include "ability_system_data_fixtures.h"
#include "base_test_fixtures.h"
#include "components/focus_component.h"
#include "components/stats_component.h"
#include "data/effect_data.h"
#include "data/effect_package.h"
#include "gtest/gtest.h"
#include "systems/ability_system.h"
#include "utility/effect_package_helper.h"

namespace simulation
{

class EffectPackageTests : public BaseTest
{
};

class EffectPackageHelperTests : public BaseTest
{
};

class EffectPackageHelperMergeEmpowerEffectPackageTests : public EffectPackageHelperTests
{
};

class EffectPackageAttributesTests : public BaseTest
{
};

TEST(EffectDataAttributes, Add)
{
    const EffectDataAttributes default_attributes;

    // Empty + Empty
    {
        EffectDataAttributes attributes;
        EffectDataAttributes other;
        attributes += other;

        EXPECT_EQ(attributes.excess_heal_to_shield, default_attributes.excess_heal_to_shield);
        EXPECT_EQ(
            attributes.missing_health_percentage_to_health,
            default_attributes.missing_health_percentage_to_health);
        EXPECT_EQ(attributes.to_shield_percentage, default_attributes.to_shield_percentage);
        EXPECT_EQ(attributes.to_shield_duration_ms, default_attributes.to_shield_duration_ms);
        EXPECT_EQ(attributes.max_health_percentage_to_health, default_attributes.max_health_percentage_to_health);
        EXPECT_EQ(attributes.cleanse_negative_states, default_attributes.cleanse_negative_states);
        EXPECT_EQ(attributes.cleanse_conditions, default_attributes.cleanse_conditions);
        EXPECT_EQ(attributes.cleanse_bots, default_attributes.cleanse_bots);
        EXPECT_EQ(attributes.cleanse_dots, default_attributes.cleanse_dots);
        EXPECT_EQ(attributes.cleanse_debuffs, default_attributes.cleanse_debuffs);
    }

    // Empty + other
    {
        EffectDataAttributes attributes;
        EffectDataAttributes other;
        other.excess_heal_to_shield = 3_fp;
        other.missing_health_percentage_to_health = 4_fp;
        other.to_shield_percentage = 6_fp;
        other.to_shield_duration_ms = 7;
        other.max_health_percentage_to_health = 8_fp;
        other.cleanse_negative_states = true;
        other.cleanse_conditions = true;
        other.cleanse_bots = true;
        other.cleanse_dots = true;
        other.cleanse_debuffs = true;

        attributes += other;

        EXPECT_EQ(attributes.excess_heal_to_shield, other.excess_heal_to_shield);
        EXPECT_EQ(attributes.missing_health_percentage_to_health, other.missing_health_percentage_to_health);
        EXPECT_EQ(attributes.to_shield_percentage, other.to_shield_percentage);
        EXPECT_EQ(attributes.to_shield_duration_ms, other.to_shield_duration_ms);
        EXPECT_EQ(attributes.max_health_percentage_to_health, other.max_health_percentage_to_health);
        EXPECT_EQ(attributes.cleanse_negative_states, other.cleanse_negative_states);
        EXPECT_EQ(attributes.cleanse_conditions, other.cleanse_conditions);
        EXPECT_EQ(attributes.cleanse_dots, other.cleanse_dots);
        EXPECT_EQ(attributes.cleanse_debuffs, other.cleanse_debuffs);
    }

    // Something + other
    {
        EffectDataAttributes attributes;
        attributes.excess_heal_to_shield = 13_fp;
        attributes.missing_health_percentage_to_health = 14_fp;
        attributes.to_shield_percentage = 16_fp;
        attributes.to_shield_duration_ms = 17;
        attributes.max_health_percentage_to_health = 18_fp;
        attributes.cleanse_negative_states = false;
        attributes.cleanse_conditions = true;
        attributes.cleanse_bots = false;
        attributes.cleanse_dots = true;
        attributes.cleanse_debuffs = false;

        EffectDataAttributes other;
        other.excess_heal_to_shield = 3_fp;
        other.missing_health_percentage_to_health = 4_fp;
        other.to_shield_percentage = 6_fp;
        other.to_shield_duration_ms = 7;
        other.max_health_percentage_to_health = 8_fp;
        other.cleanse_negative_states = true;
        other.cleanse_conditions = false;
        other.cleanse_bots = true;
        other.cleanse_dots = false;
        other.cleanse_debuffs = true;

        attributes += other;

        EXPECT_EQ(attributes.excess_heal_to_shield, 3_fp + 13_fp);
        EXPECT_EQ(attributes.missing_health_percentage_to_health, 4_fp + 14_fp);
        EXPECT_EQ(attributes.to_shield_percentage, 6_fp + 16_fp);
        EXPECT_EQ(attributes.to_shield_duration_ms, 7 + 17);
        EXPECT_EQ(attributes.max_health_percentage_to_health, 8_fp + 18_fp);
        EXPECT_EQ(attributes.cleanse_negative_states, true);
        EXPECT_EQ(attributes.cleanse_conditions, true);
        EXPECT_EQ(attributes.cleanse_dots, true);
        EXPECT_EQ(attributes.cleanse_debuffs, true);
    }
}

TEST_F(EffectPackageAttributesTests, Add)
{
    const EffectPackageAttributes default_attributes;

    // Empty + Empty
    {
        EffectPackageAttributes attributes;
        EffectPackageAttributes other;
        attributes += other;

        EXPECT_EQ(attributes.excess_vamp_to_shield, default_attributes.excess_vamp_to_shield);
        EXPECT_EQ(attributes.exploit_weakness, default_attributes.exploit_weakness);
        EXPECT_EQ(attributes.is_trueshot, default_attributes.is_trueshot);
        EXPECT_EQ(attributes.cumulative_damage, default_attributes.cumulative_damage);
        EXPECT_EQ(attributes.split_damage, default_attributes.split_damage);
        EXPECT_EQ(attributes.can_crit, default_attributes.can_crit);
        EXPECT_EQ(attributes.always_crit, default_attributes.always_crit);
        EXPECT_EQ(attributes.rotate_to_target, default_attributes.rotate_to_target);
        EXPECT_EQ(
            EvaluateNoStats(attributes.piercing_percentage),
            EvaluateNoStats(default_attributes.piercing_percentage));
        EXPECT_EQ(EvaluateNoStats(attributes.refund_health), EvaluateNoStats(default_attributes.refund_health));
        EXPECT_EQ(EvaluateNoStats(attributes.refund_energy), EvaluateNoStats(default_attributes.refund_energy));
        EXPECT_EQ(attributes.rotate_to_target, default_attributes.rotate_to_target);
        EXPECT_EQ(attributes.split_damage, default_attributes.split_damage);
        EXPECT_EQ(attributes.is_trueshot, default_attributes.is_trueshot);
        EXPECT_EQ(
            EvaluateNoStats(attributes.damage_amplification),
            EvaluateNoStats(default_attributes.damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.damage_bonus), EvaluateNoStats(default_attributes.damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_damage_amplification),
            EvaluateNoStats(default_attributes.energy_damage_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_damage_bonus),
            EvaluateNoStats(default_attributes.energy_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.physical_damage_amplification),
            EvaluateNoStats(default_attributes.physical_damage_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.physical_damage_bonus),
            EvaluateNoStats(default_attributes.physical_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.pure_damage_amplification),
            EvaluateNoStats(default_attributes.pure_damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.pure_damage_bonus), EvaluateNoStats(default_attributes.pure_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_gain_amplification),
            EvaluateNoStats(default_attributes.energy_gain_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_gain_bonus), EvaluateNoStats(default_attributes.energy_gain_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.heal_amplification),
            EvaluateNoStats(default_attributes.heal_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.heal_bonus), EvaluateNoStats(default_attributes.heal_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_burn_amplification),
            EvaluateNoStats(default_attributes.energy_burn_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_burn_bonus), EvaluateNoStats(default_attributes.energy_burn_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.crit_reduction_piercing_percentage),
            EvaluateNoStats(default_attributes.crit_reduction_piercing_percentage));
        EXPECT_EQ(
            EvaluateNoStats(attributes.shield_amplification),
            EvaluateNoStats(default_attributes.shield_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.shield_bonus), EvaluateNoStats(default_attributes.shield_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.vampiric_percentage),
            EvaluateNoStats(default_attributes.vampiric_percentage));
        EXPECT_EQ(attributes.excess_vamp_to_shield_duration_ms, default_attributes.excess_vamp_to_shield_duration_ms);
    }

    // Empty + other
    {
        EffectPackageAttributes attributes;
        EffectPackageAttributes other;
        other.excess_vamp_to_shield = true;
        other.exploit_weakness = true;
        other.is_trueshot = true;
        other.cumulative_damage = true;
        other.split_damage = true;
        other.can_crit = true;
        other.always_crit = true;
        other.rotate_to_target = true;
        other.piercing_percentage = EffectExpression::FromValue(2_fp);
        other.refund_health = EffectExpression::FromValue(21_fp);
        other.refund_energy = EffectExpression::FromValue(22_fp);
        other.damage_amplification = EffectExpression::FromValue(3_fp);
        other.damage_bonus = EffectExpression::FromValue(7_fp);
        other.energy_damage_amplification = EffectExpression::FromValue(50_fp);
        other.energy_damage_bonus = EffectExpression::FromValue(25_fp);
        other.physical_damage_amplification = EffectExpression::FromValue(36_fp);
        other.physical_damage_bonus = EffectExpression::FromValue(76_fp);
        other.pure_damage_amplification = EffectExpression::FromValue(48_fp);
        other.pure_damage_bonus = EffectExpression::FromValue(99_fp);
        other.energy_gain_amplification = EffectExpression::FromValue(5_fp);
        other.energy_gain_bonus = EffectExpression::FromValue(12_fp);
        other.heal_amplification = EffectExpression::FromValue(3_fp);
        other.heal_bonus = EffectExpression::FromValue(2_fp);
        other.energy_burn_amplification = EffectExpression::FromValue(5_fp);
        other.energy_burn_bonus = EffectExpression::FromValue(7_fp);
        other.shield_bonus = EffectExpression::FromValue(2_fp);
        other.shield_amplification = EffectExpression::FromValue(15_fp);
        other.crit_reduction_piercing_percentage = EffectExpression::FromValue(16_fp);
        other.vampiric_percentage = EffectExpression::FromValue(17_fp);
        other.excess_vamp_to_shield_duration_ms = 18;
        attributes += other;

        EXPECT_EQ(attributes.excess_vamp_to_shield, true);
        EXPECT_EQ(attributes.exploit_weakness, true);
        EXPECT_EQ(attributes.is_trueshot, true);
        EXPECT_EQ(attributes.cumulative_damage, true);
        EXPECT_EQ(attributes.split_damage, true);
        EXPECT_EQ(attributes.can_crit, true);
        EXPECT_EQ(attributes.always_crit, true);
        EXPECT_EQ(attributes.rotate_to_target, true);
        EXPECT_EQ(EvaluateNoStats(attributes.piercing_percentage), EvaluateNoStats(other.piercing_percentage));
        EXPECT_EQ(EvaluateNoStats(attributes.refund_health), EvaluateNoStats(other.refund_health));
        EXPECT_EQ(EvaluateNoStats(attributes.refund_energy), EvaluateNoStats(other.refund_energy));
        EXPECT_EQ(EvaluateNoStats(attributes.damage_amplification), EvaluateNoStats(other.damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.damage_bonus), EvaluateNoStats(other.damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_damage_amplification),
            EvaluateNoStats(other.energy_damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_damage_bonus), EvaluateNoStats(other.energy_damage_bonus));
        EXPECT_EQ(EvaluateNoStats(attributes.physical_damage_bonus), EvaluateNoStats(other.physical_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.physical_damage_amplification),
            EvaluateNoStats(other.physical_damage_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.pure_damage_amplification),
            EvaluateNoStats(other.pure_damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.pure_damage_bonus), EvaluateNoStats(other.pure_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_gain_amplification),
            EvaluateNoStats(other.energy_gain_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_gain_bonus), EvaluateNoStats(other.energy_gain_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_burn_amplification),
            EvaluateNoStats(other.energy_burn_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_burn_bonus), EvaluateNoStats(other.energy_burn_bonus));
        EXPECT_EQ(EvaluateNoStats(attributes.shield_bonus), EvaluateNoStats(other.shield_bonus));
        EXPECT_EQ(EvaluateNoStats(attributes.shield_amplification), EvaluateNoStats(other.shield_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.crit_reduction_piercing_percentage),
            EvaluateNoStats(other.crit_reduction_piercing_percentage));
        EXPECT_EQ(EvaluateNoStats(attributes.vampiric_percentage), EvaluateNoStats(other.vampiric_percentage));
        EXPECT_EQ(attributes.excess_vamp_to_shield_duration_ms, other.excess_vamp_to_shield_duration_ms);
    }

    // Something + other
    {
        EffectPackageAttributes attributes;
        attributes.excess_vamp_to_shield = true;
        attributes.exploit_weakness = true;
        attributes.is_trueshot = false;
        attributes.cumulative_damage = true;
        attributes.split_damage = false;
        attributes.can_crit = true;
        attributes.always_crit = false;
        attributes.rotate_to_target = true;
        attributes.piercing_percentage = EffectExpression::FromValue(12_fp);
        attributes.refund_health = EffectExpression::FromValue(11_fp);
        attributes.refund_energy = EffectExpression::FromValue(12_fp);
        attributes.damage_amplification = EffectExpression::FromValue(1_fp);
        attributes.damage_bonus = EffectExpression::FromValue(2_fp);
        attributes.energy_damage_amplification = EffectExpression::FromValue(5_fp);
        attributes.energy_damage_bonus = EffectExpression::FromValue(7_fp);
        attributes.physical_damage_amplification = EffectExpression::FromValue(3_fp);
        attributes.physical_damage_bonus = EffectExpression::FromValue(2_fp);
        attributes.pure_damage_amplification = EffectExpression::FromValue(12_fp);
        attributes.pure_damage_bonus = EffectExpression::FromValue(1_fp);
        attributes.energy_gain_amplification = EffectExpression::FromValue(15_fp);
        attributes.energy_gain_bonus = EffectExpression::FromValue(1_fp);
        attributes.heal_amplification = EffectExpression::FromValue(7_fp);
        attributes.heal_bonus = EffectExpression::FromValue(3_fp);
        attributes.energy_burn_amplification = EffectExpression::FromValue(11_fp);
        attributes.energy_burn_bonus = EffectExpression::FromValue(1_fp);
        attributes.shield_bonus = EffectExpression::FromValue(2_fp);
        attributes.shield_amplification = EffectExpression::FromValue(15_fp);
        attributes.crit_reduction_piercing_percentage = EffectExpression::FromValue(16_fp);
        attributes.vampiric_percentage = EffectExpression::FromValue(1_fp);
        attributes.excess_vamp_to_shield_duration_ms = 2;

        EffectPackageAttributes other;
        other.excess_vamp_to_shield = false;
        other.exploit_weakness = false;
        other.is_trueshot = true;
        other.cumulative_damage = false;
        other.split_damage = true;
        other.can_crit = false;
        other.always_crit = true;
        other.rotate_to_target = false;
        other.piercing_percentage = EffectExpression::FromValue(2_fp);
        other.refund_health = EffectExpression::FromValue(21_fp);
        other.refund_energy = EffectExpression::FromValue(22_fp);
        other.damage_amplification = EffectExpression::FromValue(3_fp);
        other.damage_bonus = EffectExpression::FromValue(7_fp);
        other.energy_damage_amplification = EffectExpression::FromValue(50_fp);
        other.energy_damage_bonus = EffectExpression::FromValue(25_fp);
        other.physical_damage_amplification = EffectExpression::FromValue(36_fp);
        other.physical_damage_bonus = EffectExpression::FromValue(76_fp);
        other.pure_damage_amplification = EffectExpression::FromValue(48_fp);
        other.pure_damage_bonus = EffectExpression::FromValue(99_fp);
        other.energy_gain_amplification = EffectExpression::FromValue(5_fp);
        other.energy_gain_bonus = EffectExpression::FromValue(12_fp);
        other.heal_amplification = EffectExpression::FromValue(3_fp);
        other.heal_bonus = EffectExpression::FromValue(2_fp);
        other.energy_burn_amplification = EffectExpression::FromValue(5_fp);
        other.energy_burn_bonus = EffectExpression::FromValue(7_fp);
        other.shield_bonus = EffectExpression::FromValue(6_fp);
        other.shield_amplification = EffectExpression::FromValue(5_fp);
        other.crit_reduction_piercing_percentage = EffectExpression::FromValue(4_fp);
        other.vampiric_percentage = EffectExpression::FromValue(18_fp);
        other.excess_vamp_to_shield_duration_ms = 29;
        attributes += other;

        EXPECT_EQ(attributes.excess_vamp_to_shield, true);
        EXPECT_EQ(attributes.exploit_weakness, true);
        EXPECT_EQ(attributes.is_trueshot, true);
        EXPECT_EQ(attributes.cumulative_damage, true);
        EXPECT_EQ(attributes.split_damage, true);
        EXPECT_EQ(attributes.can_crit, true);
        EXPECT_EQ(attributes.always_crit, true);
        EXPECT_EQ(attributes.rotate_to_target, true);
        EXPECT_EQ(EvaluateNoStats(attributes.piercing_percentage), 2_fp + 12_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.refund_health), 11_fp + 21_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.refund_energy), 12_fp + 22_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.damage_amplification), 1_fp + 3_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.damage_bonus), 2_fp + 7_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_damage_amplification), 5_fp + 50_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_damage_bonus), 7_fp + 25_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.physical_damage_bonus), 76_fp + 2_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.physical_damage_amplification), 3_fp + 36_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.pure_damage_amplification), 12_fp + 48_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.pure_damage_bonus), 1_fp + 99_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_gain_amplification), 5_fp + 15_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_gain_bonus), 1_fp + 12_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.heal_amplification), 7_fp + 3_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.heal_bonus), 3_fp + 2_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_burn_amplification), 11_fp + 5_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_burn_bonus), 1_fp + 7_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.shield_amplification), 15_fp + 5_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.crit_reduction_piercing_percentage), 16_fp + 4_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.shield_bonus), 2_fp + 6_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.vampiric_percentage), 1_fp + 18_fp);
        EXPECT_EQ(attributes.excess_vamp_to_shield_duration_ms, 2 + 29);
    }
}

TEST_F(EffectPackageAttributesTests, Subtract)
{
    const EffectPackageAttributes default_attributes;

    // Empty - Empty
    {
        EffectPackageAttributes attributes;
        EffectPackageAttributes other;
        attributes -= other;

        EXPECT_EQ(attributes.is_trueshot, default_attributes.is_trueshot);
        EXPECT_EQ(attributes.cumulative_damage, default_attributes.cumulative_damage);
        EXPECT_EQ(attributes.split_damage, default_attributes.split_damage);
        EXPECT_EQ(attributes.can_crit, default_attributes.can_crit);
        EXPECT_EQ(attributes.always_crit, default_attributes.always_crit);
        EXPECT_EQ(attributes.rotate_to_target, default_attributes.rotate_to_target);
        EXPECT_EQ(EvaluateNoStats(attributes.refund_health), EvaluateNoStats(default_attributes.refund_health));
        EXPECT_EQ(EvaluateNoStats(attributes.refund_energy), EvaluateNoStats(default_attributes.refund_energy));
        EXPECT_EQ(attributes.rotate_to_target, default_attributes.rotate_to_target);
        EXPECT_EQ(attributes.split_damage, default_attributes.split_damage);
        EXPECT_EQ(attributes.is_trueshot, default_attributes.is_trueshot);
        EXPECT_EQ(
            EvaluateNoStats(attributes.damage_amplification),
            EvaluateNoStats(default_attributes.damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.damage_bonus), EvaluateNoStats(default_attributes.damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_damage_amplification),
            EvaluateNoStats(default_attributes.energy_damage_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_damage_bonus),
            EvaluateNoStats(default_attributes.energy_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.physical_damage_amplification),
            EvaluateNoStats(default_attributes.physical_damage_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.physical_damage_bonus),
            EvaluateNoStats(default_attributes.physical_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.pure_damage_amplification),
            EvaluateNoStats(default_attributes.pure_damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.pure_damage_bonus), EvaluateNoStats(default_attributes.pure_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_gain_amplification),
            EvaluateNoStats(default_attributes.energy_gain_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_gain_bonus), EvaluateNoStats(default_attributes.energy_gain_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.heal_amplification),
            EvaluateNoStats(default_attributes.heal_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.heal_bonus), EvaluateNoStats(default_attributes.heal_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_burn_amplification),
            EvaluateNoStats(default_attributes.energy_burn_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_burn_bonus), EvaluateNoStats(default_attributes.energy_burn_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.shield_amplification),
            EvaluateNoStats(default_attributes.shield_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.crit_reduction_piercing_percentage),
            EvaluateNoStats(default_attributes.crit_reduction_piercing_percentage));
        EXPECT_EQ(EvaluateNoStats(attributes.shield_bonus), EvaluateNoStats(default_attributes.shield_bonus));
    }

    // Empty - other
    {
        EffectPackageAttributes attributes;
        EffectPackageAttributes other;
        other.is_trueshot = true;
        other.cumulative_damage = true;
        other.split_damage = true;
        other.can_crit = true;
        other.always_crit = true;
        other.rotate_to_target = true;
        other.refund_health = EffectExpression::FromValue(21_fp);
        other.refund_energy = EffectExpression::FromValue(22_fp);
        other.damage_amplification = EffectExpression::FromValue(3_fp);
        other.damage_bonus = EffectExpression::FromValue(7_fp);
        other.energy_damage_amplification = EffectExpression::FromValue(50_fp);
        other.energy_damage_bonus = EffectExpression::FromValue(25_fp);
        other.physical_damage_amplification = EffectExpression::FromValue(36_fp);
        other.physical_damage_bonus = EffectExpression::FromValue(76_fp);
        other.pure_damage_amplification = EffectExpression::FromValue(48_fp);
        other.pure_damage_bonus = EffectExpression::FromValue(99_fp);
        other.energy_gain_amplification = EffectExpression::FromValue(5_fp);
        other.energy_gain_bonus = EffectExpression::FromValue(12_fp);
        other.heal_amplification = EffectExpression::FromValue(3_fp);
        other.heal_bonus = EffectExpression::FromValue(2_fp);
        other.energy_burn_amplification = EffectExpression::FromValue(5_fp);
        other.energy_burn_bonus = EffectExpression::FromValue(7_fp);
        other.shield_bonus = EffectExpression::FromValue(2_fp);
        other.shield_amplification = EffectExpression::FromValue(15_fp);
        other.crit_reduction_piercing_percentage = EffectExpression::FromValue(16_fp);
        attributes -= other;

        EXPECT_EQ(attributes.is_trueshot, true);
        EXPECT_EQ(attributes.cumulative_damage, true);
        EXPECT_EQ(attributes.split_damage, true);
        EXPECT_EQ(attributes.can_crit, true);
        EXPECT_EQ(attributes.always_crit, true);
        EXPECT_EQ(attributes.rotate_to_target, true);
        EXPECT_EQ(EvaluateNoStats(attributes.refund_health), -EvaluateNoStats(other.refund_health));
        EXPECT_EQ(EvaluateNoStats(attributes.refund_energy), -EvaluateNoStats(other.refund_energy));
        EXPECT_EQ(EvaluateNoStats(attributes.damage_amplification), -EvaluateNoStats(other.damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.damage_bonus), -EvaluateNoStats(other.damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_damage_amplification),
            -EvaluateNoStats(other.energy_damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_damage_bonus), -EvaluateNoStats(other.energy_damage_bonus));
        EXPECT_EQ(EvaluateNoStats(attributes.physical_damage_bonus), -EvaluateNoStats(other.physical_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.physical_damage_amplification),
            -EvaluateNoStats(other.physical_damage_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.pure_damage_amplification),
            -EvaluateNoStats(other.pure_damage_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.pure_damage_bonus), -EvaluateNoStats(other.pure_damage_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_gain_amplification),
            -EvaluateNoStats(other.energy_gain_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_gain_bonus), -EvaluateNoStats(other.energy_gain_bonus));
        EXPECT_EQ(
            EvaluateNoStats(attributes.energy_burn_amplification),
            -EvaluateNoStats(other.energy_burn_amplification));
        EXPECT_EQ(EvaluateNoStats(attributes.energy_burn_bonus), -EvaluateNoStats(other.energy_burn_bonus));
        EXPECT_EQ(EvaluateNoStats(attributes.shield_bonus), -EvaluateNoStats(other.shield_bonus));
        EXPECT_EQ(EvaluateNoStats(attributes.shield_amplification), -EvaluateNoStats(other.shield_amplification));
        EXPECT_EQ(
            EvaluateNoStats(attributes.crit_reduction_piercing_percentage),
            -EvaluateNoStats(other.crit_reduction_piercing_percentage));
    }

    // Something - other
    {
        EffectPackageAttributes attributes;
        attributes.is_trueshot = false;
        attributes.cumulative_damage = true;
        attributes.split_damage = false;
        attributes.can_crit = true;
        attributes.always_crit = false;
        attributes.rotate_to_target = true;
        attributes.refund_health = EffectExpression::FromValue(11_fp);
        attributes.refund_energy = EffectExpression::FromValue(12_fp);
        attributes.damage_amplification = EffectExpression::FromValue(1_fp);
        attributes.damage_bonus = EffectExpression::FromValue(2_fp);
        attributes.energy_damage_amplification = EffectExpression::FromValue(5_fp);
        attributes.energy_damage_bonus = EffectExpression::FromValue(7_fp);
        attributes.physical_damage_amplification = EffectExpression::FromValue(3_fp);
        attributes.physical_damage_bonus = EffectExpression::FromValue(2_fp);
        attributes.pure_damage_amplification = EffectExpression::FromValue(12_fp);
        attributes.pure_damage_bonus = EffectExpression::FromValue(1_fp);
        attributes.energy_gain_amplification = EffectExpression::FromValue(15_fp);
        attributes.energy_gain_bonus = EffectExpression::FromValue(1_fp);
        attributes.heal_amplification = EffectExpression::FromValue(7_fp);
        attributes.heal_bonus = EffectExpression::FromValue(3_fp);
        attributes.energy_burn_amplification = EffectExpression::FromValue(11_fp);
        attributes.energy_burn_bonus = EffectExpression::FromValue(1_fp);
        attributes.shield_bonus = EffectExpression::FromValue(2_fp);
        attributes.shield_amplification = EffectExpression::FromValue(15_fp);
        attributes.crit_reduction_piercing_percentage = EffectExpression::FromValue(16_fp);

        EffectPackageAttributes other;
        other.is_trueshot = true;
        other.cumulative_damage = false;
        other.split_damage = true;
        other.can_crit = false;
        other.always_crit = true;
        other.rotate_to_target = false;
        other.refund_health = EffectExpression::FromValue(21_fp);
        other.refund_energy = EffectExpression::FromValue(22_fp);
        other.damage_amplification = EffectExpression::FromValue(3_fp);
        other.damage_bonus = EffectExpression::FromValue(7_fp);
        other.energy_damage_amplification = EffectExpression::FromValue(50_fp);
        other.energy_damage_bonus = EffectExpression::FromValue(25_fp);
        other.physical_damage_amplification = EffectExpression::FromValue(36_fp);
        other.physical_damage_bonus = EffectExpression::FromValue(76_fp);
        other.pure_damage_amplification = EffectExpression::FromValue(48_fp);
        other.pure_damage_bonus = EffectExpression::FromValue(99_fp);
        other.energy_gain_amplification = EffectExpression::FromValue(5_fp);
        other.energy_gain_bonus = EffectExpression::FromValue(12_fp);
        other.heal_amplification = EffectExpression::FromValue(3_fp);
        other.heal_bonus = EffectExpression::FromValue(2_fp);
        other.energy_burn_amplification = EffectExpression::FromValue(5_fp);
        other.energy_burn_bonus = EffectExpression::FromValue(7_fp);
        other.shield_bonus = EffectExpression::FromValue(6_fp);
        other.shield_amplification = EffectExpression::FromValue(5_fp);
        other.crit_reduction_piercing_percentage = EffectExpression::FromValue(2_fp);
        attributes -= other;

        EXPECT_EQ(attributes.is_trueshot, true);
        EXPECT_EQ(attributes.cumulative_damage, true);
        EXPECT_EQ(attributes.split_damage, true);
        EXPECT_EQ(attributes.can_crit, true);
        EXPECT_EQ(attributes.always_crit, true);
        EXPECT_EQ(attributes.rotate_to_target, true);
        EXPECT_EQ(EvaluateNoStats(attributes.refund_health), 11_fp - 21_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.refund_energy), 12_fp - 22_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.damage_amplification), 1_fp - 3_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.damage_bonus), 2_fp - 7_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_damage_amplification), 5_fp - 50_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_damage_bonus), 7_fp - 25_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.physical_damage_bonus), 2_fp - 76_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.physical_damage_amplification), 3_fp - 36_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.pure_damage_amplification), 12_fp - 48_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.pure_damage_bonus), 1_fp - 99_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_gain_amplification), 15_fp - 5_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_gain_bonus), 1_fp - 12_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.heal_amplification), 7_fp - 3_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.heal_bonus), 3_fp - 2_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_burn_amplification), 11_fp - 5_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.energy_burn_bonus), 1_fp - 7_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.shield_amplification), 15_fp - 5_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.crit_reduction_piercing_percentage), 16_fp - 2_fp);
        EXPECT_EQ(EvaluateNoStats(attributes.shield_bonus), 2_fp - 6_fp);
    }
}

TEST_F(EffectPackageHelperMergeEmpowerEffectPackageTests, AddEffect)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect
    EffectPackage effect_package;
    effect_package.effects = {
        EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp))};

    EffectPackage empower_effect_package;
    {
        auto damage_effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
        empower_effect_package.effects = {damage_effect};
        empower_effect_package.attributes.vampiric_percentage = EffectExpression::FromValue(20_fp);
    }

    world->GetEffectPackageHelper().MergeEmpowerEffectPackage(empower_effect_package, &effect_package);

    ASSERT_EQ(effect_package.effects.size(), 2);
    EXPECT_EQ(effect_package.attributes.IsEmpty(), false);  // not empty because vampiric is in package

    // Original effect should have remained intact
    {
        EXPECT_EQ(EvaluateNoStats(effect_package.attributes.vampiric_percentage), 20_fp);

        const auto& effect = effect_package.effects.at(0);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect.GetExpression().base_value.value, 200_fp);
    }

    // New effect added from empower
    {
        const auto& effect = effect_package.effects.at(1);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect.GetExpression().base_value.value, 100_fp);
    }
}

TEST_F(EffectPackageHelperMergeEmpowerEffectPackageTests, AddGlobalEffectAttribute)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect-Attribute
    EffectPackage effect_package;
    effect_package.effects = {
        EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp))};

    EffectPackage empower_effect_package;
    // Add empower physical damage
    {
        auto vampiric_effect_damage =
            EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
        empower_effect_package.AddEffect(vampiric_effect_damage);
    }

    // Add empower energy damage that should not be affected by the global effect attribute
    {
        auto vampiric_effect_damage =
            EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(121_fp));
        empower_effect_package.AddEffect(vampiric_effect_damage);
    }

    // Add global attribute
    {
        EffectDataAttributes global_attributes;
        global_attributes.excess_heal_to_shield = 42_fp;

        EffectTypeID empower_for_effect_type_id;
        empower_for_effect_type_id.type = EffectType::kInstantDamage;
        empower_for_effect_type_id.damage_type = EffectDamageType::kPhysical;

        const auto global_effect_attribute =
            CreateEmpowerGlobalEffectAttribute(AbilityType::kOmega, 2, empower_for_effect_type_id, global_attributes);
        empower_effect_package.AddEffect(global_effect_attribute);
    }

    world->GetEffectPackageHelper().MergeEmpowerEffectPackage(empower_effect_package, &effect_package);

    ASSERT_EQ(effect_package.effects.size(), 3);
    EXPECT_EQ(effect_package.attributes.IsEmpty(), true);

    // Original effect is affected by the global attribute
    {
        const auto& effect = effect_package.effects.at(0);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect.GetExpression().base_value.value, 200_fp);
        EXPECT_EQ(effect.attributes.excess_heal_to_shield, 42_fp);
    }

    // Empower physical damage is also affected by the global attribute
    {
        const auto& effect = effect_package.effects.at(1);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect.GetExpression().base_value.value, 100_fp);
        EXPECT_EQ(effect.attributes.excess_heal_to_shield, 42_fp);
    }

    // Empower Energy damage is NOT affected
    {
        const auto& effect = effect_package.effects.at(2);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kEnergy);
        EXPECT_EQ(effect.GetExpression().base_value.value, 121_fp);
        EXPECT_EQ(effect.attributes.excess_heal_to_shield, 0_fp);
    }
}

TEST_F(EffectPackageHelperMergeEmpowerEffectPackageTests, AddEffectPackageAttribute)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect-Package-Attribute
    EffectPackage effect_package;
    effect_package.attributes.refund_energy = EffectExpression::FromValue(20_fp);
    effect_package.attributes.always_crit = true;
    effect_package.effects = {
        EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp))};

    EffectPackage empower_effect_package;
    empower_effect_package.attributes.refund_energy = EffectExpression::FromValue(20_fp);
    empower_effect_package.attributes.is_trueshot = true;

    world->GetEffectPackageHelper().MergeEmpowerEffectPackage(empower_effect_package, &effect_package);

    ASSERT_EQ(effect_package.effects.size(), 1);
    EXPECT_EQ(effect_package.attributes.IsEmpty(), false);

    // Original effect should have remained intact
    {
        const auto& effect = effect_package.effects.at(0);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect.GetExpression().base_value.value, 200_fp);
    }

    // Effect package attributes should have changed
    EXPECT_EQ(effect_package.attributes.always_crit, true);
    EXPECT_EQ(EvaluateNoStats(effect_package.attributes.refund_energy), 40_fp);
    EXPECT_EQ(effect_package.attributes.is_trueshot, true);
}

TEST_F(EffectPackageHelperMergeEmpowerEffectPackageTests, AllTypes)
{
    // Test that combines all of them.
    // Follows the diagram here
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect-Package-Attribute
    EffectPackage effect_package;
    effect_package.attributes.refund_health = EffectExpression::FromValue(50_fp);  // A
    effect_package.attributes.is_trueshot = true;                                  // B
    effect_package.attributes.cumulative_damage = true;                            // C
    effect_package.attributes.can_crit = true;                                     // D

    // Fill existing effect package
    {
        // Effect 1
        auto damage_effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp));
        damage_effect.attributes.excess_heal_to_shield = 20_fp;  // Effect Attribute 1
        damage_effect.attributes.to_shield_duration_ms = 10;     // Effect Attribute 2

        effect_package.AddEffect(damage_effect);
    }
    {
        // Effect 2
        auto damage_effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(350_fp));
        damage_effect.attributes.to_shield_percentage = 5_fp;  // Effect Attribute 3

        effect_package.AddEffect(damage_effect);
    }
    {
        // Effect 3
        const auto negative_state = EffectData::CreateNegativeState(EffectNegativeState::kStun, 700);
        effect_package.AddEffect(negative_state);
    }

    EffectPackage empower_effect_package;
    empower_effect_package.attributes.refund_health = EffectExpression::FromValue(20_fp);  // A
    empower_effect_package.attributes.refund_energy = EffectExpression::FromValue(10_fp);  // E

    // Fill empower effect package
    {
        // Effect 1
        auto damage_effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(150_fp));
        damage_effect.attributes.excess_heal_to_shield = 7_fp;  // Effect Attribute 1

        empower_effect_package.AddEffect(damage_effect);
    }
    {
        // Effect 4
        const auto positive_state = EffectData::CreatePositiveState(EffectPositiveState::kTruesight, 500);
        empower_effect_package.AddEffect(positive_state);
    }

    // Fill global effect attributes
    {
        // Effect Attribute (Effect 1. Effect Attribute 2)
        EffectDataAttributes global_attributes;
        global_attributes.to_shield_duration_ms = 3;

        EffectTypeID empower_for_effect_type_id;
        empower_for_effect_type_id.type = EffectType::kInstantDamage;
        empower_for_effect_type_id.damage_type = EffectDamageType::kPhysical;

        const auto global_effect_attribute =
            CreateEmpowerGlobalEffectAttribute(AbilityType::kOmega, 2, empower_for_effect_type_id, global_attributes);
        empower_effect_package.AddEffect(global_effect_attribute);
    }
    {
        // Effect Attribute (Effect 3. Effect Attribute 4)
        EffectDataAttributes global_attributes;
        global_attributes.cleanse_bots = true;

        EffectTypeID empower_for_effect_type_id;
        empower_for_effect_type_id.type = EffectType::kNegativeState;
        empower_for_effect_type_id.negative_state = EffectNegativeState::kStun;

        const auto global_effect_attribute =
            CreateEmpowerGlobalEffectAttribute(AbilityType::kOmega, 2, empower_for_effect_type_id, global_attributes);
        empower_effect_package.AddEffect(global_effect_attribute);
    }

    world->GetEffectPackageHelper().MergeEmpowerEffectPackage(empower_effect_package, &effect_package);

    // Check effect package attributes
    EXPECT_EQ(EvaluateNoStats(effect_package.attributes.refund_health), 70_fp);  // A
    EXPECT_EQ(effect_package.attributes.is_trueshot, true);                      // B
    EXPECT_EQ(effect_package.attributes.cumulative_damage, true);                // C
    EXPECT_EQ(effect_package.attributes.can_crit, true);                         // D
    EXPECT_EQ(EvaluateNoStats(effect_package.attributes.refund_energy), 10_fp);  // E

    //
    // Check effects
    //
    ASSERT_EQ(effect_package.effects.size(), 5);

    // Effect 1
    {
        const auto& effect = effect_package.effects.at(0);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect.GetExpression().base_value.value, 200_fp);

        // Effect attribute 1
        EXPECT_EQ(effect.attributes.excess_heal_to_shield, 20_fp);

        // 3 from the global effect attribute
        // Effect Attribute 2 + Effect Attribute 2
        EXPECT_EQ(effect.attributes.to_shield_duration_ms, 13);
    }

    // Effect 2
    {
        const auto& effect = effect_package.effects.at(1);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kEnergy);
        EXPECT_EQ(effect.GetExpression().base_value.value, 350_fp);

        // Effect attribute 3
        EXPECT_EQ(effect.attributes.to_shield_percentage, 5_fp);

        EXPECT_EQ(effect.attributes.excess_heal_to_shield, 0_fp);
        EXPECT_EQ(effect.attributes.to_shield_duration_ms, 0);
    }

    // Effect 3
    {
        const auto& effect = effect_package.effects.at(2);
        EXPECT_EQ(effect.type_id.type, EffectType::kNegativeState);
        EXPECT_EQ(effect.type_id.negative_state, EffectNegativeState::kStun);
        EXPECT_EQ(effect.lifetime.duration_time_ms, 700);

        // Effect attribute 4
        EXPECT_EQ(effect.attributes.cleanse_bots, true);

        EXPECT_EQ(effect.attributes.to_shield_percentage, 0_fp);
        EXPECT_EQ(effect.attributes.excess_heal_to_shield, 0_fp);
        EXPECT_EQ(effect.attributes.to_shield_duration_ms, 0);
    }

    // Effect 1 from empower
    {
        const auto& effect = effect_package.effects.at(3);
        EXPECT_EQ(effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect.GetExpression().base_value.value, 150_fp);

        // Effect Attribute 1
        EXPECT_EQ(effect.attributes.excess_heal_to_shield, 7_fp);

        // 3 from the global effect attribute
        // Effect attribute 2
        EXPECT_EQ(effect.attributes.to_shield_duration_ms, 3);
    }

    // Effect 4 from empower
    {
        const auto& effect = effect_package.effects.at(4);
        EXPECT_EQ(effect.type_id.type, EffectType::kPositiveState);
        EXPECT_EQ(effect.type_id.positive_state, EffectPositiveState::kTruesight);
        EXPECT_EQ(effect.lifetime.duration_time_ms, 500);
        EXPECT_EQ(effect.attributes.cleanse_bots, false);
        EXPECT_EQ(effect.attributes.to_shield_percentage, 0_fp);
        EXPECT_EQ(effect.attributes.excess_heal_to_shield, 0_fp);
        EXPECT_EQ(effect.attributes.to_shield_duration_ms, 0);
    }
}

TEST_F(AbilitySystemTestRefundHealthAndEnergy, RefundEnergyMinEfficiency)
{
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Set energy to 0 and confirm
    blue_template_stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMinPercentageFP)
        .Set(StatType::kCurrentEnergy, 0_fp)
        .Set(StatType::kEnergyCost, 100_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    attack_ability->skills.at(0).targeting_state.CreateFromTarget(*world, red_entity->GetID(), blue_entity->GetID());
    ability_system.DeploySkill(*blue_entity, blue_entity->GetID(), attack_ability, attack_ability->skills.at(0));

    // 0 energy gain due to min energy_gain_efficiency_percentage and refund_energy = 50
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);
}

TEST_F(AbilitySystemTestRefundHealthAndEnergy, RefundEnergySomeEfficiency)
{
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Set energy to 0 and confirm
    blue_template_stats.Set(StatType::kEnergyGainEfficiencyPercentage, 75_fp)
        .Set(StatType::kCurrentEnergy, 0_fp)
        .Set(StatType::kEnergyCost, 100_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    attack_ability->skills.at(0).targeting_state.CreateFromTarget(*world, red_entity->GetID(), blue_entity->GetID());
    ability_system.DeploySkill(*blue_entity, blue_entity->GetID(), attack_ability, attack_ability->skills.at(0));

    // 37.5 energy gain due to 75 energy_gain_efficiency_percentage and refund_energy = 50
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 45_fp);
}

TEST_F(AbilitySystemTestRefundHealthAndEnergy, RefundEnergyMaxEfficiency)
{
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Set energy to 0 and confirm
    blue_template_stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP)
        .Set(StatType::kCurrentEnergy, 0_fp)
        .Set(StatType::kEnergyCost, 100_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    attack_ability->skills.at(0).targeting_state.CreateFromTarget(*world, red_entity->GetID(), blue_entity->GetID());
    ability_system.DeploySkill(*blue_entity, blue_entity->GetID(), attack_ability, attack_ability->skills.at(0));

    // 50 energy gain due to max energy_gain_efficiency_percentage and refund_energy = 50
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 60_fp);
}
TEST_F(AbilitySystemTestBonusAndAmpDamage, PhysicalDamageBonus)
{
    EffectPackageAttributes attributes;
    attributes.physical_damage_bonus = EffectExpression::FromValue(50_fp);
    attributes.damage_bonus = EffectExpression::FromValue(10_fp);

    InitAttackAbilityData(EffectDamageType::kPhysical, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 100 damage should be done, 50 for physical_damage_bonus and 50 from effect, 10 from damage_bonus
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 390_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, EnergyDamageBonus)
{
    EffectPackageAttributes attributes;
    attributes.energy_damage_bonus = EffectExpression::FromValue(50_fp);
    attributes.damage_bonus = EffectExpression::FromValue(10_fp);

    InitAttackAbilityData(EffectDamageType::kEnergy, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 100 damage should be done, 50 for energy_damage_bonus and 50 from effect, 10 from damage_bonus
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 390_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, PureDamageBonus)
{
    EffectPackageAttributes attributes;
    attributes.pure_damage_bonus = EffectExpression::FromValue(50_fp);
    attributes.damage_bonus = EffectExpression::FromValue(10_fp);

    InitAttackAbilityData(EffectDamageType::kPure, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 100 damage should be done, 50 for pure_damage_bonus and 50 from effect, 10 from damage_bonus
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 390_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, PhysicalDamageAmp)
{
    EffectPackageAttributes attributes;
    attributes.physical_damage_amplification = EffectExpression::FromValue(50_fp);
    attributes.damage_amplification = EffectExpression::FromValue(20_fp);

    InitAttackAbilityData(EffectDamageType::kPhysical, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 85 damage should be done, 35 for 70% (physical_damage_amplification + damage_amplification) and
    // 50 from effect
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 415_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, EnergyDamageAmp)
{
    EffectPackageAttributes attributes;
    attributes.energy_damage_amplification = EffectExpression::FromValue(50_fp);
    attributes.damage_amplification = EffectExpression::FromValue(20_fp);

    InitAttackAbilityData(EffectDamageType::kEnergy, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 85 damage should be done, 35 for 70% (energy_damage_amplification + damage_amplification) and 50
    // from effect
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 415_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, PureDamageAmp)
{
    EffectPackageAttributes attributes;
    attributes.pure_damage_amplification = EffectExpression::FromValue(50_fp);
    attributes.damage_amplification = EffectExpression::FromValue(20_fp);

    InitAttackAbilityData(EffectDamageType::kPure, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 85 damage should be done, 35 for 70% (pure_damage_amplification + damage_amplification) and 50
    // from effect
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 415_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, PureDamageAmpAndBonus)
{
    EffectPackageAttributes attributes;
    attributes.pure_damage_amplification = EffectExpression::FromValue(50_fp);
    attributes.pure_damage_bonus = EffectExpression::FromValue(50_fp);
    attributes.damage_amplification = EffectExpression::FromValue(25_fp);
    attributes.damage_bonus = EffectExpression::FromValue(30_fp);

    InitAttackAbilityData(EffectDamageType::kPure, attributes);
    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 227 damage should be done, 80 for (pure_damage_bonus + damage_bonus), 97 for
    // (pure_damage_amplification + damage_amplification) and 50 from effect 50 (effect) + 80 (bonus) =
    // 130 * 75% (amp) = 227
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "272.5"_fp);
}

TEST_F(AbilitySystemTestEffectPackage, BonusAndAmpEnergyGain)
{
    EffectPackageAttributes attributes;
    attributes.energy_gain_bonus = EffectExpression::FromValue(50_fp);
    attributes.energy_gain_amplification = EffectExpression::FromValue(25_fp);
    const EffectData effect_data =
        EffectData::Create(EffectType::kInstantEnergyGain, EffectExpression::FromValue(100_fp));

    InitAttackAbilityData(effect_data, attributes);
    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentEnergy, 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        blue_entity->GetID());

    // 187 energy should be gained, 100 from effect, 50 from energy_gain_bonus and 37 from 50%
    // energy_gain_amplification
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), "187.5"_fp);
}

TEST_F(AbilitySystemTestEffectPackage, BonusAndAmpHeal)
{
    EffectPackageAttributes attributes;
    attributes.heal_bonus = EffectExpression::FromValue(50_fp);
    attributes.heal_amplification = EffectExpression::FromValue(25_fp);

    const EffectData effect = EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(50_fp));

    InitAttackAbilityData(effect, attributes);
    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 500_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 100_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        blue_entity->GetID());

    // 125 health should be agined, 50 from effect, 50 from heal_bonus and 25 from 25%
    // heal_amplification
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 225_fp);
}

TEST_F(AbilitySystemTestEffectPackage, EnergyBurnBonus)
{
    EffectPackageAttributes attributes;
    attributes.energy_burn_bonus = EffectExpression::FromValue(50_fp);

    const EffectData effect = EffectData::Create(EffectType::kInstantEnergyBurn, EffectExpression::FromValue(50_fp));

    InitAttackAbilityData(effect, attributes);
    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentEnergy, 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 100 energy should be burned, 50 for effect_value and 50 for energy_burn_bonus
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 400_fp);
}

TEST_F(AbilitySystemTestEffectPackage, EnergyBurnAmplified)
{
    EffectPackageAttributes attributes;
    attributes.energy_burn_amplification = EffectExpression::FromValue(75_fp);
    const EffectData effect = EffectData::Create(EffectType::kInstantEnergyBurn, EffectExpression::FromValue(50_fp));

    InitAttackAbilityData(effect, attributes);
    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentEnergy, 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 175 energy should be burned, 50 for effect_value and 37 for 75% energy_burn_amplification
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), "412.5"_fp);
}

TEST_F(AbilitySystemTestEffectPackage, BonusAndAmpEnergyGainOverTime)
{
    EffectPackageAttributes attributes;
    attributes.energy_gain_bonus = EffectExpression::FromValue(50_fp);
    attributes.energy_gain_amplification = EffectExpression::FromValue(25_fp);

    const EffectData effect = EffectData::CreateEnergyGainOverTime(
        EffectExpression::FromValue(100_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    InitAttackAbilityData(effect, attributes);

    SpawnCombatUnits();
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentEnergy, 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);
    ASSERT_EQ(blue_stats_component.GetEnergyCost(), 500_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        blue_entity->GetID());

    // Each iteration should gain 18 energy, final_value = 100 (effect) + 50 bonus + 25% (27) =
    // 187 7 extra should be burned on final tick
    for (FixedPoint i = 1_fp; i < 11_fp; i += 1_fp)
    {
        if (i == 10_fp)
        {
            world->TimeStep();
            ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), "187.5"_fp);
        }
        else
        {
            world->TimeStep();
            ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), i * "18.75"_fp);
        }
    }
}

TEST_F(AbilitySystemTestEffectPackage, BonusAndAmpEnergyBurnOverTime)
{
    EffectPackageAttributes attributes;
    attributes.energy_burn_bonus = EffectExpression::FromValue(50_fp);
    attributes.energy_burn_amplification = EffectExpression::FromValue(25_fp);

    const EffectData effect = EffectData::CreateEnergyBurnOverTime(
        EffectExpression::FromValue(100_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    InitAttackAbilityData(effect, attributes);

    SpawnCombatUnits();
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentEnergy, 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 500_fp);
    ASSERT_EQ(red_stats_component.GetEnergyCost(), 500_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // Each iteration should burn 18.75 energy, value = 100 (effect) + 50 bonus + 25% (27) = 187.5
    for (int i = 1; i < 11; i++)
    {
        world->TimeStep();
        ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 500_fp - FixedPoint::FromInt(i) * "18.75"_fp);
    }
}

TEST_F(AbilitySystemTestEffectPackage, BonusAndAmpHealOverTime)
{
    EffectPackageAttributes attributes;
    attributes.heal_bonus = EffectExpression::FromValue(50_fp);
    attributes.heal_amplification = EffectExpression::FromValue(25_fp);

    const EffectData effect = EffectData::CreateHealOverTime(
        EffectHealType::kNormal,
        EffectExpression::FromValue(100_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    InitAttackAbilityData(effect, attributes);

    SpawnCombatUnits();
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 500_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 100_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        blue_entity->GetID());

    // Each iteration should heal 18.75 health, value = 100 (effect) + 50 bonus + 25% (27) = 187.5
    for (int i = 1; i < 11; i++)
    {
        world->TimeStep();
        ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp + FixedPoint::FromInt(i) * "18.75"_fp) << i;
    }
}

TEST_F(AbilitySystemTestEffectPackage, BonusAndAmpDamageOverTime)
{
    EffectPackageAttributes attributes;
    attributes.physical_damage_bonus = EffectExpression::FromValue(50_fp);
    attributes.damage_bonus = EffectExpression::FromValue(50_fp);
    attributes.damage_amplification = EffectExpression::FromValue(25_fp);

    const EffectData effect = EffectData::CreateDamageOverTime(
        EffectDamageType::kPhysical,
        EffectExpression::FromValue(100_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    InitAttackAbilityData(effect, attributes);

    SpawnCombatUnits();
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // Each iteration should deal 25 damage, value = 100 (effect) + 50 damage_bonus + 50
    // physical_damage_bonus + 25% (50) = 250
    for (int i = 1; i < 11; i++)
    {
        world->TimeStep();
        ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp - FixedPoint::FromInt(i * 25)) << i;
    }
}
TEST_F(AbilitySystemTestEffectPackage, ShieldBonus)
{
    EffectPackageAttributes attributes;
    attributes.shield_bonus = EffectExpression::FromValue(50_fp);

    const EffectData effect = EffectData::Create(EffectType::kSpawnShield, EffectExpression::FromValue(125_fp));

    InitAttackAbilityData(effect, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        blue_entity->GetID());

    // 175 shield should be gained, 125 from effect 50 from shield_bonus
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 175_fp);
}

TEST_F(AbilitySystemTestEffectPackage, ShieldAmplification)
{
    EffectPackageAttributes attributes;
    attributes.shield_amplification = EffectExpression::FromValue(50_fp);

    const EffectData effect = EffectData::Create(EffectType::kSpawnShield, EffectExpression::FromValue(125_fp));

    InitAttackAbilityData(effect, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        blue_entity->GetID());

    // 187 shield should be gained, 125 from effect and 62 from 50% shield amplification
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), "187.5"_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, PhysicalPiercing)
{
    EffectPackageAttributes attributes;
    attributes.piercing_percentage = EffectExpression::FromValue(50_fp);
    attributes.physical_piercing_percentage = EffectExpression::FromValue(25_fp);

    // 50 damage ability
    InitAttackAbilityData(EffectDamageType::kPhysical, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalResist, 100_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 40 damage should be done - 50 from effect but physical_resist 100 and piercing_percentage 50 +
    // piercing_percentage 25
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 460_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, EnergyPiercing)
{
    EffectPackageAttributes attributes;
    attributes.piercing_percentage = EffectExpression::FromValue(50_fp);
    attributes.energy_piercing_percentage = EffectExpression::FromValue(25_fp);

    // 50 damage ability
    InitAttackAbilityData(EffectDamageType::kEnergy, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyResist, 100_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // 40 damage should be done - 50 from effect but physical_resist 100 and piercing_percentage 50 +
    // piercing_percentage 25
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 460_fp);
}

TEST_F(AbilitySystemTestBonusAndAmpDamage, CritReductionPiercing)
{
    EffectPackageAttributes attributes;
    attributes.crit_reduction_piercing_percentage = EffectExpression::FromValue(25_fp);

    // Make sure we always crit
    attributes.always_crit = true;

    // 50 damage ability
    InitAttackAbilityData(EffectDamageType::kEnergy, attributes);

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Set some crit reduction
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCritReductionPercentage, 50_fp);

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        attack_ability->skills.at(0).CheckAllIfIsCritical(),
        red_entity->GetID());

    // 50 normal damage
    // 75 critical damage
    // 75 - 50 = 25 damage from critical = 25 * (1 - crit reduction - crit reduction piercing) =
    // = 25_fp * (1_fp - (0_fp.5 - 0.25)) = 25 * 0.75 = 18
    // Total Damage Done: 68 instead of 75
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "431.25"_fp);
}

}  // namespace simulation
