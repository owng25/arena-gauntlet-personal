#include "base_test_fixtures.h"
#include "components/stats_component.h"
#include "ecs/world.h"
#include "gtest/gtest.h"
#include "systems/ability_system.h"
#include "utility/hyper_helper.h"

namespace simulation
{
class OverloadSystemTest : public BaseTest
{
    using Super = BaseTest;

protected:
    void InitCombatUnitData()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 5_fp);
        data.type_data.stats.Set(StatType::kAttackEnergyDamage, 10_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kCurrentHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        data.type_data.stats.Set(StatType::kHealthRegeneration, 0_fp);

        // Stop Movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    }

    OverloadConfig GetOverloadConfig() const override
    {
        OverloadConfig config;
        config.start_seconds_apply_overload_damage = 4;
        config.enable_overload_system = true;
        return config;
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
    }

    // Combat unit data used
    CombatUnitData data;
};

TEST_F(OverloadSystemTest, SameCombatUnitsLastOrder)
{
    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    EventHistory<EventType::kBattleFinished> events_battle_finished(*world);
    EventHistory<EventType::kOverloadStarted> events_overload_started(*world);

    ASSERT_EQ(world->GetOverloadConfig().enable_overload_system, true);

    // Add Entities
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    // Both combat untis die but red is added later
    while (events_battle_finished.Size() == 0)
    {
        world->TimeStep();
        if (events_overload_started.Size() != 0)
        {
            const int seconds_since_start = events_overload_started[0].second_since_start;
            const int current_seconds = Time::TimeStepsToSeconds(world->GetTimeStepCount());
            const int seconds_to_apply =
                world->GetOverloadConfig().start_seconds_apply_overload_damage + seconds_since_start;

            ASSERT_EQ(current_seconds, seconds_to_apply);
            events_overload_started.Clear();
        }
    }

    ASSERT_GT(events_battle_finished.Size(), 0);
    EXPECT_EQ(events_battle_finished[0].winning_team, Team::kRed);
    EXPECT_TRUE(red_entity->IsActive());
    EXPECT_FALSE(blue_entity->IsActive());
}

TEST_F(OverloadSystemTest, SameCombatUnitsLeastNegativeHealth)
{
    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    EventHistory<EventType::kBattleFinished> events_battle_finished(*world);
    EventHistory<EventType::kOverloadStarted> events_overload_started(*world);

    ASSERT_EQ(world->GetOverloadConfig().enable_overload_system, true);

    // Add Entities
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    blue_stats_component.SetCurrentHealth(30_fp);
    red_stats_component.SetCurrentHealth(100_fp);

    // Both combat untis die but one has more health so it will win
    while (events_battle_finished.Size() == 0)
    {
        world->TimeStep();
        if (events_overload_started.Size() != 0)
        {
            const int seconds_since_start = events_overload_started[0].second_since_start;
            const int current_seconds = Time::TimeStepsToSeconds(world->GetTimeStepCount());
            const int seconds_to_apply =
                world->GetOverloadConfig().start_seconds_apply_overload_damage + seconds_since_start;

            ASSERT_EQ(current_seconds, seconds_to_apply);
            events_overload_started.Clear();
        }
    }

    ASSERT_GT(events_battle_finished.Size(), 0);
    EXPECT_EQ(events_battle_finished[0].winning_team, Team::kRed);
    EXPECT_TRUE(red_entity->IsActive());
    EXPECT_FALSE(blue_entity->IsActive());
}

TEST_F(OverloadSystemTest, RedCombatUnitDies)
{
    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    EventHistory<EventType::kBattleFinished> events_battle_finished(*world);
    EventHistory<EventType::kOverloadStarted> events_overload_started(*world);

    ASSERT_EQ(world->GetOverloadConfig().enable_overload_system, true);

    // Add Entities
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    auto& red_stats_component = red_entity->Get<StatsComponent>();
    red_stats_component.SetCurrentHealth(30_fp);

    // Red combat unit dies earlier than blue so blue wins
    while (events_battle_finished.Size() == 0)
    {
        world->TimeStep();
        if (events_overload_started.Size() != 0)
        {
            const int seconds_since_start = events_overload_started[0].second_since_start;
            const int current_seconds = Time::TimeStepsToSeconds(world->GetTimeStepCount());
            const int seconds_to_apply =
                world->GetOverloadConfig().start_seconds_apply_overload_damage + seconds_since_start;

            ASSERT_EQ(current_seconds, seconds_to_apply);
            events_overload_started.Clear();
        }
    }

    ASSERT_GT(events_battle_finished.Size(), 0);
    EXPECT_EQ(events_battle_finished[0].winning_team, Team::kBlue);
    EXPECT_FALSE(red_entity->IsActive());
    EXPECT_TRUE(blue_entity->IsActive());
}

TEST_F(OverloadSystemTest, SpreadDamageAcrossTeam)
{
    ASSERT_EQ(world->GetOverloadConfig().enable_overload_system, true);

    // Add Entities

    std::vector<Entity*> blue_entities;
    blue_entities.resize(1);
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entities[0]);

    std::vector<Entity*> red_entities;
    red_entities.resize(2);
    SpawnCombatUnit(Team::kRed, {-72, 37}, data, red_entities[0]);
    SpawnCombatUnit(Team::kRed, {-66, 37}, data, red_entities[1]);

    EventHistory<EventType::kOnDamage> damage_events(*world);
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kOverloadStarted>());

    auto get_damage_received_by_entity = [&](const Entity* entity)
    {
        FixedPoint damage = 0_fp;
        for (auto& event : damage_events.events)
        {
            if (event.receiver_id == entity->GetID())
            {
                damage += event.damage_value;
            }
        }
        return damage;
    };

    ASSERT_EQ(damage_events.Size(), 3);
    const FixedPoint damage_to_blue = get_damage_received_by_entity(blue_entities[0]);
    const FixedPoint damage_to_red = damage_to_blue / 2_fp;
    ASSERT_EQ(get_damage_received_by_entity(red_entities[0]), damage_to_red);
    ASSERT_EQ(get_damage_received_by_entity(red_entities[1]), damage_to_red);
}

TEST_F(OverloadSystemTest, TieBreakerPercentage)
{
    ASSERT_EQ(world->GetOverloadConfig().enable_overload_system, true);

    // Add Entities

    std::vector<Entity*> blue_entities;
    blue_entities.resize(1);
    data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);  // first overload tick - 250 damage
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entities[0]);
    blue_entities[0]->Get<StatsComponent>().SetCurrentHealth(40_fp);  // -210 (-2.1%)

    std::vector<Entity*> red_entities;
    red_entities.resize(1);
    data.type_data.stats.Set(StatType::kMaxHealth, 5000_fp);    // first overload tick - 125 damage
    data.type_data.stats.Set(StatType::kCurrentHealth, 10_fp);  // -15
    SpawnCombatUnit(Team::kRed, {-72, 37}, data, red_entities[0]);
    red_entities[0]->Get<StatsComponent>().SetCurrentHealth(10_fp);  // -115 (-2.3%)

    EventHistory<EventType::kBattleFinished> battle_finished_events(*world);
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kOverloadStarted>());

    // If we choose by absolute value this test would fail, since
    // blue entity has bigger overflow than red entity.
    ASSERT_GT(battle_finished_events.Size(), 0);
    ASSERT_EQ(battle_finished_events[0].winning_team, Team::kBlue);
}

}  // namespace simulation
