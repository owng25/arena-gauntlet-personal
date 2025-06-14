#include <memory>

#include "base_test_fixtures.h"
#include "components/abilities_component.h"
#include "components/attached_effects_component.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "data/effect_expression.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"
#include "gtest/gtest.h"
#include "systems/effect_system.h"
#include "utility/entity_helper.h"
#include "utility/grid_helper.h"

namespace simulation
{
class EffectSystemTest : public BaseTest
{
    typedef BaseTest Super;

public:
    static constexpr FixedPoint stats_scale = 1000_fp;

protected:
    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 7_fp);
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 5_fp);
        data.type_data.stats.Set(StatType::kAttackEnergyDamage, 10_fp);
        data.type_data.stats.Set(StatType::kAttackPureDamage, 0_fp);

        data.type_data.stats.Set(StatType::kWillpowerPercentage, 11_fp);
        data.type_data.stats.Set(StatType::kTenacityPercentage, 12_fp);
        // 15^2 == 225
        // square distance between blue and red is 200
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);

        data.type_data.stats.Set(StatType::kGrit, 10_fp);
        data.type_data.stats.Set(StatType::kResolve, 20_fp);

        // Resists
        data.type_data.stats.Set(StatType::kPhysicalResist, 10_fp);
        data.type_data.stats.Set(StatType::kEnergyResist, 5_fp);

        data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        // Stop movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {5, 5}, data, blue_entity2);
        SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

        // Add shields
        ShieldData blue_shield_data, blue_shield_data2, red_shield_data;
        blue_shield_data.sender_id = blue_entity->GetID();
        blue_shield_data.receiver_id = blue_entity->GetID();
        blue_shield_data2.sender_id = blue_entity2->GetID();
        blue_shield_data2.receiver_id = blue_entity2->GetID();

        red_shield_data.sender_id = red_entity->GetID();
        red_shield_data.receiver_id = red_entity->GetID();

        blue_shield_data.value = 50_fp;
        blue_shield_data2.value = 50_fp;
        red_shield_data.value = 50_fp;
        EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data);
        EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data2);
        EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
        SpawnCombatUnits();
    }

    Entity* blue_entity = nullptr;
    Entity* blue_entity2 = nullptr;
    Entity* red_entity = nullptr;
    CombatUnitData data;
};

// Param test data for EffectSystemDamageOptionalTest
struct EffectSystemDamageOptionalTest_StructData
{
    EffectDamageType sender_damage_type = EffectDamageType::kNone;
    FixedPoint sender_damage = 0_fp;
    FixedPoint vampiric_percentage = 0_fp;
    FixedPoint to_shield_percentage = 0_fp;
    bool excess_vamp_to_shield = false;
    FixedPoint expected_post_mitigation_damage = 0_fp;
    FixedPoint expected_total_vamp = 0_fp;
    FixedPoint expected_health_gain = 0_fp;
    FixedPoint expected_excess_vamp_to_shield = 0_fp;
    FixedPoint expected_to_shield_gain = 0_fp;
    FixedPoint shield_value = 0_fp;

    friend std::ostream& operator<<(std::ostream& os, const EffectSystemDamageOptionalTest_StructData& obj)
    {
        return os << obj.sender_damage_type << obj.sender_damage << obj.vampiric_percentage << obj.to_shield_percentage
                  << obj.excess_vamp_to_shield << obj.expected_post_mitigation_damage << obj.expected_total_vamp
                  << obj.expected_health_gain << obj.expected_excess_vamp_to_shield << obj.expected_to_shield_gain;
    }
};

class EffectSystemDamageOptionalTest : public BaseTest
{
    typedef BaseTest Super;

public:
    static constexpr FixedPoint stats_scale = 1000_fp;

protected:
    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);

        // 15^2 == 225
        // square distance between blue and red is 200
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);

        data.type_data.stats.Set(StatType::kGrit, 20_fp);
        data.type_data.stats.Set(StatType::kResolve, 10_fp);

        // Resists
        data.type_data.stats.Set(StatType::kPhysicalResist, 50_fp);
        data.type_data.stats.Set(StatType::kEnergyResist, 30_fp);

        data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 500_fp);

        // Stop movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
        SpawnCombatUnits();
    }

    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    CombatUnitData data;
    EffectSystemDamageOptionalTest_StructData test_data{};
};

struct EffectSystemDamageVampiricOptionalTest_Param
    : EffectSystemDamageOptionalTest,
      ::testing::WithParamInterface<EffectSystemDamageOptionalTest_StructData>
{
    EffectSystemDamageVampiricOptionalTest_Param()
    {
        test_data = GetParam();
    }

    FRIEND_TEST(EffectDamageOptionalTests, TestCase1);
};

TEST_P(EffectSystemDamageVampiricOptionalTest_Param, Test)
{
    std::vector<event_data::ApplyShieldGain> events_shield_gain;
    world->SubscribeToEvent(
        EventType::kApplyShieldGain,
        [&events_shield_gain](const Event& event)
        {
            events_shield_gain.emplace_back(event.Get<event_data::ApplyShieldGain>());
        });

    auto& current_test_data = GetParam();
    const auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Add shields to red
    ShieldData red_shield_data;
    red_shield_data.sender_id = red_entity->GetID();
    red_shield_data.receiver_id = red_entity->GetID();
    red_shield_data.value = current_test_data.shield_value;
    EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);

    const FixedPoint shield_before_damage = world->GetShieldTotal(*red_entity);
    ASSERT_EQ(shield_before_damage, current_test_data.shield_value);
    const FixedPoint health_before_damage = red_stats_component.GetCurrentHealth();

    // Construct effect data
    auto effect = EffectData::CreateDamage(
        current_test_data.sender_damage_type,
        EffectExpression::FromValue(current_test_data.sender_damage));
    effect.attached_effect_package_attributes.vampiric_percentage =
        EffectExpression::FromValue(current_test_data.vampiric_percentage);
    effect.attached_effect_package_attributes.excess_vamp_to_shield = current_test_data.excess_vamp_to_shield;
    effect.attributes.to_shield_percentage = current_test_data.to_shield_percentage;

    EffectState effect_state;

    // Apply effect on red
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Track the health before any vampiric is applied
    const FixedPoint health_before_vamp = red_stats_component.GetCurrentHealth();

    // Track the shield before any excess_to_shield or to_shield is applied
    // const FixedPoint shield_before_vamp = world->GetShieldTotal(*red_entity);

    // Apply effect on blue, so that we can track vampiric from the sender better
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // Track health after damage and vampiric has occurred
    // const FixedPoint health_after_damage = red_stats_component.GetCurrentHealth();

    // Track shield after all vampiric and to_shield has occurred
    const FixedPoint shield_after_vamp = world->GetShieldTotal(*red_entity);

    // Track how much post_mitigation_damage actually occurred
    const FixedPoint actual_post_mitigation_damage = health_before_damage - health_before_vamp + shield_before_damage;

    // Track how much vampiric actually occurred
    FixedPoint actual_total_vampiric = 0_fp;

    // Track how much health was actually gained
    FixedPoint actual_health_gain = 0_fp;

    // Track how much shield was actually gained
    FixedPoint actual_shield_gain = 0_fp;

    // Following checks will confirm if the proper amount of shield was gained if the test requires
    // it
    if (current_test_data.excess_vamp_to_shield == true)
    {
        // Following will check tests that have vampiric_percentage active
        if (current_test_data.vampiric_percentage != 0_fp)
        {
            // If vampiric_percentage and to_shield_percentage are active
            if (current_test_data.to_shield_percentage > 0_fp)
            {
                ASSERT_EQ(events_shield_gain.size(), 4);
                EXPECT_EQ(events_shield_gain.at(0).gain_amount.Floor(), current_test_data.expected_total_vamp);
                EXPECT_EQ(events_shield_gain.at(1).gain_amount.Floor(), current_test_data.expected_to_shield_gain);
                EXPECT_EQ(
                    events_shield_gain.at(2).gain_amount.Floor(),
                    current_test_data.expected_excess_vamp_to_shield);

                actual_shield_gain = events_shield_gain.at(1).gain_amount;
            }
            // If only vampiric_percentage is active
            else
            {
                ASSERT_EQ(events_shield_gain.size(), 2);
                EXPECT_EQ(events_shield_gain.at(0).gain_amount.Floor(), current_test_data.expected_total_vamp);
                EXPECT_EQ(
                    events_shield_gain.at(1).gain_amount.Floor(),
                    current_test_data.expected_excess_vamp_to_shield);
            }
        }
        // If there is no vampiric_percentage but there is to_shield_percentage
        else if (current_test_data.vampiric_percentage == 0_fp && current_test_data.to_shield_percentage > 0_fp)
        {
            EXPECT_EQ(events_shield_gain.size(), 2);
            actual_shield_gain = events_shield_gain.at(0).gain_amount;
        }
    }

    // The following is for tests that have to_shield_percentage but no excess_vamp_to_shield
    else if (current_test_data.to_shield_percentage > 0_fp && current_test_data.excess_vamp_to_shield == false)
    {
        // 2 Events since blue and red both have shield_gain event
        EXPECT_EQ(events_shield_gain.size(), 2);
        ASSERT_EQ(events_shield_gain.at(0).gain_amount.Floor(), current_test_data.expected_to_shield_gain);
        actual_shield_gain = events_shield_gain.at(0).gain_amount;
    }

    // Calculations only to occur if vampiric is present
    if (current_test_data.vampiric_percentage > 0_fp)
    {
        // Calculate health gain
        actual_health_gain = health_before_damage - health_before_vamp;

        // Calculate vampiric amount
        actual_total_vampiric = (shield_after_vamp - actual_shield_gain) + actual_health_gain;
    }

    // Damage type being sent
    EXPECT_EQ(current_test_data.sender_damage_type, test_data.sender_damage_type);

    // Amount of damage being sent
    EXPECT_EQ(current_test_data.sender_damage, test_data.sender_damage);

    auto evaluate = [&](const EffectExpression& expression) -> FixedPoint
    {
        return expression.Evaluate(ExpressionEvaluationContext(world.get(), blue_entity->GetID(), red_entity->GetID()));
    };

    // Confirm values that were sent were applied correctly
    EXPECT_EQ(
        current_test_data.vampiric_percentage,
        evaluate(effect.attached_effect_package_attributes.vampiric_percentage).Floor());
    EXPECT_EQ(current_test_data.expected_post_mitigation_damage, actual_post_mitigation_damage.Floor());
    EXPECT_EQ(current_test_data.expected_total_vamp, actual_total_vampiric.Floor());
    EXPECT_EQ(current_test_data.expected_health_gain, actual_health_gain.Floor());
    EXPECT_EQ(current_test_data.expected_to_shield_gain, actual_shield_gain.Floor());
}

INSTANTIATE_TEST_SUITE_P(
    EffectSystemTest,
    EffectSystemDamageVampiricOptionalTest_Param,
    // clang-format off
    // Data from: https://docs.google.com/spreadsheets/d/1jsLooIzD3cZv_g2ARqt2Tknmlbv4xHPBRAK33Vwb5I0/edit#gid=0
    testing::Values(
        // sender_damage_type, sender_damage, vampiric_percentage, to_shield_percentage, excess_vamp_to_shield, expected_post_mitigation_damage, expected_total_vamp, expected_health_gain, expected_excess_vamp_to_shield, expected_to_shield_gain
        EffectSystemDamageOptionalTest_StructData{ // 0
            EffectDamageType::kPhysical, 220_fp, 10_fp, 20_fp, true, 133_fp, 13_fp, 5_fp, 8_fp, 26_fp, 128_fp},
        EffectSystemDamageOptionalTest_StructData{ // 1
            EffectDamageType::kPhysical, 220_fp, 0_fp, 20_fp, true, 133_fp, 0_fp, 0_fp, 20_fp, 26_fp, 128_fp},
        EffectSystemDamageOptionalTest_StructData{ // 2
            EffectDamageType::kPhysical, 135_fp, 10_fp, 20_fp, true, 76_fp, 7_fp, 5_fp, 2_fp, 15_fp, 71_fp},
        EffectSystemDamageOptionalTest_StructData{ // 3
            EffectDamageType::kPhysical, 135_fp, 0_fp, 20_fp, true, 76_fp, 0_fp, 0_fp, 20_fp, 15_fp, 71_fp},
        EffectSystemDamageOptionalTest_StructData{ // 4
            EffectDamageType::kEnergy, 220_fp, 12_fp, 20_fp, true, 161_fp, 19_fp, 5_fp, 13_fp, 32_fp, 156_fp},
        EffectSystemDamageOptionalTest_StructData{ // 5
            EffectDamageType::kEnergy, 220_fp, 0_fp, 20_fp, true, 161_fp, 0_fp, 0_fp, 20_fp, 32_fp, 156_fp},
        EffectSystemDamageOptionalTest_StructData{ // 6
            EffectDamageType::kEnergy, 135_fp, 10_fp, 0_fp, true, 96_fp, 9_fp, 5_fp, 4_fp, 0_fp, 91_fp},
        EffectSystemDamageOptionalTest_StructData{ // 7
            EffectDamageType::kEnergy, 135_fp, 0_fp, 0_fp, true, 96_fp, 0_fp, 0_fp, 20_fp, 0_fp, 91_fp},
        EffectSystemDamageOptionalTest_StructData{ // 8
            EffectDamageType::kPure, 220_fp, 10_fp, 0_fp, true, 220_fp, 22_fp, 5_fp, 17_fp, 0_fp, 215_fp},
        EffectSystemDamageOptionalTest_StructData{ // 9
            EffectDamageType::kPure, 220_fp, 0_fp, 0_fp, true, 220_fp, 0_fp, 0_fp, 20_fp, 0_fp, 215_fp},
        EffectSystemDamageOptionalTest_StructData{ // 10
            EffectDamageType::kPure, 135_fp, 10_fp, 0_fp, true, 135_fp, 13_fp, 5_fp, 8_fp, 0_fp, 130_fp},
        EffectSystemDamageOptionalTest_StructData{ // 11
            EffectDamageType::kPure, 135_fp, 0_fp, 0_fp, true, 135_fp, 0_fp, 0_fp, 20_fp, 0_fp, 130_fp},
        EffectSystemDamageOptionalTest_StructData{ // 12
            EffectDamageType::kPhysical, 220_fp, 10_fp, 20_fp, false, 133_fp, 5_fp, 5_fp, 20_fp, 26_fp, 128_fp},
        EffectSystemDamageOptionalTest_StructData{ // 13
            EffectDamageType::kPhysical, 220_fp, 0_fp, 20_fp, false, 133_fp, 0_fp, 0_fp, 20_fp, 26_fp, 128_fp},
        EffectSystemDamageOptionalTest_StructData{ // 14
            EffectDamageType::kPhysical, 135_fp, 10_fp, 20_fp, false, 76_fp, 5_fp, 5_fp, 20_fp, 15_fp, 71_fp},
        EffectSystemDamageOptionalTest_StructData{ // 15
            EffectDamageType::kPhysical, 135_fp, 0_fp, 20_fp, false, 76_fp, 0_fp, 0_fp, 20_fp, 15_fp, 71_fp},
        EffectSystemDamageOptionalTest_StructData{ // 16
            EffectDamageType::kEnergy, 220_fp, 10_fp, 20_fp, false, 161_fp, 5_fp, 5_fp, 20_fp, 32_fp, 156_fp},
        EffectSystemDamageOptionalTest_StructData{ // 17
            EffectDamageType::kEnergy, 220_fp, 0_fp, 20_fp, false, 161_fp, 0_fp, 0_fp, 20_fp, 32_fp, 156_fp},
        EffectSystemDamageOptionalTest_StructData{ // 18
            EffectDamageType::kEnergy, 135_fp, 10_fp, 0_fp, false, 96_fp, 5_fp, 5_fp, 20_fp, 0_fp, 91_fp},
        EffectSystemDamageOptionalTest_StructData{ // 19
            EffectDamageType::kEnergy, 135_fp, 0_fp, 0_fp, false, 96_fp, 0_fp, 0_fp, 20_fp, 0_fp, 91_fp},
        EffectSystemDamageOptionalTest_StructData{ // 20
            EffectDamageType::kPure, 220_fp, 10_fp, 0_fp, false, 220_fp, 5_fp, 5_fp, 20_fp, 0_fp, 215_fp},
        EffectSystemDamageOptionalTest_StructData{ // 21
            EffectDamageType::kPure, 220_fp, 0_fp, 0_fp, false, 220_fp, 0_fp, 0_fp, 20_fp, 0_fp, 215_fp},
        EffectSystemDamageOptionalTest_StructData{ // 22
            EffectDamageType::kPure, 135_fp, 10_fp, 0_fp, false, 135_fp, 5_fp, 5_fp, 20_fp, 0_fp, 130_fp},
        EffectSystemDamageOptionalTest_StructData{ // 23
            EffectDamageType::kPure, 135_fp, 0_fp, 0_fp, false, 135_fp, 0_fp, 0_fp, 20_fp, 0_fp, 130_fp},
        EffectSystemDamageOptionalTest_StructData{ // 24
            EffectDamageType::kPhysical, 220_fp, 10_fp, 20_fp, true, 133_fp, 13_fp, 5_fp, 8_fp, 26_fp, 128_fp},
        EffectSystemDamageOptionalTest_StructData{ // 25
            EffectDamageType::kPhysical, 220_fp, 0_fp, 20_fp, true, 133_fp, 0_fp, 0_fp, 20_fp, 26_fp, 128_fp},
        EffectSystemDamageOptionalTest_StructData{ // 26
            EffectDamageType::kPhysical, 135_fp, 10_fp, 20_fp, true, 76_fp, 7_fp, 5_fp, 2_fp, 15_fp, 71_fp},
        EffectSystemDamageOptionalTest_StructData{ // 27
            EffectDamageType::kPhysical, 135_fp, 0_fp, 20_fp, true, 76_fp, 0_fp, 0_fp, 20_fp, 15_fp, 71_fp},
        EffectSystemDamageOptionalTest_StructData{ // 28
            EffectDamageType::kEnergy, 220_fp, 10_fp, 20_fp, true, 161_fp, 16_fp, 5_fp, 10_fp, 32_fp, 156_fp},
        EffectSystemDamageOptionalTest_StructData{ // 29
            EffectDamageType::kEnergy, 220_fp, 0_fp, 20_fp, true, 161_fp, 0_fp, 0_fp, 20_fp, 32_fp, 156_fp},
        EffectSystemDamageOptionalTest_StructData{ // 30
            EffectDamageType::kEnergy, 135_fp, 10_fp, 0_fp, true, 96_fp, 9_fp, 5_fp, 4_fp, 0_fp, 91_fp},
        EffectSystemDamageOptionalTest_StructData{ // 31
            EffectDamageType::kEnergy, 135_fp, 0_fp, 0_fp, true, 96_fp, 0_fp, 0_fp, 20_fp, 0_fp, 91_fp},
        EffectSystemDamageOptionalTest_StructData{ // 32
            EffectDamageType::kPure, 220_fp, 10_fp, 0_fp, true, 220_fp, 22_fp, 5_fp, 17_fp, 0_fp, 215_fp},
        EffectSystemDamageOptionalTest_StructData{ // 33
            EffectDamageType::kPure, 220_fp, 0_fp, 0_fp, true, 220_fp, 0_fp, 0_fp, 20_fp, 0_fp, 215_fp},
        EffectSystemDamageOptionalTest_StructData{ // 34
            EffectDamageType::kPure, 135_fp, 10_fp, 0_fp, true, 135_fp, 13_fp, 5_fp, 8_fp, 0_fp, 130_fp},
        EffectSystemDamageOptionalTest_StructData{ // 35
            EffectDamageType::kPure, 135_fp, 0_fp, 0_fp, true, 135_fp, 0_fp, 0_fp, 20_fp, 0_fp, 130_fp},
        EffectSystemDamageOptionalTest_StructData{ // 36
            EffectDamageType::kPhysical, 220_fp, 10_fp, 20_fp, false, 133_fp, 5_fp, 5_fp, 20_fp, 26_fp, 128_fp},
        EffectSystemDamageOptionalTest_StructData{ // 37
            EffectDamageType::kPhysical, 220_fp, 0_fp, 20_fp, false, 133_fp, 0_fp, 0_fp, 20_fp, 26_fp, 128_fp},
        EffectSystemDamageOptionalTest_StructData{ // 38
            EffectDamageType::kPhysical, 135_fp, 10_fp, 20_fp, false, 76_fp, 5_fp, 5_fp, 20_fp, 15_fp, 71_fp},
        EffectSystemDamageOptionalTest_StructData{ // 39
            EffectDamageType::kPhysical, 135_fp, 0_fp, 20_fp, false, 76_fp, 0_fp, 0_fp, 20_fp, 15_fp, 71_fp},
        EffectSystemDamageOptionalTest_StructData{ // 40
            EffectDamageType::kEnergy, 220_fp, 10_fp, 20_fp, false, 161_fp, 5_fp, 5_fp, 20_fp, 32_fp, 156_fp},
        EffectSystemDamageOptionalTest_StructData{ // 41
            EffectDamageType::kEnergy, 220_fp, 0_fp, 20_fp, false, 161_fp, 0_fp, 0_fp, 20_fp, 32_fp, 156_fp},
        EffectSystemDamageOptionalTest_StructData{ // 42
            EffectDamageType::kEnergy, 135_fp, 10_fp, 0_fp, false, 96_fp, 5_fp, 5_fp, 20_fp, 0_fp, 91_fp},
        EffectSystemDamageOptionalTest_StructData{ // 43
            EffectDamageType::kEnergy, 135_fp, 0_fp, 0_fp, false, 96_fp, 0_fp, 0_fp, 20_fp, 0_fp, 91_fp},
        EffectSystemDamageOptionalTest_StructData{ // 44
            EffectDamageType::kPure, 220_fp, 10_fp, 0_fp, false, 220_fp, 5_fp, 5_fp, 20_fp, 0_fp, 215_fp},
        EffectSystemDamageOptionalTest_StructData{ // 45
            EffectDamageType::kPure, 220_fp, 0_fp, 0_fp, false, 220_fp, 0_fp, 0_fp, 20_fp, 0_fp, 215_fp},
        EffectSystemDamageOptionalTest_StructData{ // 46
            EffectDamageType::kPure, 135_fp, 10_fp, 0_fp, false, 135_fp, 5_fp, 5_fp, 20_fp, 0_fp, 130_fp},
        EffectSystemDamageOptionalTest_StructData{ // 47
            EffectDamageType::kPure, 135_fp, 0_fp, 0_fp, false, 135_fp, 0_fp, 0_fp, 20_fp, 0_fp, 130_fp}
    ));
// clang-format on

TEST_F(EffectSystemTest, ApplyEffectPhysicalDamageFromAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 (effect) - 10 (grit) = 90 (10 armour, which makes the attack 90% effective) = 81

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 36 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "18.182"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, BaseStatOverride)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    auto get_max_hp = [this](Entity* entity)
    {
        return world->GetBaseStats(*entity).Get(StatType::kMaxHealth);
    };

    // This check is necessary because stat value will be cached at this point
    // We have to test that cache is cleared properly after effect application
    EXPECT_EQ(get_max_hp(blue_entity), 1000_fp);
    EXPECT_EQ(get_max_hp(red_entity), 1000_fp);

    // Create effect
    constexpr FixedPoint new_stat_value = 150_fp;
    auto effect = EffectData::CreateStatOverride(StatType::kMaxHealth, new_stat_value);

    // Apply Effect
    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    EXPECT_EQ(get_max_hp(blue_entity), 1000_fp);
    EXPECT_EQ(get_max_hp(red_entity), new_stat_value);
}

TEST_F(EffectSystemTest, ApplyEffectPhysicalDamageFromOmega)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kOmega;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 - 10 (grit) = 90 (10 armour, which makes the attack 90% effective) = 81

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 31 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "18.182"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectPhysicalDamageCriticalFromAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(50_fp));

    // Apply Effect with critical

    EffectState effect_state;
    effect_state.is_critical = true;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.sender_stats.live.Set(StatType::kCritAmplificationPercentage, kDefaultCritAmplificationPercentage);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 150% * (50 (effect)) = 75 - 10 (grit) = 65 (10 armour, which makes the attack 90%
    // effective) = 59

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 15 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "40.91"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectPhysicalDamagePercentageExpression)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Set omega power
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kOmegaPowerPercentage, 300_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kOmegaPowerPercentage, 200_fp);
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage), 300_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage), 200_fp);

    // Check assumptions
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    // 15% MaxHP (Receiver) * OmegaPower (Receiver) + 200 * OmegaPower (Sender)
    // = 150 * 2 + 200 * 3 = 900

    // 15% MaxHP (Receiver) * OmegaPower (Receiver)
    EffectExpression left_side;
    left_side.operation_type = EffectOperationType::kMultiply;
    left_side.operands = {
        EffectExpression::FromStatPercentage(
            15_fp,
            StatType::kMaxHealth,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver),
        EffectExpression::FromStat(
            StatType::kOmegaPowerPercentage,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver)};

    // 200 * OmegaPower (Sender)
    EffectExpression right_side;
    right_side.operation_type = EffectOperationType::kMultiply;
    right_side.operands = {
        EffectExpression::FromValue(200_fp),
        EffectExpression::FromStat(
            StatType::kOmegaPowerPercentage,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kSender)};

    // Add both
    EffectExpression expression;
    expression.operation_type = EffectOperationType::kAdd;
    expression.operands = {left_side, right_side};

    // Update current stats
    const FullStatsData blue_stats = world->GetFullStats(blue_entity->GetID());
    const ExpressionEvaluationContext expression_context(world.get(), blue_entity->GetID(), red_entity->GetID());

    world->LogDebug(-1, "{}", expression);
    ASSERT_EQ(expression.Evaluate(expression_context), 900_fp);
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, expression);

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = blue_stats;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kOmega;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 900 - 10 (grit) = 890 (10 armour, which makes the attack 90% effective) =
    // = 890*100/(100 + 10 armor) = 809

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // 809 - 50 (shield) = 759
    // 1000 - 759 = 241
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "240.91"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);
}

TEST_F(EffectSystemTest, ApplyEffectEnergySomeVulnerability)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kVulnerabilityPercentage, 50_fp);

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(90_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // (90 (effect) * 0.5) - 20 (resolve) = 25 (5 energy resist, which makes the attack
    // 95% effective) = 23

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), "26.191"_fp);

    // Health should be reduced by only 26 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectPhysicalSomeVulnerability)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kVulnerabilityPercentage, 50_fp);
    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(40_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // (40 (effect) * 0.5) - 10 (grit) = 10 (10 armour, which makes the attack 90%
    // effective) = 9 Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), "40.91"_fp);

    // Health should be reduced by only 36 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectOptionalExploitWeakness)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // Assume health/shield is at maximum
    // Reduce health from starting value of MaxHealth
    blue_template_stats.Set(StatType::kMaxHealth, 100_fp).Set(StatType::kCurrentHealth, 80_fp);
    red_template_stats.Set(StatType::kMaxHealth, 100_fp).Set(StatType::kCurrentHealth, 80_fp);
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 100_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 100_fp);

    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 80_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 80_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));
    effect.attached_effect_package_attributes.exploit_weakness = true;

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // (100 (effect) * 1.2 (exploit_weakness true, missing health-20%)) - 20 (resolve) = 100
    // (5 energy resist, which makes the attack 95% effective) = 95

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be 35( 80 - 45)
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "34.762"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 80_fp);
}

TEST_F(EffectSystemTest, ApplyEffectSomeOptionalPiercing)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));
    effect.attached_effect_package_attributes.piercing_percentage = EffectExpression::FromValue(50_fp);

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 (effect) - 20 (resolve) = 80 (5 energy resist, but 50% pierce, = 2.5 resist) =
    // 78

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 26 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "21.952"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectSomeEnergyPiercing)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyPiercingPercentage, 50_fp);

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 (effect) - 20 (resolve) = 80 (5 energy resist, but 50% pierce, = 2.5 resist) =
    // 78

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 26 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "21.952"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectMaxEnergyPiercing)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyPiercingPercentage, kMaxPercentageFP);

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 (effect) - 20 (resolve) = 80 (5 energy resist, but 100% pierce)

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 26 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 20_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectSomePhysicalPiercing)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalPiercingPercentage, 50_fp);

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 phys resist, but 50% pierce so phys resist = 5)
    // = 90

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 40 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "9.524"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectMaxPhysicalPiercing)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalPiercingPercentage, kMaxPercentageFP);

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which is negated by 100% pierce) = 95

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by 45 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 5_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectEnergyDamageFromAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // (100 (effect) - 20 (resolve) = 80 (5 energy resist, which makes the attack 95%
    // effective) = 76

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 26 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "23.81"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectEnergyDamageFromOmega)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(120_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kOmega;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 120 - 20 (resolve) = 100 (5 energy resist, which makes the attack 95% effective) = 95

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 45 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "4.762"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectEnergyDamageCriticalFromAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(80_fp));

    // Apply Effect with critical

    EffectState effect_state;
    effect_state.is_critical = true;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.sender_stats.live.Set(StatType::kCritAmplificationPercentage, kDefaultCritAmplificationPercentage);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    //  150% * (70 (effect)) = 120 - 20 (resolve) = 100 (5 energy resist, which makes
    //  the
    // attack 95% effective) = 95

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 45 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "4.762"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectPureDamage)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(30_fp));

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Blue should not be affected
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);

    // Pure damage should not be affected by armor or energy resist
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 20_fp)
        << "Pure damage should have reduced the shield by the amount of damage given";
}

TEST_F(EffectSystemTest, ApplyDebuffMinWillpower)
{
    auto& attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 0);

    // Create debuff
    auto effect = EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Update current stats
    auto& attached_effect = attached_effects_component.GetAttachedEffects().at(0);
    // The attached effect should be present
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 1);
    ASSERT_EQ(EvaluateNoStats(attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(attached_effect->effect_data.lifetime.duration_time_ms, 100);

    // Current armour reduced by 10
    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
}

TEST_F(EffectSystemTest, ApplyDebuffSomeWillpower)
{
    auto& attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // EffectExpression kStatPercentage TODO and change this to kMaxPercentageFP
    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, 70_fp);

    // 0 Attached effects should be present
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 0);

    // Create debuff
    auto effect = EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Update current stats

    auto& attached_effect = attached_effects_component.GetAttachedEffects().at(0);
    // The attached effect should be present
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 1);
    ASSERT_EQ(EvaluateNoStats(attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(attached_effect->effect_data.lifetime.duration_time_ms, 100);

    // Willpower reduced the debuff effect by 70%
    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 7_fp);
}

TEST_F(EffectSystemTest, ApplyDebuffMaxWillpower)
{
    auto& attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMaxPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 0);

    // Create debuff
    auto effect = EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    auto& attached_effect = attached_effects_component.GetAttachedEffects().at(0);
    // The attached effect should be present
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 1);
    ASSERT_EQ(EvaluateNoStats(attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(attached_effect->effect_data.lifetime.duration_time_ms, 100);

    // Willpower reduced the debuff effect by 100%
    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 10_fp);
}

TEST_F(EffectSystemTest, ApplyWoundRejection)
{
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 0);

    // Create effect that doesn't wound the target - so conditional will remain false
    auto effect = EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    // conditional effect that should not trigger because the target is not wounded
    auto conditional_effect =
        EffectData::CreateCondition(EffectConditionType::kWound, kDefaultAttachedEffectsFrequencyMs);

    // Set the conditional_effect to require the target to be wounded.
    conditional_effect.required_conditions.Add(ConditionType::kWounded);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    EffectState conditional_effect_state;
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Attempt to fire second effect (wound), should be rejected.
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        conditional_effect,
        conditional_effect_state);

    // Update current stats

    //// The first effect should fire like normal so remain size(1). The conditional_effect should
    /// not fire since the conditional bool remains false.
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 1);

    // Below will confirm non-conditional effect executed properly and was not effected by
    // conditional_effect being rejected. This with the attached_effect.size being 1 shows that it
    // was only the debuff that executed.
    auto& attached_effect = red_attached_effects_component.GetAttachedEffects().at(0);
    ASSERT_EQ(attached_effect->effect_data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(EvaluateNoStats(attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(attached_effect->effect_data.lifetime.duration_time_ms, 100);
    ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kPhysicalResist));

    // Should have reduced armour
    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
    ASSERT_FALSE(EntityHelper::IsWounded(*red_entity));
}

TEST_F(EffectSystemTest, ApplyWoundApproved)
{
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 0);

    // Wound the target
    auto effect = EffectData::CreateCondition(EffectConditionType::kWound, kDefaultAttachedEffectsFrequencyMs);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Conditional_effect that should fire if the condition is true
    auto conditional_effect =
        EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 800);

    // Set the condition to require the target to be wounded
    conditional_effect.required_conditions.Add(ConditionType::kWounded);

    EffectState effect_state2;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        conditional_effect,
        effect_state2);

    // First four are from the wound
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 3);

    auto& conditional_attached_effect = red_attached_effects_component.GetAttachedEffects().at(2);

    // Update current stats

    //// The attached effect should applied.
    ASSERT_EQ(conditional_attached_effect->effect_data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(conditional_attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(EvaluateNoStats(conditional_attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(conditional_attached_effect->effect_data.lifetime.duration_time_ms, 800);
    ASSERT_TRUE(EntityHelper::IsWounded(*red_entity));
    ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kPhysicalResist));

    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
}

TEST_F(EffectSystemTest, ApplyBurnRejection)
{
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 0);

    // Create effect that doesn't burn the target - so conditional will remain false
    auto effect = EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    // conditional effect that should not trigger because the target is not burned
    auto conditional_effect =
        EffectData::CreateCondition(EffectConditionType::kBurn, kDefaultAttachedEffectsFrequencyMs);

    // Set the conditional_effect to require the target to be burned.
    conditional_effect.required_conditions.Add(ConditionType::kBurned);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    EffectState conditional_effect_state;
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Attempt to fire second effect (burn), should be rejected.
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        conditional_effect,
        conditional_effect_state);

    // Update current stats

    //// The first effect should fire like normal so remain size(1). The conditional_effect should
    /// not fire since the conditional bool remains false.
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 1);

    // Below will confirm non-conditional effect executed properly and was not effected by
    // conditional_effect being rejected. This with the attached_effect.size being 1 shows that it
    // was only the debuff that executed.
    auto& attached_effect = red_attached_effects_component.GetAttachedEffects().at(0);
    ASSERT_EQ(attached_effect->effect_data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(EvaluateNoStats(attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(attached_effect->effect_data.lifetime.duration_time_ms, 100);
    ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kPhysicalResist));

    // Should have reduced armour
    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));
}

TEST_F(EffectSystemTest, ApplyBurnApproved)
{
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 0);

    // Burn the target
    auto effect = EffectData::CreateCondition(EffectConditionType::kBurn, kDefaultAttachedEffectsFrequencyMs);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Conditional_effect that should fire if the condition is true
    auto conditional_effect =
        EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 800);

    // Set the condition to require the target to be burned
    conditional_effect.required_conditions.Add(ConditionType::kBurned);

    EffectState effect_state2;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        conditional_effect,
        effect_state2);

    // First tree are from the burn
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 3);

    auto& conditional_attached_effect = red_attached_effects_component.GetAttachedEffects().at(2);

    // Update current stats

    //// The attached effect should applied.
    ASSERT_EQ(conditional_attached_effect->effect_data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(conditional_attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(EvaluateNoStats(conditional_attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(conditional_attached_effect->effect_data.lifetime.duration_time_ms, 800);
    ASSERT_TRUE(EntityHelper::IsBurned(*red_entity));
    ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kPhysicalResist));

    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
}

TEST_F(EffectSystemTest, ApplyPoisonRejection)
{
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 0);

    // Create effect that doesn't poison the target - so conditional will remain false
    auto effect = EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    // conditional effect that should not trigger because the target is not poisoned
    auto conditional_effect =
        EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);

    // Set the conditional_effect to require the target to be poisoned.
    conditional_effect.required_conditions.Add(ConditionType::kPoisoned);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    EffectState conditional_effect_state;
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Attempt to fire second effect (poison), should be rejected.
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        conditional_effect,
        conditional_effect_state);

    // Update current stats

    //// The first effect should fire like normal so remain size(1). The conditional_effect should
    /// not fire since the conditional bool remains false.
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 1);

    // Below will confirm non-conditional effect executed properly and was not effected by
    // conditional_effect being rejected. This with the attached_effect.size being 1 shows that it
    // was only the debuff that executed.
    auto& attached_effect = red_attached_effects_component.GetAttachedEffects().at(0);
    ASSERT_EQ(attached_effect->effect_data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(EvaluateNoStats(attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(attached_effect->effect_data.lifetime.duration_time_ms, 100);
    ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kPhysicalResist));

    // Should have reduced armour
    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
    ASSERT_FALSE(EntityHelper::IsPoisoned(*red_entity));
}

TEST_F(EffectSystemTest, ApplyPoisonApproved)
{
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 0);

    // Poison the target
    auto effect = EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Conditional_effect that should fire if the condition is true
    auto conditional_effect =
        EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 800);

    // Set the condition to require the target to be poisoned
    conditional_effect.required_conditions.Add(ConditionType::kPoisoned);

    EffectState effect_state2;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        conditional_effect,
        effect_state2);

    // First tree are from the poison
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 3);

    auto& conditional_attached_effect = red_attached_effects_component.GetAttachedEffects().at(2);

    // Update current stats

    //// The attached effect should applied.
    ASSERT_EQ(conditional_attached_effect->effect_data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(conditional_attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(EvaluateNoStats(conditional_attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(conditional_attached_effect->effect_data.lifetime.duration_time_ms, 800);
    ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity));
    ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kPhysicalResist));

    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
}

TEST_F(EffectSystemTest, ApplyFrostRejection)
{
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 0);

    // Create effect that doesn't frost the target - so conditional will remain false
    auto physical_resist_effect =
        EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    // conditional effect that should not trigger because the target is not frozen
    auto frost_effect = EffectData::CreateCondition(EffectConditionType::kFrost, kDefaultAttachedEffectsFrequencyMs);

    // Set the conditional_effect to require the target to be frosted.
    frost_effect.required_conditions.Add(ConditionType::kFrosted);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    EffectState conditional_effect_state;
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        physical_resist_effect,
        effect_state);

    // Attempt to frost second effect (frost), should be rejected.
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        frost_effect,
        conditional_effect_state);

    // Update current stats

    //// The first effect should fire like normal so remain size(1). The conditional_effect should
    /// not fire since the conditional bool remains false.
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 1);

    // Below will confirm non-conditional effect executed properly and was not effected by
    // conditional_effect being rejected. This with the attached_effect.size being 1 shows that it
    // was only the debuff that executed.
    auto& attached_effect = red_attached_effects_component.GetAttachedEffects().at(0);
    ASSERT_EQ(attached_effect->effect_data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(EvaluateNoStats(attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(attached_effect->effect_data.lifetime.duration_time_ms, 100);
    ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kPhysicalResist));

    // Should have reduced armour
    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
    ASSERT_FALSE(EntityHelper::IsFrosted(*red_entity));
}

TEST_F(EffectSystemTest, ApplyFrostApproved)
{
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, kMinPercentageFP);

    // 0 Attached effects should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 0);

    // Frost the target
    auto effect = EffectData::CreateCondition(EffectConditionType::kFrost, kDefaultAttachedEffectsFrequencyMs);

    // Apply Effect with max effect ratio

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Conditional_effect that should fire if the condition is true
    auto conditional_effect =
        EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 800);

    // Set the condition to require the target to be poisoned
    conditional_effect.required_conditions.Add(ConditionType::kFrosted);

    EffectState effect_state2;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        conditional_effect,
        effect_state2);

    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 3);

    auto& conditional_attached_effect = red_attached_effects_component.GetAttachedEffects().at(2);

    // Update current stats

    //// The attached effect should applied.
    ASSERT_EQ(conditional_attached_effect->effect_data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(conditional_attached_effect->effect_data.type_id.stat_type, StatType::kPhysicalResist);
    ASSERT_EQ(EvaluateNoStats(conditional_attached_effect->effect_data.GetExpression()), 10_fp);
    ASSERT_EQ(conditional_attached_effect->effect_data.lifetime.duration_time_ms, 800);
    ASSERT_TRUE(EntityHelper::IsFrosted(*red_entity));
    ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kPhysicalResist));

    ASSERT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kPhysicalResist), 0_fp);
}

TEST_F(EffectSystemTest, ShieldedCondition)
{
    // Add some effect
    auto damage_effect_data = EffectData::Create(EffectType::kInstantDamage, EffectExpression::FromValue(100_fp));
    damage_effect_data.required_conditions.Add(ConditionType::kShielded);

    // Checking shielded condition
    // Note: Entity have a shield from start. Shield applied to entities in test setup.
    ASSERT_EQ(world->DoesPassConditions(*red_entity, damage_effect_data.required_conditions), true);
}

TEST_F(EffectSystemTest, ShieldedConditionDamage)
{
    auto shield_effect_data = EffectData::Create(EffectType::kSpawnShield, EffectExpression::FromValue(100_fp));
    shield_effect_data.attributes.to_shield_percentage = kMaxPercentageFP;
    shield_effect_data.required_conditions.Add(ConditionType::kShielded);

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        shield_effect_data,
        effect_state);

    // Create damage effect
    auto damage_effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

    EffectState damage_effect_state;
    damage_effect_state.is_critical = false;
    damage_effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    damage_effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        damage_effect,
        damage_effect_state);

    // After damage effect we have no shield
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Checking shielded condition
    ASSERT_EQ(world->DoesPassConditions(*red_entity, shield_effect_data.required_conditions), false);
}

TEST_F(EffectSystemTest, MinPhysicalVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalVampPercentage, kMinPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 0% physical vamp percentage so no healing should be done.
    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
}

TEST_F(EffectSystemTest, SomePhysicalVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalVampPercentage, 75_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86. 75% physical vamp means 64 health is gained
    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "164.772"_fp);
}

TEST_F(EffectSystemTest, MaxPhysicalVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalVampPercentage, kMaxPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "186.363"_fp);
}

TEST_F(EffectSystemTest, MinPureVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPureVampPercentage, kMinPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 0% pure vamp percentage so no healing should be done.
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
}

TEST_F(EffectSystemTest, SomePureVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPureVampPercentage, 75_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 pure damage is done with 75% pure vamp so 75 health is gained
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 175_fp);
}

TEST_F(EffectSystemTest, MaxPureVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPureVampPercentage, kMaxPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 pure damage is done so 100 health is gained
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 200_fp);
}

TEST_F(EffectSystemTest, MinEnergyVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyVampPercentage, kMinPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 0% energy vamp percentage so no healing should be done.
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
}

TEST_F(EffectSystemTest, SomeEnergyVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyVampPercentage, 75_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(105_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 20 (resolve) = 85 (5 energy_resist, which makes the
    // attack 95% effective) = 80
    // 75% energy vamp means 60 health is gained
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "160.714"_fp);
}

TEST_F(EffectSystemTest, MaxEnergyVamp)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyVampPercentage, kMaxPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(105_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 20 (resolve) = 85 (5 energy_resist, which makes the
    // attack 95% effective) = 80
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "180.952"_fp);
}

TEST_F(EffectSystemTest, PhysicalVampAndVampiric)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalVampPercentage, kMaxPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    effect.attached_effect_package_attributes.vampiric_percentage = EffectExpression::FromValue(50_fp);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86, 86 health gained through 100% physical vamp, 43 gained through effect 50% vampiric value
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "229.544"_fp);
}

TEST_F(EffectSystemTest, EnergyVampAndVampiric)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyVampPercentage, kMaxPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    EffectData effect;
    effect.type_id.type = EffectType::kInstantDamage;
    effect.type_id.damage_type = EffectDamageType::kEnergy;

    // attack energy damage + 100 extra
    {
        EffectExpression expression{};
        expression.operation_type = EffectOperationType::kAdd;
        expression.operands.push_back(EffectExpression::FromSenderLiveStat(StatType::kAttackEnergyDamage));
        expression.operands.push_back(EffectExpression::FromValue(100_fp));
        effect.SetExpression(expression);
    }

    // 50% vampiric
    effect.attached_effect_package_attributes.vampiric_percentage = EffectExpression::FromValue(50_fp);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 5 (attack energy damage) + 100 (extra) - 20 (resolve) = 95 (5 energy_resist, which makes the
    // attack 95% effective) = 85, 85 health gained through 100% energy_vamp, 42 health gained
    // through 50% vampiric
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "228.571"_fp);
}

TEST_F(EffectSystemTest, PureVampAndVampiric)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPureVampPercentage, kMaxPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));

    effect.attached_effect_package_attributes.vampiric_percentage = EffectExpression::FromValue(50_fp);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 pure damage, 100 health gained through 100% pure_vamp and
    // 50 health gained through 50% vampiric effect value
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 250_fp);
}

TEST_F(EffectSystemTest, VampiricEffectOnlyPhysicalDamage)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalVampPercentage, kMinPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    effect.attached_effect_package_attributes.vampiric_percentage = EffectExpression::FromValue(50_fp);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86, 43 gained through effect 50% vampiric value
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "143.181"_fp);
}

TEST_F(EffectSystemTest, VampiricEffectOnlyEnergyDamage)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyVampPercentage, kMinPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));

    effect.attached_effect_package_attributes.vampiric_percentage = EffectExpression::FromValue(50_fp);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 (effect) - 20 (resolve) = 80 (5 energy_resist, which makes the
    // attack 95% effective) = 76, 38 health gained through 50% vampiric effect
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    const auto current_health = blue_stats_component.GetCurrentHealth();
    constexpr auto expected_health = "138.095"_fp;
    ASSERT_EQ(current_health, expected_health);
}

TEST_F(EffectSystemTest, VampiricEffectOnlyPureDamage)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kPureVampPercentage, kMinPercentageFP);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(100_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));

    effect.attached_effect_package_attributes.vampiric_percentage = EffectExpression::FromValue(50_fp);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100 pure damage, 50 gained through effect 50% vampiric value
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 150_fp);
}

TEST_F(EffectSystemTest, ApplyEffectPureDamageFromAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kMaxHealth, 110_fp)
        .Set(StatType::kCurrentHealth, 110_fp)
        .Set(StatType::kAttackPureDamage, 5_fp);
    red_template_stats.Set(StatType::kMaxHealth, 110_fp).Set(StatType::kCurrentHealth, 110_fp);
    blue_stats_component.GetMutableTemplateStats();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 110_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 110_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 110_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 110_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(105_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) = 105 damage

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by 55 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 55_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 110_fp);
}

TEST_F(EffectSystemTest, ExcessVampToShield)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    // Set health
    blue_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kPhysicalVampPercentage, kMaxPercentageFP)
        .Set(StatType::kCurrentHealth, 500_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    // Set excess_vamp_to_shield so excess vamp that heals above max hp will be sent to the shield.
    effect.attached_effect_package_attributes.excess_vamp_to_shield = true;
    effect.attached_effect_package_attributes.excess_vamp_to_shield_duration_ms = 500;

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // Confirm red damaged the blue unit
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "463.637"_fp);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86, 36 health was healed, 50 shield recovered since 50 excess vamp occured
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(effect.attached_effect_package_attributes.excess_vamp_to_shield_duration_ms, 500);
}

TEST_F(EffectSystemTest, MinEffectDamageToShield)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set health
    blue_stats_component.SetCurrentHealth(500_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    // Set to_shield_percentage to 0% so none of the damage goes towards the shield
    effect.attributes.to_shield_percentage = kMinPercentageFP;
    effect.attributes.to_shield_duration_ms = 300;

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // Confirm red damaged the blue unit
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "463.637"_fp);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86, health should be reduced and 0 shield gained since to_shield_percentage = 0%
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "463.637"_fp);
    ASSERT_EQ(effect.attributes.to_shield_duration_ms, 300);
}

TEST_F(EffectSystemTest, SomeEffectDamageToShield)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set health
    blue_stats_component.SetCurrentHealth(500_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    // Set to_shield_percentage to 75% so some of the damage goes towards the shield
    effect.attributes.to_shield_percentage = 75_fp;
    effect.attributes.to_shield_duration_ms = 300;

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // Confirm red damaged the blue unit
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "463.637"_fp);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86, health should be reduced but 64 shield gained since to_shield_percentage = 75%
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), "64.772"_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "463.637"_fp);
    ASSERT_EQ(effect.attributes.to_shield_duration_ms, 300);
}

TEST_F(EffectSystemTest, MaxEffectDamageToShield)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set health
    blue_stats_component.SetCurrentHealth(500_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

    // Set to_shield_percentage to 100% so all of the damage goes towards the shield
    effect.attributes.to_shield_percentage = kMaxPercentageFP;
    effect.attributes.to_shield_duration_ms = 300;

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // Confirm red damaged the blue unit
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "463.637"_fp);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86, health should be reduced but 86 shield gained since to_shield_percentage = 100%
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), "86.363"_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), "463.637"_fp);
    ASSERT_EQ(effect.attributes.to_shield_duration_ms, 300);
}

TEST_F(EffectSystemTest, InstantHealMinEfficiency)
{
    // Send to ally
    auto& blue_stats_component2 = blue_entity2->Get<StatsComponent>();

    // Set health
    blue_stats_component2.SetCurrentHealth(100_fp);
    blue_stats_component2.GetMutableTemplateStats().Set(StatType::kHealthGainEfficiencyPercentage, kMinPercentageFP);
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 100_fp);

    // Create effect
    const auto effect = EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 0% health_gain_efficiency_percentage so no healing should be done.
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 100_fp);
}

TEST_F(EffectSystemTest, InstantHealSomeEfficiency)
{
    // Send to ally
    auto& blue_stats_component2 = blue_entity2->Get<StatsComponent>();

    // Set health
    blue_stats_component2.SetCurrentHealth(100_fp);
    blue_stats_component2.GetMutableTemplateStats().Set(StatType::kHealthGainEfficiencyPercentage, 75_fp);
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 100_fp);

    // Create effect
    const auto effect = EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity2->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 75% health_gain_efficiency_percentage so 75 healing should be done.
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 175_fp);
}

TEST_F(EffectSystemTest, InstantHealMaxEfficiency)
{
    // Send to ally
    auto& blue_stats_component2 = blue_entity2->Get<StatsComponent>();

    // Set health
    blue_stats_component2.SetCurrentHealth(100_fp);
    blue_stats_component2.GetMutableTemplateStats().Set(StatType::kHealthGainEfficiencyPercentage, kMaxPercentageFP);
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 100_fp);

    // Create effect
    const auto effect = EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity2->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 100% health_gain_efficiency_percentage so 100 healing should be done.
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 200_fp);
}

TEST_F(EffectSystemTest, InstantPureHeal)
{
    // Send to ally
    auto& blue_stats_component2 = blue_entity2->Get<StatsComponent>();

    // Set health
    blue_stats_component2.SetCurrentHealth(100_fp);
    blue_stats_component2.GetMutableTemplateStats().Set(StatType::kHealthGainEfficiencyPercentage, 75_fp);
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 100_fp);

    // Create effect
    const auto effect = EffectData::CreateHeal(EffectHealType::kPure, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity2->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // Pure heal so health_gain_efficiency_percentage should not effect heal
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 200_fp);
}

TEST_F(EffectSystemTest, InstantHealExcessHealthToShield)
{
    auto& blue_stats_component2 = blue_entity2->Get<StatsComponent>();
    auto& blue_template_stats2 = blue_stats_component2.GetMutableTemplateStats();

    // Set health
    blue_template_stats2.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kHealthGainEfficiencyPercentage, 100_fp);
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 500_fp);

    // Create effect
    auto effect = EffectData::CreateHeal(EffectHealType::kPure, EffectExpression::FromValue(100_fp));
    effect.attributes.excess_heal_to_shield = 1_fp;

    // Create effect
    const auto effect_damage =
        EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(80_fp));

    // Apply some damage
    {
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity2->GetID(),
            effect_damage,
            effect_state);
    }

    ASSERT_EQ(world->GetShieldTotal(*blue_entity2), 0_fp);
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), "486.364"_fp);

    // Apply some heal to an ally
    {
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            blue_entity2->GetID(),
            effect,
            effect_state);
    }

    // Manual calculation
    // Pure heal so health_gain_efficiency_percentage should not effect heal
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity2), "86.364"_fp);
}

TEST_F(EffectSystemTest, InstantHealPercentMissingHealth)
{
    auto& blue_stats_component2 = blue_entity2->Get<StatsComponent>();
    auto& blue_template_stats2 = blue_stats_component2.GetMutableTemplateStats();

    // Set health
    blue_template_stats2.Set(StatType::kMaxHealth, 500_fp * stats_scale)
        .Set(StatType::kCurrentHealth, 100_fp * stats_scale)
        .Set(StatType::kHealthGainEfficiencyPercentage, 100_fp);
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 100_fp * stats_scale);

    // Create effect
    auto effect = EffectData::CreateHeal(EffectHealType::kPure, EffectExpression::FromValue(100_fp * stats_scale));
    effect.attributes.missing_health_percentage_to_health = 50_fp;

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity2->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // Base heal amount = 100000, missing health = 400000 * missing_health_percentage_to_health
    // (.5) = 200000 heal Total heal 300000
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 400_fp * stats_scale);
}

TEST_F(EffectSystemTest, InstantHealPercentMaxHealth)
{
    auto& blue_stats_component2 = blue_entity2->Get<StatsComponent>();
    auto& blue_template_stats2 = blue_stats_component2.GetMutableTemplateStats();

    // Set health
    blue_template_stats2.Set(StatType::kMaxHealth, 500_fp * stats_scale)
        .Set(StatType::kCurrentHealth, 100_fp * stats_scale)
        .Set(StatType::kHealthGainEfficiencyPercentage, 100_fp);
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 100_fp * stats_scale);

    // Create effect
    auto effect = EffectData::CreateHeal(EffectHealType::kPure, EffectExpression::FromValue(100_fp * stats_scale));
    effect.attributes.max_health_percentage_to_health = 50_fp;

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity2->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // Base heal = 100, max_health_percentage_to_health  = .5, 500 * .5 = 250 heal
    // 350 heal + 100 so health is 450 * stats_scale(1000) = 450000
    ASSERT_EQ(blue_stats_component2.GetCurrentHealth(), 450_fp * stats_scale);
}

TEST_F(EffectSystemTest, CleanseAll)
{
    auto& attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();

    StatsData blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), 10_fp);

    // Create effect
    EffectData cleanse_effect;
    cleanse_effect.type_id.type = EffectType::kCleanse;
    cleanse_effect.attributes.cleanse_conditions = true;
    cleanse_effect.attributes.cleanse_debuffs = true;
    cleanse_effect.attributes.cleanse_negative_states = true;
    cleanse_effect.attributes.cleanse_dots = true;
    cleanse_effect.attributes.cleanse_bots = true;

    // Create debuff/negative/conditions
    auto debuff_effect_data =
        EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 500);
    auto negative_state_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 300);
    auto poison_effect_data =
        EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);
    auto dot_effect_data = EffectData::CreateDamageOverTime(
        EffectDamageType::kPhysical,
        EffectExpression::FromValue(100_fp),
        500,
        kDefaultAttachedEffectsFrequencyMs);
    auto energy_burn_effect_data = EffectData::CreateEnergyBurnOverTime(
        EffectExpression::FromValue(100_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    // Listen to events
    std::vector<event_data::Effect> events_removed_effect;
    world->SubscribeToEvent(
        EventType::kOnAttachedEffectRemoved,
        [&events_removed_effect](const Event& event)
        {
            events_removed_effect.emplace_back(event.Get<event_data::Effect>());
        });

    EffectState poison_effect_state;
    poison_effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        poison_effect_data,
        poison_effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        negative_state_effect_data,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        energy_burn_effect_data,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        debuff_effect_data,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        dot_effect_data,
        effect_state);

    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 6);

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));

    ASSERT_TRUE(attached_effects_component.HasEnergyBurn());

    ASSERT_TRUE(EntityHelper::IsPoisoned(*blue_entity));

    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), "1.1"_fp);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity->GetID(),
        cleanse_effect,
        effect_state);

    // Update current stats
    blue_live_stats = world->GetLiveStats(blue_entity->GetID());

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));

    ASSERT_FALSE(EntityHelper::IsPoisoned(*blue_entity));

    ASSERT_FALSE(attached_effects_component.HasEnergyBurn());

    // Current armour should 10 since the heal cleansed the debuff
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), 10_fp);

    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 6);
    attached_effects_component.EraseDestroyedEffects(true);
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 0);

    // Make sure we have the remove effects
    ASSERT_EQ(events_removed_effect.size(), 6);
}

TEST_F(EffectSystemTest, AllyApplyDetrimental)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);

    StatsData blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), 10_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(25_fp));

    auto dot_effect_data = EffectData::CreateDamageOverTime(
        EffectDamageType::kPhysical,
        EffectExpression::FromValue(100_fp),
        500,
        kDefaultAttachedEffectsFrequencyMs);

    // Create debuff
    auto debuff_effect = EffectData::CreateDebuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    auto negative_state_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 300);

    auto poison_effect_data =
        EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);

    const auto displacement_effect = EffectData::CreateDisplacement(EffectDisplacementType::kKnockUp, 300);

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        dot_effect_data,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        debuff_effect,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        negative_state_effect_data,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        poison_effect_data,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        displacement_effect,
        effect_state);

    // Manual calculation
    // 0 damage should be done due to agency check
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);

    // 0 damage should be done due to agency check
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));

    ASSERT_FALSE(EntityHelper::IsPoisoned(*blue_entity));

    // Blue 2 should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), 10_fp);

    // Knock up should not have occured
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
}

TEST_F(EffectSystemTest, EnemyApplyBeneficial)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& blue_attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);

    StatsData blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), 10_fp);

    // Create effect
    const auto instant_effect = EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(100_fp));
    const auto instant_pure_effect = EffectData::CreateHeal(EffectHealType::kPure, EffectExpression::FromValue(100_fp));

    // Create debuff
    auto buff_effect = EffectData::CreateBuff(StatType::kPhysicalResist, EffectExpression::FromValue(10_fp), 100);

    auto shield_effect_data = EffectData::Create(EffectType::kSpawnShield, EffectExpression::FromValue(100_fp));

    auto positive_state_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 300);

    auto empower_effect_data = EffectData::Create(EffectType::kEmpower, EffectExpression::FromValue(100_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        instant_effect,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        instant_pure_effect,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        buff_effect,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        shield_effect_data,
        effect_state);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        positive_state_effect_data,
        effect_state);

    // Manual calculation
    // 0 damage should be done due to agency check
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);

    ASSERT_FALSE(EntityHelper::IsInvulnerable(*blue_entity));
    ASSERT_EQ(blue_attached_effects_component.GetEmpowers().size(), 0);

    // 0 damage should be done due to agency check
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    // Blue 2 should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), 10_fp);
}

TEST_F(EffectSystemTest, ApplyBuffAndGetBaseBonusValues)
{
    constexpr FixedPoint initial_stat_value = 10_fp;
    constexpr FixedPoint buff_value = 20_fp;
    StatsData blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), initial_stat_value);

    // Create buff
    const auto buff_effect =
        EffectData::CreateBuff(StatType::kPhysicalResist, EffectExpression::FromValue(buff_value), 100);

    // Apply Effect
    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity->GetID(),
        buff_effect,
        effect_state);

    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kPhysicalResist), initial_stat_value + buff_value);

    // Test Base
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kPhysicalResist,
            StatEvaluationType::kBase,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(
            world->EvaluateExpression(expression, blue_entity->GetID(), blue_entity->GetID()),
            initial_stat_value);
    }

    // Test Live
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kPhysicalResist,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(
            world->EvaluateExpression(expression, blue_entity->GetID(), blue_entity->GetID()),
            initial_stat_value + buff_value);
    }

    // Test Bonus
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kPhysicalResist,
            StatEvaluationType::kBonus,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(world->EvaluateExpression(expression, blue_entity->GetID(), blue_entity->GetID()), buff_value);
    }
}

TEST_F(EffectSystemTest, ApplyAttackDamageAsEffect)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 10_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackEnergyDamage, 15_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPureDamage, 25_fp);

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    ASSERT_EQ(blue_stats_component.GetAttackDamage(), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(
        EffectDamageType::kPhysical,
        EffectExpression::FromSenderLiveStat(StatType::kAttackDamage));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 50 damage (from effect)
    // 10 grit -> 50 - 10 = 40
    // 10 armour make the attack 90% effective -> 40 * 0.9 = 36

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), "13.637"_fp);

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(EffectSystemTest, ApplyEffectWithMissingStat)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);

    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Set Physical and Energy to 100
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 100_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackEnergyDamage, 100_fp);

    // Set damage mitigation to 0 so we can see the damage effects apply
    red_stats_component.GetMutableTemplateStats().Set(StatType::kGrit, 0_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kResolve, 0_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalResist, 0_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyResist, 0_fp);

    // Create effect with expression
    auto effect = EffectData::CreateDamage(
        EffectDamageType::kPhysical,
        EffectExpression::FromStatPercentage(
            10_fp,
            StatType::kMaxHealth,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 50 because 50 was given to the shield and 50 was done by
    // effect which deals 10% of receiver hp
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 950_fp);
}

TEST_F(EffectSystemTest, ApplyEnergyBurn)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set energy and confirm the value
    red_stats_component.SetCurrentEnergy(100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 100_fp);

    // Create effect
    const auto effect = EffectData::Create(EffectType::kInstantEnergyBurn, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // 100 energy should be removed
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);
}

TEST_F(EffectSystemTest, ApplyEnergyGain)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set energy and confirm the value
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 500_fp);
    blue_stats_component.SetCurrentEnergy(100_fp);
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kEnergyCost), 500_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 100_fp);

    // Create effect
    const auto effect = EffectData::Create(EffectType::kInstantEnergyGain, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // 100 energy should be added
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 200_fp);
}

TEST_F(EffectSystemTest, ApplyEnergyBurnMoreThanMax)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set energy and confirm the value
    red_stats_component.SetCurrentEnergy(100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 100_fp);

    // Create effect
    const auto effect = EffectData::Create(EffectType::kInstantEnergyBurn, EffectExpression::FromValue(150_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // 100 energy should be removed, should not be able to be a negative value
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);
}

TEST_F(EffectSystemTest, ApplyBlinkAcross)
{
    // Access position component
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Verify starting position of blue
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(10, 10));

    // Create effect
    const auto effect = EffectData::CreateBlink(ReservedPositionType::kAcross, 0, 0);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    // Apply effect from blue to self
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Move on a step
    world->TimeStep();

    // Blue should have teleported
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(52, -74));
}

TEST_F(EffectSystemTest, ApplyBlinkWithReservation)
{
    // Access position component
    auto& blue_position_component = blue_entity->Get<PositionComponent>();
    blue_position_component.SetReservedPosition(HexGridPosition(26, -30));

    // Verify starting position of blue
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(10, 10));

    // Create effect
    const auto effect = EffectData::CreateBlink(ReservedPositionType::kAcross, 0, 0);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    // Apply effect from blue to self
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Move on a step
    world->TimeStep();

    // Blue should have teleported
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(26, -30));
}

TEST_F(EffectSystemTest, BlinkAbility)
{
    // Access position component
    auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Verify starting position of blue
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(10, 10));

    // Create blink ability
    AbilitiesData omega_abilities{};
    omega_abilities.selection_type = AbilitySelectionType::kCycle;

    // Create the ability
    auto& ability = omega_abilities.AddAbility();
    ability.is_use_once = true;
    auto& blink_skill = ability.AddSkill();

    // Set up the skill
    blink_skill.name = "Blink";
    blink_skill.targeting.type = SkillTargetingType::kSelf;
    blink_skill.deployment.type = SkillDeploymentType::kDirect;

    // Blink effect
    blink_skill.AddEffect(EffectData::CreateBlink(ReservedPositionType::kAcross, 0, 0));

    // Timing
    ability.total_duration_ms = 1000;
    blink_skill.percentage_of_ability_duration = 100;
    blink_skill.deployment.pre_deployment_delay_percentage = 20;

    // Add ability to blue
    auto& abilities_component = blue_entity->Get<AbilitiesComponent>();
    abilities_component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);

    // Allow blue to do omega
    auto stats_data = world->GetLiveStats(blue_entity->GetID());
    stats_data.Set(StatType::kEnergyCost, 0_fp);
    auto& stats_component = blue_entity->Get<StatsComponent>();
    stats_component.SetTemplateStats(stats_data);

    // Blue should not have reserved position yet
    ASSERT_EQ(blue_position_component.GetReservedPosition(), kInvalidHexHexGridPosition);

    // Time step before deployment
    TimeStepForNTimeSteps(2);

    // Blue should now have reserved position
    ASSERT_EQ(blue_position_component.GetReservedPosition(), HexGridPosition(52, -74));

    // Wait for deployment
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Reserved should now be actual position
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(52, -74));

    // Allow time for reservation to be cleared
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Position no longer reserved
    ASSERT_EQ(blue_position_component.GetReservedPosition(), kInvalidHexHexGridPosition);
}

TEST_F(EffectSystemTest, BlinkAbilityCancelled)
{
    // Access position component
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Verify starting position of blue
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(10, 10));

    // Create blink ability
    AbilitiesData omega_abilities{};
    omega_abilities.selection_type = AbilitySelectionType::kCycle;

    // Create the ability
    auto& ability = omega_abilities.AddAbility();
    ability.is_use_once = true;
    auto& blink_skill = ability.AddSkill();

    // Set up the skill
    blink_skill.name = "Blink";
    blink_skill.targeting.type = SkillTargetingType::kSelf;
    blink_skill.deployment.type = SkillDeploymentType::kDirect;

    // Blink effect
    blink_skill.AddEffect(EffectData::CreateBlink(ReservedPositionType::kAcross, 0, 0));

    // Timing
    ability.total_duration_ms = 1000;
    blink_skill.percentage_of_ability_duration = 100;
    blink_skill.deployment.pre_deployment_delay_percentage = 20;

    // Add ability to blue
    auto& abilities_component = blue_entity->Get<AbilitiesComponent>();
    abilities_component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);

    // Allow blue to do omega
    auto stats_data = world->GetLiveStats(blue_entity->GetID());
    stats_data.Set(StatType::kEnergyCost, 0_fp);
    auto& stats_component = blue_entity->Get<StatsComponent>();
    stats_component.SetTemplateStats(stats_data);

    // Blue should not have reserved position yet
    ASSERT_EQ(blue_position_component.GetReservedPosition(), kInvalidHexHexGridPosition);

    // Time step before deployment
    TimeStepForNTimeSteps(2);

    // Blue should now have reserved position
    ASSERT_EQ(blue_position_component.GetReservedPosition(), HexGridPosition(52, -74));

    // Stun effect
    EffectData stun_effect = EffectData::CreateNegativeState(EffectNegativeState::kStun, 100);

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    // Apply stun effect from red to blue
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        stun_effect,
        effect_state);

    // Pass time to make sure skill doesn't deploy
    TimeStepForNTimeSteps(2);

    const auto& attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();
    ASSERT_FALSE(attached_effects_component.HasBlink());

    // Blue did not move
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(10, 10));

    // Position no longer reserved
    ASSERT_EQ(blue_position_component.GetReservedPosition(), kInvalidHexHexGridPosition);

    // Make sure reserved position is no longer an obstacle (for red entity)
    GetGridHelper().BuildObstacles(red_entity);
    ASSERT_FALSE(GetGridHelper().HasObstacleAt(HexGridPosition(52, -74)));
}

TEST_F(EffectSystemTest, ApplyEffectShieldBypass)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 1000_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    EffectData effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(255_fp));
    effect.attributes.shield_bypass = true;

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // No shield should be destroyed due to shield bypass
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Manual calculation
    // (255 (effect)) - 10 (grit) = 10 (10 armour, which makes the attack 90%
    // effective) = 222 health should be destroyed
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "777.273"_fp);
}

TEST_F(EffectSystemTest, ShieldBypassDamageOverTime)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 0_fp);

    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    EffectData effect = EffectData::CreateDamageOverTime(
        EffectDamageType::kPhysical,
        EffectExpression::FromValue(200_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);
    effect.attributes.shield_bypass = true;

    EffectState effect_state;
    effect_state.is_critical = false;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);

    // Physical resist and grit 10
    for (int i = 1; i < 11; i++)
    {
        world->TimeStep();
        ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);
        ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp - FixedPoint::FromInt(i) * "9.09"_fp) << i;
    }
}

TEST_F(EffectSystemTest, BlindCannotCleanse)
{
    auto& attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();

    // Create effect
    EffectData cleanse_effect;
    cleanse_effect.type_id.type = EffectType::kCleanse;
    cleanse_effect.attributes.cleanse_negative_states = true;

    // Create negative
    auto negative_state_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 300);
    negative_state_effect_data.can_cleanse = false;

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        negative_state_effect_data,
        effect_state);

    ASSERT_TRUE(EntityHelper::IsBlinded(*blue_entity));

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity->GetID(),
        cleanse_effect,
        effect_state);

    ASSERT_TRUE(EntityHelper::IsBlinded(*blue_entity));

    // 0 Attached effects should be present
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 1);
}

TEST_F(EffectSystemTest, ApplyHyperGain)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set hyper and confirm the value
    blue_stats_component.SetCurrentSubHyper(0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentSubHyper(), 0_fp);

    // Create effect
    const auto effect = EffectData::Create(EffectType::kInstantHyperGain, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // 100 hyper should be added
    ASSERT_EQ(blue_stats_component.GetCurrentSubHyper(), 100_fp);
}

TEST_F(EffectSystemTest, ApplyHyperBurn)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set hyper and confirm the value
    blue_stats_component.SetCurrentSubHyper(100_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentSubHyper(), 100_fp);

    // Create effect
    const auto effect = EffectData::Create(EffectType::kInstantHyperBurn, EffectExpression::FromValue(100_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // 100 hyper should be burnt
    ASSERT_EQ(blue_stats_component.GetCurrentSubHyper(), 0_fp);
}

TEST_F(EffectSystemTest, ApplyEffectPhysicalDamageCriticalFromAttackWithCritReduction)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 0_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kGrit, 0_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kPhysicalResist, 0_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCritReductionPercentage, 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

    // Apply Effect with critical

    EffectState effect_state;
    effect_state.is_critical = true;
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.sender_stats.live.Set(StatType::kCritAmplificationPercentage, kDefaultCritAmplificationPercentage);

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // Manual calculation
    // 150% crit_amplification is 50 bonus damage from crit, reduced 50% by crit_reduction_percentage
    // so 25, effect should do
    // 125 damage

    // Shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 15 because 50 was given to the shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 925_fp);
}

TEST_F(BaseTest, BuffWithSenderCurrentFocus)
{
    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    EventHistory<EventType::kOnEffectApplied> applied_effects(*world);
    EventHistory<EventType::kFainted> fainted(*world);

    CombatUnitData data = CreateCombatUnitData();

    // Add attack ability for all units
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        ability.name = "Foo";

        // Skills
        auto& skill = ability.AddSkill();
        skill.name = "Attack";

        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.SetDefaults(AbilityType::kAttack);
        skill.AddDamageEffect(
            EffectDamageType::kPhysical,
            EffectExpression::FromSenderLiveStat(StatType::kAttackDamage));

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    // Stats
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);
    data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 1_fp);
    data.type_data.stats.Set(StatType::kAttackEnergyDamage, 0_fp);
    data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
    // no need movement in this kind of tests so let units attack the whole map
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 9999_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 10000_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 900_fp);
    data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

    Entity* blue_a = nullptr;
    Entity* blue_b = nullptr;
    Entity* red = nullptr;

    data.type_data.stats.Set(StatType::kMaxHealth, 900_fp);
    SpawnCombatUnit(Team::kBlue, {0, 10}, data, blue_a);

    data.type_data.stats.Set(StatType::kMaxHealth, 2700_fp);
    SpawnCombatUnit(Team::kBlue, {-10, 10}, data, blue_b);

    // Add omega for red
    {
        auto& ability = data.type_data.omega_abilities.AddAbility();
        ability.name = "Foo";

        // Skills
        auto& skill = ability.AddSkill();
        skill.name = "Attack";

        EffectExpression expr;
        expr.operation_type = EffectOperationType::kDivide;
        expr.operands.push_back(EffectExpression::FromSenderFocusLiveStat(StatType::kMaxHealth));
        expr.operands.push_back(EffectExpression::FromValue(3_fp));
        EffectData red_buff = EffectData::CreateBuff(StatType::kAttackPhysicalDamage, expr, 5000);

        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.SetDefaults(AbilityType::kOmega);
        skill.AddEffect(red_buff);

        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    data.type_data.stats.Set(StatType::kStartingEnergy, data.type_data.stats.Get(StatType::kEnergyCost));
    data.type_data.stats.Set(StatType::kMaxHealth, 8100_fp);
    SpawnCombatUnit(Team::kRed, {0, 15}, data, red);

    auto get_abilities_count = [&activated_abilities](AbilityType ability_type)
    {
        return std::count_if(
            activated_abilities.events.begin(),
            activated_abilities.events.end(),
            [&](const event_data::AbilityActivated& event)
            {
                return event.ability_type == ability_type;
            });
    };

    auto get_effects_count = [&applied_effects](EffectType effect_type)
    {
        return std::count_if(
            applied_effects.events.begin(),
            applied_effects.events.end(),
            [&](const event_data::Effect& event)
            {
                return event.data.type_id.type == effect_type;
            });
    };

    const EntityID blue_a_id = blue_a->GetID();

    ///////////////////////// Actual test begins

    // Part 1 - red entity applies an omega ability with buff to itself
    world->TimeStep();

    ASSERT_EQ(get_abilities_count(AbilityType::kAttack), 2);
    ASSERT_EQ(get_abilities_count(AbilityType::kOmega), 1);
    ASSERT_EQ(get_effects_count(EffectType::kInstantDamage), 2);
    ASSERT_EQ(get_effects_count(EffectType::kBuff), 1);

    EXPECT_EQ(blue_a->Get<StatsComponent>().GetCurrentHealth(), 900_fp);
    EXPECT_EQ(blue_b->Get<StatsComponent>().GetCurrentHealth(), 2700_fp);
    EXPECT_EQ(red->Get<StatsComponent>().GetCurrentHealth(), 8098_fp);

    EXPECT_EQ(world->GetLiveStats(*red).Get(StatType::kAttackPhysicalDamage), 301_fp);

    applied_effects.Clear();
    activated_abilities.Clear();

    // Part 2 - do a few time steps to kill blue_a entity
    TimeStepForNTimeSteps(3);

    ASSERT_EQ(get_abilities_count(AbilityType::kAttack), 9);
    ASSERT_EQ(get_effects_count(EffectType::kInstantDamage), 9);
    ASSERT_EQ(fainted.Size(), 1);
    ASSERT_EQ(fainted[0].entity_id, blue_a_id);

    EXPECT_EQ(blue_b->Get<StatsComponent>().GetCurrentHealth(), 2700_fp);
    EXPECT_EQ(red->Get<StatsComponent>().GetCurrentHealth(), 8092_fp);

    applied_effects.Clear();
    activated_abilities.Clear();

    // Part 3 - One extra time step so that blue_a can refocus on a new target
    TimeStepForNTimeSteps(1);
    ASSERT_EQ(get_abilities_count(AbilityType::kAttack), 1);
    ASSERT_EQ(get_effects_count(EffectType::kInstantDamage), 1);

    EXPECT_EQ(blue_b->Get<StatsComponent>().GetCurrentHealth(), 2700_fp);
    EXPECT_EQ(red->Get<StatsComponent>().GetCurrentHealth(), 8091_fp);

    applied_effects.Clear();
    activated_abilities.Clear();

    // Part 4 - Ensure that the buff is still there and it's value did not change
    TimeStepForNTimeSteps(1);

    ASSERT_EQ(get_abilities_count(AbilityType::kAttack), 2);
    ASSERT_EQ(get_effects_count(EffectType::kInstantDamage), 2);

    EXPECT_EQ(blue_b->Get<StatsComponent>().GetCurrentHealth(), 2399_fp);
    EXPECT_EQ(red->Get<StatsComponent>().GetCurrentHealth(), 8090_fp);
}

TEST_F(BaseTest, EnergyOnTakeDamage)
{
    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    EventHistory<EventType::kOnEffectApplied> applied_effects(*world);
    EventHistory<EventType::kFainted> fainted(*world);

    CombatUnitData data = CreateCombatUnitData();

    // Add attack ability for all units
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        ability.name = "Foo";

        // Skills
        auto& skill = ability.AddSkill();
        skill.name = "Attack";

        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.SetDefaults(AbilityType::kAttack);
        skill.AddDamageEffect(
            EffectDamageType::kPhysical,
            EffectExpression::FromSenderLiveStat(StatType::kAttackDamage));

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    // Stats
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);
    data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 100_fp);
    data.type_data.stats.Set(StatType::kAttackEnergyDamage, 0_fp);
    data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 9999_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 10000_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
    data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);
    data.type_data.stats.Set(StatType::kGrit, 10_fp);

    // Has no abilities (dummy)
    Entity* blue = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, 10}, data, blue);

    Entity* red = nullptr;
    SpawnCombatUnit(Team::kRed, {0, 15}, data, red);

    EventHistory<EventType::kEnergyChanged> energy_changed_events(*world);
    EventHistory<EventType::kOnDamage> damage_received_events(*world);

    world->TimeStep();
    ASSERT_EQ(damage_received_events.Size(), 2);
    ASSERT_EQ(damage_received_events[0].damage_value, 90_fp);
    ASSERT_EQ(damage_received_events[1].damage_value, 90_fp);

    ASSERT_EQ(energy_changed_events.Size(), 4);
    ASSERT_EQ(energy_changed_events[0].delta, 1_fp);  // regen per time step
    ASSERT_EQ(energy_changed_events[1].delta, ((90_fp * 100_fp) / (90_fp)) / 20_fp);
}

}  // namespace simulation
