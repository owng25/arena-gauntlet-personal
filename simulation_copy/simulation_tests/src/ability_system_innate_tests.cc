#include "ability_system_innate_data_fixtures.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "gtest/gtest.h"
#include "systems/ability_system.h"

namespace simulation
{
TEST_F(AbilitySystemInnateTest_OnReceiveDamage, ApplyEffectWithOnTakeDamageInnateAbility)
{
    // 1. activation of just attacks
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should have only fired once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_OnDealPhysDamage, ApplyEffectWithOnEnemyDealPhyscialDamageInnateAbility)
{
    // 1. activation of just attacks
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 8) << "Should have fired attack + innate for both";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(4).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(5).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(6).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(7).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnDealEnergyDamage, ApplyEffectWithOnEnemyDealEnergyDamageInnateAbility)
{
    // 1. activation of just attacks
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 4)
        << "Should have fired attack. Innates should not pass damage type check.";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_OnDealDamageAndThreshold, ApplyEffectWithOnEnemyDealDamageAndThreshold)
{
    // 1. activation of just attacks
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 4)
        << "Should have fired attack. Innates should not pass damage threshold check.";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnDealCrit, ActivateInnateAbilityOnDealCrit)
{
    TimeStepForMs(1500);

    ASSERT_EQ(events_ability_activated.size(), 8) << "Should have fired attack + innate for both";

    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_TRUE(events_ability_activated.at(0).is_critical);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);

    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_TRUE(events_ability_activated.at(2).is_critical);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kInnate);

    EXPECT_EQ(events_ability_activated.at(4).ability_type, AbilityType::kAttack);
    EXPECT_TRUE(events_ability_activated.at(4).is_critical);
    EXPECT_EQ(events_ability_activated.at(5).ability_type, AbilityType::kInnate);

    EXPECT_EQ(events_ability_activated.at(6).ability_type, AbilityType::kAttack);
    EXPECT_TRUE(events_ability_activated.at(6).is_critical);
    EXPECT_EQ(events_ability_activated.at(7).ability_type, AbilityType::kInnate);

    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnEnemyMiss, ApplyEffectWithOnEnemyMissInnateAbility)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    // 1. activation of just attacks
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // Make blue always miss
    blue_template_stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);

    // 2. activation of attack
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have fired only once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 3. activation of next attack
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have fired only once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnVanquish, ApplyEffect)
{
    EventHistory<EventType::kFainted> events_fainted(*world);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // Make blue die
    blue_template_stats.Set(StatType::kMaxHealth, 200_fp).Set(StatType::kCurrentHealth, 200_fp);
    red_template_stats.Set(StatType::kMaxHealth, 500_fp).Set(StatType::kCurrentHealth, 500_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    // 1. activation of just attacks
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // Spawn new target
    Entity* blue_entity3 = nullptr;
    SpawnCombatUnit(Team::kBlue, {30, 30}, data, blue_entity3);

    // 2. activation of attack ~ blue dies after 2 attacks, which activates takedown innate
    TimeStepForMs(200);

    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(events_fainted[0].entity_id, blue_entity->GetID());
    EXPECT_EQ(events_fainted[0].vanquisher_id, red_entity->GetID());

    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();

    // Spawn new target
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity2);

    // 3. activation, takedown innate should have fired
    TimeStepForMs(400);
    ASSERT_EQ(events_ability_activated.size(), 4);
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_OnVanquish, Allegiance)
{
    auto set_hp = [](Entity& entity, FixedPoint health)
    {
        auto& stats_component = entity.Get<StatsComponent>();
        auto& template_stats = stats_component.GetMutableTemplateStats();
        template_stats.Set(StatType::kMaxHealth, health);
        template_stats.Set(StatType::kCurrentHealth, health);
    };

    // Create combat unit data with kAlly allegiance in innate ability trigger
    auto ally_allegiance_data = ModifyInnate(
        [&](AbilityData& ability)
        {
            ability.activation_trigger_data.sender_allegiance = AllegianceType::kAlly;
        });

    // Create combat unit data with kEnemy allegiance in innate ability trigger
    auto enemy_allegiance_data = ModifyInnate(
        [&](AbilityData& ability)
        {
            ability.activation_trigger_data.sender_allegiance = AllegianceType::kEnemy;
        });

    // Create combat unit data with kSelf allegiance in innate ability trigger
    auto self_allegiance_data = ModifyInnate(
        [&](AbilityData& ability)
        {
            ability.activation_trigger_data.sender_allegiance = AllegianceType::kSelf;
        });

    // Blue use kAlly allegiance - when ally kills somebody
    SpawnBlue({-50, 10}, ally_allegiance_data);
    SpawnBlue({0, 10}, ally_allegiance_data);
    SpawnBlue({50, 10}, ally_allegiance_data);

    // Red[0] uses kSelf allegiance - when self kills somebody
    SpawnRed({-50, 15}, self_allegiance_data);
    // Red[1..2] use kEnemy allegiance - when enemy kills somebody
    SpawnRed({0, 15}, enemy_allegiance_data);
    SpawnRed({50, 15}, enemy_allegiance_data);

    set_hp(GetBlueEntity(0), 100_fp);
    set_hp(GetBlueEntity(1), 400_fp);
    set_hp(GetBlueEntity(2), 400_fp);
    set_hp(GetRedEntity(0), 400_fp);
    set_hp(GetRedEntity(1), 400_fp);
    set_hp(GetRedEntity(2), 200_fp);

    EventHistory<EventType::kFainted> events_fainted(*world);

    // Step 0: red[0] kills blue[0]
    // blue:  xxx 300 300
    // red: 300 300 100
    TimeStepForMs(100);

    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(events_fainted.events.front().entity_id, blue_entities_ids[0]);

    // blue entity was spawned first so it attacks too
    ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 6);

    // One innate activated here from red 0 (due to kSelf alleginace)
    ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 1);
    ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, red_entities_ids[0]), 1);

    events_ability_activated.clear();
    events_fainted.events.clear();

    // Step 1: blue[2] kills red[2]
    // blue:      200 300
    // red: 300 200 xxx
    TimeStepForMs(100);

    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(events_fainted.events.front().entity_id, red_entities_ids[2]);

    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 3);

    // blue[1] activates innate since blue[2] killed an enemy
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[1]), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[2]), 0);

    // red[0] does not activate innate because it has kSelf allegiance
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, red_entities_ids[0]), 0);

    // red[1] activates innate since blue entity kills an enemy
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, red_entities_ids[1]), 1);
}

TEST_F(AbilitySystemInnateTest_OnVanquish, Range)
{
    auto set_hp = [](Entity& entity, FixedPoint health)
    {
        auto& stats_component = entity.Get<StatsComponent>();
        auto& template_stats = stats_component.GetMutableTemplateStats();
        template_stats.Set(StatType::kMaxHealth, health);
        template_stats.Set(StatType::kCurrentHealth, health);
    };

    auto distance = [](const Entity& a, const Entity& b) -> int
    {
        const auto position_a = a.Get<PositionComponent>().GetPosition();
        const auto position_b = b.Get<PositionComponent>().GetPosition();
        return (position_a - position_b).Length();
    };

    constexpr int range_limit = 60;

    // Create combat unit data with range and allegiance restrictions in innate ability trigger
    auto mod_data = ModifyInnate(
        [&](AbilityData& ability)
        {
            ability.activation_trigger_data.range_units = range_limit;
            ability.activation_trigger_data.sender_allegiance = AllegianceType::kAlly;
        });

    SpawnBlue({-50, 10}, mod_data);
    SpawnBlue({0, 10}, mod_data);
    SpawnBlue({50, 10}, mod_data);

    SpawnRed({-50, 15}, mod_data);
    SpawnRed({0, 15}, mod_data);
    SpawnRed({50, 15}, mod_data);

    EXPECT_LT(distance(GetBlueEntity(0), GetBlueEntity(1)), range_limit);
    EXPECT_LT(distance(GetBlueEntity(0), GetRedEntity(1)), range_limit);
    EXPECT_GT(distance(GetBlueEntity(0), GetRedEntity(2)), range_limit);
    EXPECT_GT(distance(GetBlueEntity(0), GetRedEntity(2)), range_limit);

    set_hp(GetBlueEntity(0), 400_fp);
    set_hp(GetBlueEntity(1), 400_fp);
    set_hp(GetBlueEntity(2), 400_fp);
    set_hp(GetRedEntity(0), 400_fp);
    set_hp(GetRedEntity(1), 100_fp);
    set_hp(GetRedEntity(2), 200_fp);

    EventHistory<EventType::kFainted> events_fainted(*world);

    // Step 0: blue[1] kills red[1]
    // blue:  300 400 300
    // red: 300 xxx 100
    TimeStepForMs(100);

    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(events_fainted.events.front().entity_id, red_entities_ids[1]);

    ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 5);

    // One innate activated here from red 0 (due to kSelf alleginace)
    ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 2);
    ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[0]), 1);
    ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[2]), 1);

    events_ability_activated.clear();
    events_fainted.events.clear();

    // Step 1: blue[2] kills red[2]
    // blue:  200 400 300
    // red: 200 xxx xxx
    TimeStepForMs(100);

    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(events_fainted.events.front().entity_id, red_entities_ids[2]);

    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 3);

    // blue[1] activates innate since blue[2] killed an enemy in range
    // but blue blue[0] is too far away
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[1]), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[0]), 0);
}

TEST_F(AbilitySystemInnateTest_OnVanquish, Counter)
{
    auto set_hp = [](Entity& entity, FixedPoint health)
    {
        auto& stats_component = entity.Get<StatsComponent>();
        auto& template_stats = stats_component.GetMutableTemplateStats();
        template_stats.Set(StatType::kMaxHealth, health);
        template_stats.Set(StatType::kCurrentHealth, health);
    };

    // Make unit data for each variant of comparison type

    // steps 1 2
    auto data_less_than_3 = ModifyInnate(
        [&](AbilityData& ability)
        {
            ability.activation_trigger_data.every_x = false;
            ability.activation_trigger_data.comparison_type = ComparisonType::kLess;
            ability.activation_trigger_data.trigger_value = 3;
        });

    // steps 6 7 (not 8 - battle finished)
    auto data_greater_than_5 = ModifyInnate(
        [&](AbilityData& ability)
        {
            ability.activation_trigger_data.every_x = false;
            ability.activation_trigger_data.comparison_type = ComparisonType::kGreater;
            ability.activation_trigger_data.trigger_value = 5;
        });

    // step 4
    auto data_equal_4 = ModifyInnate(
        [&](AbilityData& ability)
        {
            ability.activation_trigger_data.every_x = false;
            ability.activation_trigger_data.comparison_type = ComparisonType::kEqual;
            ability.activation_trigger_data.trigger_value = 4;
        });

    // step 3 6
    auto data_every_3 = ModifyInnate(
        [&](AbilityData& ability)
        {
            ability.activation_trigger_data.every_x = true;
            ability.activation_trigger_data.trigger_value = 3;
        });

    auto no_innate = data;
    no_innate.type_data.innate_abilities.abilities.clear();

    // 16 entities with different number of health so that one entity dies each time step
    // index in team:   0    1    2    3    4    5    6    7
    //         blue: 9999 9999 9999 9999 9999 9999 9999 9999
    //        red:  100  200  300  400  500  600  700  800

    SpawnBlue({-70, 10}, data_less_than_3);
    SpawnBlue({-50, 10}, data_greater_than_5);
    SpawnBlue({-30, 10}, data_equal_4);
    SpawnBlue({-10, 10}, data_every_3);
    SpawnBlue({10, 10}, no_innate);
    SpawnBlue({30, 10}, no_innate);
    SpawnBlue({50, 10}, no_innate);
    SpawnBlue({60, 10}, no_innate);

    SpawnRed({-70, 15}, no_innate);
    SpawnRed({-50, 15}, no_innate);
    SpawnRed({-30, 15}, no_innate);
    SpawnRed({-10, 15}, no_innate);
    SpawnRed({10, 15}, no_innate);
    SpawnRed({30, 15}, no_innate);
    SpawnRed({50, 15}, no_innate);
    SpawnRed({60, 15}, no_innate);

    for (size_t i = 0; i != 8; ++i)
    {
        set_hp(GetBlueEntity(i), 9999_fp);
        set_hp(GetRedEntity(i), FixedPoint::FromInt(static_cast<int>(i + 1) * 100));
    }

    EventHistory<EventType::kFainted> events_fainted(*world);

    // 1
    TimeStepForMs(100);  // blue[0]
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[0]), 1);
    events_fainted.events.clear();
    events_ability_activated.clear();

    // 2
    TimeStepForMs(100);  // blue[0]
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[0]), 1);
    events_fainted.events.clear();
    events_ability_activated.clear();

    // 3
    TimeStepForMs(100);  // blue[3]
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[3]), 1);
    events_fainted.events.clear();
    events_ability_activated.clear();

    // 4
    TimeStepForMs(100);  // blue[2]
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[2]), 1);
    events_fainted.events.clear();
    events_ability_activated.clear();

    // 5
    TimeStepForMs(100);
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 0);
    events_fainted.events.clear();
    events_ability_activated.clear();

    // 6
    TimeStepForMs(100);  // blue[1] blue[3]
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 2);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[1]), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[3]), 1);
    events_fainted.events.clear();
    events_ability_activated.clear();

    // 7
    TimeStepForMs(100);  // blue[1]
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entities_ids[1]), 1);
    events_fainted.events.clear();
    events_ability_activated.clear();

    // 8. Here innate of blue[1] may be activated one more time
    // but battle is finished.
    TimeStepForMs(100);  // blue[1]
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 0);
    events_fainted.events.clear();
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnAllyFaint, ApplyEffect)
{
    EventHistory<EventType::kFainted> events_fainted(*world);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // Make blue die
    blue_template_stats.Set(StatType::kMaxHealth, 100_fp).Set(StatType::kCurrentHealth, 100_fp);
    red_template_stats.Set(StatType::kMaxHealth, 500_fp).Set(StatType::kCurrentHealth, 500_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    // 1. activation of just attacks, ally should have fainted
    TimeStepForMs(200);

    // Red should have vanquished blue
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(events_fainted[0].entity_id, blue_entity->GetID());
    EXPECT_EQ(events_fainted[0].vanquisher_id, red_entity->GetID());

    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);

    // Should just be basic attacks
    ASSERT_EQ(events_effect_package_received.size(), 3);
    EXPECT_EQ(events_effect_package_received.at(0).combat_unit_sender_id, blue_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(0).receiver_id, red_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_effect_package_received.at(1).combat_unit_sender_id, red_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(1).receiver_id, blue_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(1).ability_type, AbilityType::kAttack);

    // From innate
    EXPECT_EQ(events_effect_package_received.at(2).combat_unit_sender_id, blue_entity2->GetID());
    EXPECT_EQ(events_effect_package_received.at(2).receiver_id, red_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(2).ability_type, AbilityType::kInnate);
}

TEST_F(AbilitySystemInnateTest_WithOnFaint, ApplyEffect)
{
    EventHistory<EventType::kFainted> events_fainted(*world);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // Make blue die
    blue_template_stats.Set(StatType::kMaxHealth, 100_fp).Set(StatType::kCurrentHealth, 100_fp);
    red_template_stats.Set(StatType::kMaxHealth, 500_fp).Set(StatType::kCurrentHealth, 500_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    // 1. activation of just attacks, ally should have fainted
    TimeStepForMs(200);

    // Red should have vanquished blue
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(events_fainted[0].entity_id, blue_entity->GetID());
    EXPECT_EQ(events_fainted[0].vanquisher_id, red_entity->GetID());

    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for one entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);

    // Should just be basic attacks
    ASSERT_EQ(events_effect_package_received.size(), 3);
    EXPECT_EQ(events_effect_package_received.at(0).combat_unit_sender_id, blue_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(0).receiver_id, red_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_effect_package_received.at(1).combat_unit_sender_id, red_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(1).receiver_id, blue_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(1).ability_type, AbilityType::kAttack);

    // From innate
    EXPECT_EQ(events_effect_package_received.at(2).combat_unit_sender_id, blue_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(2).receiver_id, red_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(2).ability_type, AbilityType::kInnate);

    // Not in range for anything else
    events_ability_activated.clear();
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 0);
}

TEST_F(AbilitySystemInnateTest_WithOnAssist, ApplyEffect)
{
    EventHistory<EventType::kFainted> events_fainted(*world);

    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // Make blue die
    red_template_stats.Set(StatType::kMaxHealth, 150_fp).Set(StatType::kCurrentHealth, 150_fp);

    // 1. activation innates ab
    TimeStepForMs(400);

    ASSERT_EQ(events_ability_activated.size(), 6) << "Should have only fired once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);

    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).sender_id, blue_entity->GetID());

    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(3).sender_id, blue_entity2->GetID());

    EXPECT_EQ(events_ability_activated.at(4).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(4).sender_id, blue_entity->GetID());

    EXPECT_EQ(events_ability_activated.at(5).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(5).sender_id, blue_entity2->GetID());

    // // Blue should have vanquished red
    ASSERT_EQ(events_fainted.Size(), 1);
    EXPECT_EQ(events_fainted[0].entity_id, red_entity->GetID());
    EXPECT_EQ(events_fainted[0].vanquisher_id, blue_entity2->GetID());

    // Should just be from innates and the attacks that come immediately after
    ASSERT_EQ(events_effect_package_received.size(), 6);
    EXPECT_EQ(events_effect_package_received.at(2).combat_unit_sender_id, blue_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(2).receiver_id, red_entity2->GetID());
    EXPECT_EQ(events_effect_package_received.at(2).ability_type, AbilityType::kInnate);

    EXPECT_EQ(events_effect_package_received.at(3).combat_unit_sender_id, blue_entity2->GetID());
    EXPECT_EQ(events_effect_package_received.at(3).receiver_id, red_entity2->GetID());
    EXPECT_EQ(events_effect_package_received.at(3).ability_type, AbilityType::kInnate);

    EXPECT_EQ(events_effect_package_received.at(4).combat_unit_sender_id, blue_entity->GetID());
    EXPECT_EQ(events_effect_package_received.at(4).receiver_id, red_entity2->GetID());
    EXPECT_EQ(events_effect_package_received.at(4).ability_type, AbilityType::kAttack);

    EXPECT_EQ(events_effect_package_received.at(5).combat_unit_sender_id, blue_entity2->GetID());
    EXPECT_EQ(events_effect_package_received.at(5).receiver_id, red_entity2->GetID());
    EXPECT_EQ(events_effect_package_received.at(5).ability_type, AbilityType::kAttack);
}

TEST_F(AbilitySystemInnateTest_WithOnDodge, ApplyEffectWithOnDodgeInnateAbility)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // 1. activation of just attacks
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // Mark blue to always dodge
    blue_template_stats.Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);

    // 2. activation of attack
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for both entities + 1 innate(dodge)";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();

    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for both entities + 1 innate(dodge)";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();

    // Mark red as always dodge
    red_template_stats.Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);

    // 3. activation of attacks
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should have fired innate + attack for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();

    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should have fired innate + attack for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnDodgeCurrentFocus, OnEnemyDodgeApplyOnFocusOnly)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_2_stats_component = red_entity_2->Get<StatsComponent>();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();
    auto& red_2_template_stats = red_2_stats_component.GetMutableTemplateStats();

    auto get_abilites_count = [&](AbilityType ability_type)
    {
        size_t count = 0;
        for (const auto& event : events_ability_activated)
        {
            if (event.ability_type == ability_type)
            {
                ++count;
            }
        }

        return count;
    };

    // 1. activation of just attacks
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for both entities";
    ASSERT_EQ(get_abilites_count(AbilityType::kAttack), 3);

    events_ability_activated.clear();

    // set dodge chance for red and red 2 to maximum
    red_template_stats.Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);
    red_2_template_stats.Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);

    // 2. activation of attack
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should have only fired once for all entities + 1 innate(dodge)";
    ASSERT_EQ(get_abilites_count(AbilityType::kAttack), 3);
    ASSERT_EQ(get_abilites_count(AbilityType::kInnate), 1);

    // Find who was attacked by blue entity
    EntityID attacked_entity = kInvalidEntityID;
    EntityID innate_receiver = kInvalidEntityID;
    for (const auto& event : events_ability_activated)
    {
        if (event.sender_id == blue_entity->GetID())
        {
            ASSERT_EQ(event.predicted_receiver_ids.size(), 1);
            if (event.ability_type == AbilityType::kAttack)
            {
                attacked_entity = event.predicted_receiver_ids.front();
            }
            else if (event.ability_type == AbilityType::kInnate)
            {
                innate_receiver = event.predicted_receiver_ids.front();
            }
            else
            {
                assert(false);
            }
        }
    }
    EXPECT_NE(attacked_entity, kInvalidEntityID);
    EXPECT_EQ(attacked_entity, innate_receiver);

    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnDodgeApplyToSelf, OnEnemyDodgeApplyOnSelf)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    auto get_abilites_count = [&](AbilityType ability_type)
    {
        size_t count = 0;
        for (const auto& event : events_ability_activated)
        {
            if (event.ability_type == ability_type)
            {
                ++count;
            }
        }

        return count;
    };

    // 1. activation of just attacks
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(get_abilites_count(AbilityType::kAttack), 2);

    events_ability_activated.clear();

    // set dodge chance for red and red 2 to maximum
    red_template_stats.Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);

    // 2. activation of attack
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for all entities + 1 innate(dodge)";
    ASSERT_EQ(get_abilites_count(AbilityType::kAttack), 2);
    ASSERT_EQ(get_abilites_count(AbilityType::kInnate), 1);

    // Find who was attacked by blue entity
    EntityID innate_receiver = kInvalidEntityID;
    for (const auto& event : events_ability_activated)
    {
        if (event.sender_id == red_entity->GetID())
        {
            ASSERT_EQ(event.predicted_receiver_ids.size(), 1);
            if (event.ability_type == AbilityType::kInnate)
            {
                innate_receiver = event.predicted_receiver_ids.front();
                break;
            }
        }
    }

    EXPECT_EQ(innate_receiver, red_entity->GetID());

    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnMissStart, ApplyEffectWithOnMissInnateAbility)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // 1. activation of just attacks
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // Make blue always miss
    blue_template_stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);

    // 2. activation of attacks with misses for blue
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for both entities + 1 innate(dodge)";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should have only fired once for both entities + 1 innate(dodge)";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // Make red always miss
    red_template_stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);

    // 3. activation of attacks with misses for red
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should have fired innate + attack for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();

    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should have fired innate + attack for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnAbilityActivated, ApplyEffectWithOnAttackInnateAbility)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // 1. activation
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 8) << "Should have fired attack + innate for both";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(4).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(5).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(6).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(7).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    red_template_stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);

    // 2. activation - Should still activate innate, despite miss
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should have fired attack + innate for both";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    blue_template_stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);

    // 3. activations both miss, but now shouldn't activate as max_activations is 3
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should have only fired twice for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnHitWithTimeLimit, ApplyEffectWithOnHitInnateAbilityWithTimeLimit)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // 1. activation
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should fire attack and innate for both entities(4)";
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 2);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 2);
    events_ability_activated.clear();

    // Call function with minimum hit chance
    blue_template_stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);
    red_template_stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);

    // 2. activation no hits
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 2);
    events_ability_activated.clear();

    // Call function with maximum hit chance
    blue_template_stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    red_template_stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);

    // 2. activation  hits
    TimeStepForMs(400);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should fire attack and innate for both entities(4)";
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 2);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 2);
    events_ability_activated.clear();

    // 4. Activation.  innates should NOT fired as time limit is exceeded
    TimeStepForMs(400);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should fired attack and innate for both entities(4)";
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 4);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_OnAllyHitByEnemy, TriggerInnateWhenAllyHitByEnemy)
{
    TimeStepForMs(600);
    ASSERT_EQ(events_ability_activated.size(), 9) << "Should fired attack and innate for both entities";
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 6);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 3);
}

TEST_F(AbilitySystemInnateTest_OnEnemyHitBySelf, TriggerInnateWhenEnemyHitBySelf)
{
    TimeStepForMs(600);
    ASSERT_EQ(events_ability_activated.size(), 9) << "Should fired attack and innate for both entities";
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 6);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entity->GetID()), 3);
}

TEST_F(AbilitySystemInnateTest_OnSelfHitByEnemy, TriggerInnateWhenSelfHitByEnemy)
{
    TimeStepForMs(600);
    ASSERT_EQ(events_ability_activated.size(), 12) << "Should fired attack and innate for both entities";
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 6);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 6);
}

TEST_F(AbilitySystemInnateTest_WithHealthInRange, ApplyEffectWithHealthInRange)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set healths to 45 to emit kHealthInRange ability
    blue_stats_component.SetCurrentHealth(45_fp);
    red_stats_component.SetCurrentHealth(45_fp);

    // 1. activation
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 3) << "Should fired attack and innate";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithEveryXAttack, ApplyEffectWithEveryXAttack)
{
    const AbilitiesComponent& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    const AbilitiesComponent& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    for (size_t i = 1; i != 6; ++i)
    {
        TimeStepForMs(400);

        // One innate ability for each two attacks
        ASSERT_EQ(events_ability_activated.size(), 6);

        EXPECT_EQ(blue_abilities_component.GetTotalActivatedAbilitiesCount(AbilityType::kAttack), i * 2);
        EXPECT_EQ(blue_abilities_component.GetTotalActivatedAbilitiesCount(AbilityType::kInnate), i);
        EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack, blue_entity->GetID()), 2);
        EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entity->GetID()), 1);

        EXPECT_EQ(red_abilities_component.GetTotalActivatedAbilitiesCount(AbilityType::kAttack), i * 2);
        EXPECT_EQ(red_abilities_component.GetTotalActivatedAbilitiesCount(AbilityType::kInnate), i);
        EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack, red_entity->GetID()), 2);
        EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, red_entity->GetID()), 1);
        events_ability_activated.clear();
    }
}

TEST_F(AbilitySystemInnateTest_WithInRange, ApplyEffectWithInRange)
{
    // Validate that red_entity and blue_entity is poisoned
    ASSERT_TRUE(EntityHelper::IsPoisoned(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity));

    // Activation
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 6) << "Should activate attack and innate for both entities";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(4).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(5).ability_type, AbilityType::kInnate);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnBattleStart_Blink, BlinkOnStart)
{
    // 1. Should only activate innate
    TimeStepForMs(GetInnateTotalDurationMs());
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should fired attack";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_skill_deployed.size(), 2);
    EXPECT_EQ(events_blinked.size(), 2);

    events_blinked.clear();
    events_skill_deployed.clear();
    events_ability_activated.clear();

    // 2. Should start activating attack abilities
    TimeStepForMs(400);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should fired attack and innate";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnBattleStart_Blink_And_NegativeState, BlinkAndNegativeStateOnStart)
{
    // 1. Should only activate innate
    // Only the first skills should have activated
    TimeStepForMs(500);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should fired attack";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kInnate);

    // Check first skill fired
    ASSERT_EQ(events_skill_deployed.size(), 2);
    EXPECT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_skill_deployed.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_skill_deployed.at(0).skill_index, 0);
    EXPECT_EQ(events_skill_deployed.at(1).skill_index, 0);

    // Check blink fired
    EXPECT_EQ(events_blinked.size(), 2);
    ASSERT_EQ(events_effect_applied.size(), 2);
    EXPECT_EQ(events_effect_applied.at(0).data.type_id.type, EffectType::kBlink);
    EXPECT_EQ(events_effect_applied.at(1).data.type_id.type, EffectType::kBlink);

    events_blinked.clear();
    events_skill_deployed.clear();
    events_ability_activated.clear();
    events_effect_applied.clear();

    // Activate the second skill of the innates
    TimeStepForMs(500);

    // Not abilities should have activated
    EXPECT_EQ(events_ability_activated.size(), 0);

    // Check first skill fired
    ASSERT_EQ(events_skill_deployed.size(), 2);
    EXPECT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_skill_deployed.at(1).ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_skill_deployed.at(0).skill_index, 1);
    EXPECT_EQ(events_skill_deployed.at(1).skill_index, 1);

    // Check effects
    EXPECT_EQ(events_blinked.size(), 0);
    ASSERT_EQ(events_effect_applied.size(), 2);
    EXPECT_EQ(events_effect_applied.at(0).data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_effect_applied.at(1).data.type_id.type, EffectType::kNegativeState);

    // 2. Should start activating attack abilities
    TimeStepForMs(400);
    ASSERT_EQ(events_ability_activated.size(), 4) << "Should fired attack and innate";
    EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(2).ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated.at(3).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnEnergyFull, OnEnergyFull)
{
    auto& blue_stats = blue_entity->Get<StatsComponent>();
    blue_stats.GetMutableTemplateStats().Set(StatType::kEnergyCost, 5_fp);

    EventHistory<EventType::kOnEffectApplied> effects(*world);

    // both blue and red attack applying effects with physical attack
    EXPECT_EQ(blue_stats.GetCurrentEnergy(), 0_fp);
    world->TimeStep();
    ASSERT_EQ(effects.Size(), 2);
    EXPECT_EQ(effects[0].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(effects[0].data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(effects[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(effects[1].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(effects[1].data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(effects[1].sender_id, red_entity->GetID());
    effects.events.clear();

    world->TimeStep();
    world->TimeStep();
    ASSERT_EQ(effects.Size(), 3);

    // Blue entity activates innate
    EXPECT_EQ(effects[0].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(effects[0].data.type_id.damage_type, EffectDamageType::kPure);
    EXPECT_EQ(effects[0].sender_id, blue_entity->GetID());

    // Blue entity also attacks
    EXPECT_EQ(effects[1].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(effects[1].data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(effects[1].sender_id, blue_entity->GetID());

    // Red entity just attacks
    EXPECT_EQ(effects[2].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(effects[2].data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(effects[2].sender_id, red_entity->GetID());
}

TEST_F(AbilitySystemInnateTest_WithOnEveryXEffectPackage, OnEveryXEffectPackage)
{
    TimeStepForMs(600);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 6);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entity->GetID()), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, red_entity->GetID()), 0);
    events_ability_activated.clear();

    TimeStepForMs(1000);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 10);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entity->GetID()), 1);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, red_entity->GetID()), 0);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemInnateTest_WithOnEveryXEffectPackage_Zone, OnEveryXEffectPackage_Zone)
{
    EventHistory<EventType::kZoneCreated> zone_created(*world);
    EventHistory<EventType::kZoneDestroyed> zone_destroyed(*world);
    EventHistory<EventType::kTryToApplyEffect> apply_effect(*world);

    // 1. Both red and client attack and spawn zone entities
    TimeStepForMs(100);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 2);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 0);
    ASSERT_EQ(zone_created.Size(), 2);
    EXPECT_EQ(zone_destroyed.Size(), 0);
    EXPECT_EQ(apply_effect.Size(), 0);

    const EntityID zone_a = zone_created.events[0].entity_id;
    const EntityID zone_b = zone_created.events[1].entity_id;

    zone_created.events.clear();
    events_ability_activated.clear();

    auto test_step = [&](bool with_innate)
    {
        TimeStepForMs(100);

        ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack, blue_entity->GetID()), 0);
        ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack, red_entity->GetID()), 0);
        ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack, zone_a), 1);
        ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack, zone_b), 1);
        ASSERT_EQ(zone_created.Size(), 0);
        if (with_innate)
        {
            ASSERT_EQ(apply_effect.Size(), 3);
            ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entity->GetID()), 1);
            ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, red_entity->GetID()), 0);
            ASSERT_EQ(apply_effect.events[0].data.type_id.type, EffectType::kInstantDamage);
            ASSERT_EQ(apply_effect.events[0].data.type_id.damage_type, EffectDamageType::kPhysical);
            ASSERT_EQ(apply_effect.events[1].data.type_id.type, EffectType::kInstantDamage);
            ASSERT_EQ(apply_effect.events[1].data.type_id.damage_type, EffectDamageType::kPure);
            ASSERT_EQ(apply_effect.events[2].data.type_id.type, EffectType::kInstantDamage);
            ASSERT_EQ(apply_effect.events[2].data.type_id.damage_type, EffectDamageType::kPhysical);
        }
        else
        {
            ASSERT_EQ(apply_effect.Size(), 2);
            ASSERT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 0);
            ASSERT_EQ(apply_effect.events[0].data.type_id.type, EffectType::kInstantDamage);
            ASSERT_EQ(apply_effect.events[0].data.type_id.damage_type, EffectDamageType::kPhysical);
            ASSERT_EQ(apply_effect.events[1].data.type_id.type, EffectType::kInstantDamage);
            ASSERT_EQ(apply_effect.events[1].data.type_id.damage_type, EffectDamageType::kPhysical);
        }
        apply_effect.events.clear();
        events_ability_activated.clear();
    };

    // 2. Zone entities spawned on the previous time step
    // Now innate should be activated on each second received effect package
    test_step(false);
    test_step(true);
    test_step(false);
    test_step(true);
}

TEST_F(AbilitySystemInnateTest_WithOnEveryXEffectPackage_Dot, OnEveryXEffectPackage_Dot)
{
    EventHistory<EventType::kTryToApplyEffect> apply_effect(*world);

    const auto from_attacks = MakeEnumSet(AbilityType::kAttack);

    EXPECT_EQ(blue_entity->Get<AbilitiesComponent>().GetReceivedEffectPackagesCount(from_attacks), 0);
    EXPECT_EQ(red_entity->Get<AbilitiesComponent>().GetReceivedEffectPackagesCount(from_attacks), 0);

    // Both red and client attack and apply damage over time effects
    TimeStepForMs(100);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack), 2);
    EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate), 0);
    ASSERT_EQ(apply_effect.Size(), 2);
    EXPECT_EQ(apply_effect.events[0].data.type_id.type, EffectType::kDamageOverTime);
    EXPECT_EQ(apply_effect.events[0].data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(apply_effect.events[1].data.type_id.type, EffectType::kDamageOverTime);
    EXPECT_EQ(apply_effect.events[1].data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(blue_entity->Get<AbilitiesComponent>().GetReceivedEffectPackagesCount(from_attacks), 1);
    EXPECT_EQ(red_entity->Get<AbilitiesComponent>().GetReceivedEffectPackagesCount(from_attacks), 1);
    events_ability_activated.clear();
    apply_effect.events.clear();

    for (size_t i = 0; i != 10; ++i)
    {
        TimeStepForMs(100);
        EXPECT_EQ(blue_entity->Get<AbilitiesComponent>().GetReceivedEffectPackagesCount(from_attacks), 1);
        EXPECT_EQ(red_entity->Get<AbilitiesComponent>().GetReceivedEffectPackagesCount(from_attacks), 1);
    }
}

TEST_F(AbilitySystemInnateTest_WithOnDeployXSkills, OnDeploy5Skills)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();
    blue_template_stats.Set(StatType::kMaxHealth, 10000_fp).Set(StatType::kCurrentHealth, 10000_fp);
    red_template_stats.Set(StatType::kMaxHealth, 10000_fp).Set(StatType::kCurrentHealth, 10000_fp);

    // Attack each time step.
    blue_template_stats.Set(StatType::kAttackSpeed, 1000_fp);
    red_template_stats.Set(StatType::kAttackSpeed, 1000_fp);

    // kOnDeployXSkills trigger works on kSkillFinished event so it will be triggerred
    // on the next time step (when attack skill deactivates).
    for (int i = 1; i != 27; ++i)
    {
        world->TimeStep();
        const int expected_innates = (world->GetTimeStepCount() - 1) / GetInnatePeriod();
        EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack, blue_entity->GetID()), i)
            << " after " << i << " time steps";
        EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kAttack, red_entity->GetID()), i)
            << " after " << i << " time steps";
        EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, blue_entity->GetID()), expected_innates)
            << " after " << i << " time steps";
        EXPECT_EQ(GetActivatedAbilitiesCount(AbilityType::kInnate, red_entity->GetID()), 0)
            << " after " << i << " time steps";
    }
}

}  // namespace simulation
