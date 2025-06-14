#include "ability_system_data_fixtures.h"
#include "components/position_component.h"
#include "data/enums.h"
#include "gtest/gtest.h"
#include "systems/effect_system.h"
#include "test_data_loader.h"

namespace simulation
{
class DisplacementTestWithStartAttackEveryTimeStep : public BaseTest
{
protected:
    static CombatUnitData MakeUnitDataWithDefaultStats()
    {
        CombatUnitData data = CreateCombatUnitData();

        // Stats
        data.radius_units = 5;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);
        data.type_data.stats.Set(StatType::kEnergyResist, 0_fp);
        data.type_data.stats.Set(StatType::kPhysicalResist, 0_fp);
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        // No critical attacks
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
        // No Dodge
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        // No Miss
        data.type_data.stats.Set(StatType::kHitChancePercentage, 0_fp);

        // Once every second
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);

        data.type_data.stats.Set(StatType::kResolve, 0_fp);
        data.type_data.stats.Set(StatType::kGrit, 0_fp);
        data.type_data.stats.Set(StatType::kWillpowerPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kTenacityPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 5_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 10000_fp);

        return data;
    }
};

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementKnockUp)
{
    const auto data = MakeUnitDataWithDefaultStats();

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-12, -25}, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {13, 25}, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kDisplacementStarted> displacement_started(*world);
    EventHistory<EventType::kDisplacementStopped> displacement_stopped(*world);
    EventHistory<EventType::kAbilityActivated> events_ability_activated(*world);

    // Clear event trackers
    displacement_started.Clear();
    displacement_stopped.Clear();

    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();

    // Ensure initial position is correct
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(-12, -25));
    ASSERT_EQ(red_position_component.GetPosition(), HexGridPosition(13, 25));

    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Knock up for 3 time steps
    const auto effect = EffectData::CreateDisplacement(EffectDisplacementType::kKnockUp, 300);

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            effect,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            effect,
            effect_state);
    }

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    for (int i = 0; i < 3; i++)
    {
        events_ability_activated.Clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire
        ASSERT_EQ(events_ability_activated.Size(), 0) << "i = " << i;

        // Should not be able to activate attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    }

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again, knock up ends
    world->TimeStep();

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 2);

    ASSERT_EQ(events_ability_activated.Size(), 0);
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Ensure entities are still in initial position
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(-12, -25));
    ASSERT_EQ(red_position_component.GetPosition(), HexGridPosition(13, 25));

    // Should have fired
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // Ensure entities started moving
    ASSERT_NE(blue_position_component.GetPosition(), HexGridPosition(-12, -25));
    ASSERT_NE(red_position_component.GetPosition(), HexGridPosition(13, 25));
}

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementKnockBack)
{
    const auto data = MakeUnitDataWithDefaultStats();

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-12, -25}, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {13, 25}, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kDisplacementStarted> displacement_started(*world);
    EventHistory<EventType::kDisplacementStopped> displacement_stopped(*world);
    EventHistory<EventType::kAbilityActivated> events_ability_activated(*world);

    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();

    // Clear event trackers
    displacement_started.Clear();
    displacement_stopped.Clear();

    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Knock back over 3 time steps for a distance of 50000 subunits
    auto effect = EffectData::CreateDisplacement(EffectDisplacementType::kKnockBack, 300);
    effect.displacement_distance_sub_units = 50000;

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            effect,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            effect,
            effect_state);
    }

    // Ensure initial position is correct
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(-12, -25));
    ASSERT_EQ(red_position_component.GetPosition(), HexGridPosition(13, 25));

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    for (int i = 0; i < 3; i++)
    {
        events_ability_activated.Clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire
        ASSERT_EQ(events_ability_activated.Size(), 0) << "i = " << i;

        // Should not be able to activate attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    }

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again, knock back ends
    world->TimeStep();

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 2);

    ASSERT_EQ(events_ability_activated.Size(), 0);
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Ensure positions are correct from displacement
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(-28, -58));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(29, 58));

    // Can activate abilities again
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    ASSERT_EQ(events_ability_activated.Size(), 0);
    for (const auto& event_data : events_ability_activated.events)
    {
        ASSERT_EQ(event_data.ability_type, AbilityType::kAttack);
    }

    // Ensure final positions are correct - entities are moving again
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(-25, -51));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(26, 51));
}

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementKnockBackBlocked)
{
    const auto data = MakeUnitDataWithDefaultStats();

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-12, -25}, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {13, 25}, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Knock back over 6 time steps for a distance of 100000 subunits
    auto effect = EffectData::CreateDisplacement(EffectDisplacementType::kKnockBack, 600);
    effect.displacement_distance_sub_units = 100000;

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            effect,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            effect,
            effect_state);
    }

    // Some obstacles
    Entity* blue_dummy1 = nullptr;
    Entity* red_dummy1 = nullptr;
    SpawnCombatUnit(Team::kBlue, {25, 50}, data, blue_dummy1);
    SpawnCombatUnit(Team::kRed, {-25, -50}, data, red_dummy1);

    // Ensure initial position is correct
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(-12, -25));
    ASSERT_EQ(red_position_component.GetPosition(), HexGridPosition(13, 25));

    for (int i = 0; i < 6; i++)
    {
        // TimeStep the world (6 time steps)
        world->TimeStep();
    }

    // TimeStep again, knock back ends
    world->TimeStep();

    // Ensure positions are correct from displacement
    // Paths cut short due to obstacles
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(-36, -73));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(37, 73));
}

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementPull)
{
    auto data = MakeUnitDataWithDefaultStats();

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-12, -25}, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    auto& skill = data.type_data.attack_abilities.AddAbility().AddSkill();
    skill.targeting.type = SkillTargetingType::kCurrentFocus;
    skill.deployment.type = SkillDeploymentType::kDirect;
    skill.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {13, 25}, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kDisplacementStarted> displacement_started(*world);
    EventHistory<EventType::kDisplacementStopped> displacement_stopped(*world);
    EventHistory<EventType::kAbilityActivated> events_ability_activated(*world);

    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();

    // Clear event trackers
    displacement_started.Clear();
    displacement_stopped.Clear();

    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Knock back over 3 time steps for a distance of 50000 subunits
    auto effect = EffectData::CreateDisplacement(EffectDisplacementType::kPull, 300);
    effect.displacement_distance_sub_units = 50000;

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            effect,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            effect,
            effect_state);
    }

    // Ensure initial position is correct
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(-12, -25));
    ASSERT_EQ(red_position_component.GetPosition(), HexGridPosition(13, 25));

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    for (int i = 0; i < 3; i++)
    {
        events_ability_activated.Clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire
        ASSERT_EQ(events_ability_activated.Size(), 0) << "i = " << i;

        // Should not be able to activate attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    }

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again, pull ends
    world->TimeStep();

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 2);

    ASSERT_EQ(events_ability_activated.Size(), 0);
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // Ensure positions are correct from displacement
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(2, 3));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(7, 14));

    // Can activate abilities again, should have fired
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // Ensure blue entity  should move
    EXPECT_NE(blue_position_component.GetPosition(), HexGridPosition(-4, -8));

    // Red in range after blue move, so it's doesn't move and attacked
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(7, 14));
    ASSERT_EQ(events_ability_activated.Size(), 1);
    ASSERT_EQ(events_ability_activated[0].sender_id, red_entity->GetID());
}

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementImmune)
{
    const auto data = MakeUnitDataWithDefaultStats();

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-12, -25}, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {13, 25}, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kDisplacementStarted> displacement_started(*world);
    EventHistory<EventType::kDisplacementStopped> displacement_stopped(*world);

    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Immune effect
    const auto effect1 = EffectData::CreatePositiveState(EffectPositiveState::kImmune, kTimeInfinite);

    // Knock up for 3 time steps
    const auto effect2 = EffectData::CreateDisplacement(EffectDisplacementType::kKnockUp, 300);

    // Blue immune, red attack
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effects
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            blue_entity->GetID(),
            effect1,
            effect_state);
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            effect1,
            effect_state);
    }

    // Red immune, blue attack
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effects
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            red_entity->GetID(),
            effect1,
            effect_state);
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            effect2,
            effect_state);
    }

    ASSERT_EQ(displacement_started.Size(), 0);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    // Confirm entities have not been affected
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementApplyEffects)
{
    const auto data = MakeUnitDataWithDefaultStats();

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-12, -25}, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {13, 25}, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kDisplacementStarted> displacement_started(*world);
    EventHistory<EventType::kDisplacementStopped> displacement_stopped(*world);

    // Red entity stats
    const auto& red_stats_component = red_entity->Get<StatsComponent>();
    const auto red_initial_health = red_stats_component.GetCurrentHealth();

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Knock up for 3 time steps
    const auto effect = EffectData::CreateDisplacement(EffectDisplacementType::kKnockUp, 300);

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            effect,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            effect,
            effect_state);
    }

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 time steps)
        world->TimeStep();
    }

    // Entity to use as source for effects
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {25, 50}, data, blue_entity2);

    // Apply some effects before the knock up ends
    const auto negative_effect = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 600);
    const auto positive_effect = EffectData::CreatePositiveState(EffectPositiveState::kTruesight, 600);
    const auto damage_effect = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity2->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply negative effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity2->GetID(),
            red_entity->GetID(),
            negative_effect,
            effect_state);

        // Apply damage effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity2->GetID(),
            red_entity->GetID(),
            damage_effect,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity2->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply positive effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity2->GetID(),
            blue_entity->GetID(),
            positive_effect,
            effect_state);
    }

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    // TimeStep again, knock up ends
    world->TimeStep();

    ASSERT_EQ(displacement_started.Size(), 2);
    ASSERT_EQ(displacement_stopped.Size(), 2);

    // Verify effects
    EXPECT_TRUE(EntityHelper::IsTaunted(*red_entity));
    EXPECT_TRUE(EntityHelper::HasTruesight(*blue_entity));
    EXPECT_LT(red_stats_component.GetCurrentHealth(), red_initial_health);
}

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementKnockUpThenKnockBack)
{
    const auto data = MakeUnitDataWithDefaultStats();

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-12, -25}, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {13, 25}, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kDisplacementStarted> displacement_started(*world);
    EventHistory<EventType::kDisplacementStopped> displacement_stopped(*world);
    EventHistory<EventType::kAbilityActivated> events_ability_activated(*world);

    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();

    // Clear event trackers
    displacement_started.Clear();
    displacement_stopped.Clear();

    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Knock up for 3 time steps
    const auto effect1 = EffectData::CreateDisplacement(EffectDisplacementType::kKnockUp, 300);

    // Knock back over 3 time steps for a distance of 50000 subunits
    auto effect2 = EffectData::CreateDisplacement(EffectDisplacementType::kKnockBack, 300);
    effect2.displacement_distance_sub_units = 25000;

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effects
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            effect1,
            effect_state);
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            effect2,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            effect1,
            effect_state);
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            effect2,
            effect_state);
    }

    // Ensure initial position is correct
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(-12, -25));
    ASSERT_EQ(red_position_component.GetPosition(), HexGridPosition(13, 25));

    ASSERT_EQ(displacement_started.Size(), 4);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    for (int i = 0; i < 3; i++)
    {
        events_ability_activated.Clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire
        ASSERT_EQ(events_ability_activated.Size(), 0) << "i = " << i;

        // Should not be able to activate attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    }

    ASSERT_EQ(displacement_started.Size(), 4);
    ASSERT_EQ(displacement_stopped.Size(), 0);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again, knock back ends
    world->TimeStep();

    ASSERT_EQ(displacement_started.Size(), 4);
    ASSERT_EQ(displacement_stopped.Size(), 4);

    ASSERT_EQ(events_ability_activated.Size(), 0);
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Ensure positions are correct from displacement
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(-20, -41));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(21, 41));

    // Can activate abilities again
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    ASSERT_EQ(events_ability_activated.Size(), 0);
    for (const auto& event_data : events_ability_activated.events)
    {
        ASSERT_EQ(event_data.ability_type, AbilityType::kAttack);
    }

    // Ensure final positions are correct - entities are moving again
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(-17, -34));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(18, 34));
}

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementThrow)
{
    const auto data = MakeUnitDataWithDefaultStats();

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-12, -25}, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {13, 25}, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    // Some obstacles
    Entity* blue_dummy1 = nullptr;
    Entity* red_dummy1 = nullptr;
    Entity* red_dummy2 = nullptr;
    CombatUnitData dummy_data = data;
    dummy_data.type_id.line_name = "dummy";
    dummy_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    SpawnCombatUnit(Team::kBlue, {40, -68}, dummy_data, blue_dummy1);
    SpawnCombatUnit(Team::kRed, {0, 70}, dummy_data, red_dummy1);
    SpawnCombatUnit(Team::kRed, {5, 47}, dummy_data, red_dummy2);

    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Throw over 6 time steps
    TestingDataLoader loader(world->GetLogger());
    const auto effect = loader.ParseAndLoadEffect(R"({
        "Type": "Displacement",
        "DisplacementType": "ThrowToFurthestEnemy",
        "DurationMs": 600
    })");

    ASSERT_TRUE(effect.has_value());
    ASSERT_EQ(effect->type_id.type, EffectType::kDisplacement);
    ASSERT_EQ(effect->type_id.displacement_type, EffectDisplacementType::kThrowToFurthestEnemy);
    ASSERT_EQ(effect->lifetime.duration_time_ms, 600);

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity->GetID(),
            blue_entity->GetID(),
            *effect,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entity->GetID(),
            red_entity->GetID(),
            *effect,
            effect_state);
    }

    // Ensure initial position is correct
    ASSERT_EQ(blue_position_component.GetPosition(), HexGridPosition(-12, -25));
    ASSERT_EQ(red_position_component.GetPosition(), HexGridPosition(13, 25));

    ASSERT_EQ(blue_position_component.GetReservedPosition(), HexGridPosition(29, -57));
    ASSERT_EQ(red_position_component.GetReservedPosition(), HexGridPosition(0, 59));

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    ASSERT_TRUE(blue_position_component.IsOverlapable());
    ASSERT_TRUE(red_position_component.IsOverlapable());

    TimeStepForNTimeSteps(6);

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // Reached target
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(29, -57));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(0, 59));

    // After this step entities are movable, but not moved yet
    world->TimeStep();

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    ASSERT_FALSE(blue_position_component.IsOverlapable());
    ASSERT_FALSE(red_position_component.IsOverlapable());

    // Entities should move somewhere
    world->TimeStep();
    EXPECT_NE(blue_position_component.GetPosition(), HexGridPosition(29, -57));
    EXPECT_NE(red_position_component.GetPosition(), HexGridPosition(0, 59));
}

TEST_F(DisplacementTestWithStartAttackEveryTimeStep, DisplacementThrowToDensity)
{
    auto data = MakeUnitDataWithDefaultStats();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

    constexpr std::array<HexGridPosition, 7> blue_positions{{
        {0, -2},
        {-43, -14},
        {-42, -17},
        {-39, -19},
        {-37, -18},
        {-41, -13},
        {-38, -15},
    }};
    std::array<Entity*, blue_positions.size()> blue_entities{};
    for (size_t index = 0; index != blue_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kBlue, blue_positions[index], data, blue_entities[index]);
        ASSERT_NE(blue_entities[index], nullptr);
    }

    data.radius_units = 5;
    constexpr std::array<HexGridPosition, 7> red_positions{{
        {-4, 6},
        {-6, 53},
        {-1, 42},
        {10, 36},
        {16, 41},
        {11, 52},
        {0, 58},
    }};
    std::array<Entity*, red_positions.size()> red_entities{};
    for (size_t index = 0; index != red_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kRed, red_positions[index], data, red_entities[index]);
        ASSERT_NE(red_entities[index], nullptr);
    }

    const auto& blue_position_component = blue_entities[0]->Get<PositionComponent>();
    const auto& red_position_component = red_entities[0]->Get<PositionComponent>();

    // Access effect system
    auto effect_system = world->GetSystem<EffectSystem>();

    // Throw over 6 time steps
    TestingDataLoader loader(world->GetLogger());
    const auto effect = loader.ParseAndLoadEffect(R"({
        "Type": "Displacement",
        "DisplacementType": "ThrowToHighestEnemyDensity",
        "DurationMs": 600
    })");

    ASSERT_TRUE(effect.has_value());
    ASSERT_EQ(effect->type_id.type, EffectType::kDisplacement);
    ASSERT_EQ(effect->type_id.displacement_type, EffectDisplacementType::kThrowToHighestEnemyDensity);
    ASSERT_EQ(effect->lifetime.duration_time_ms, 600);

    // Red
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(red_entities[0]->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entities[0]->GetID(),
            blue_entities[0]->GetID(),
            *effect,
            effect_state);
    }

    // Blue
    {
        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(blue_entities[0]->GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        // Apply effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            blue_entities[0]->GetID(),
            red_entities[0]->GetID(),
            *effect,
            effect_state);
    }

    constexpr HexGridPosition blue_reserved_pos{-40, -16};
    constexpr HexGridPosition red_reserved_pos{5, 47};

    ASSERT_EQ(blue_position_component.GetReservedPosition(), blue_reserved_pos);
    ASSERT_EQ(red_position_component.GetReservedPosition(), red_reserved_pos);

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entities[0]));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entities[0]));

    ASSERT_TRUE(blue_position_component.IsOverlapable());
    ASSERT_TRUE(red_position_component.IsOverlapable());

    TimeStepForNTimeSteps(6);

    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entities[0]));
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entities[0]));

    // Reached target
    ASSERT_EQ(blue_position_component.GetReservedPosition(), blue_reserved_pos);
    ASSERT_EQ(red_position_component.GetReservedPosition(), red_reserved_pos);

    // After this step entities are movable, but not moved yet
    world->TimeStep();

    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entities[0]));
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entities[0]));

    ASSERT_FALSE(blue_position_component.IsOverlapable());
    ASSERT_FALSE(red_position_component.IsOverlapable());

    // Entities should move somewhere
    world->TimeStep();
    ASSERT_NE(blue_position_component.GetReservedPosition(), blue_reserved_pos);
    ASSERT_NE(red_position_component.GetReservedPosition(), red_reserved_pos);
}

}  // namespace simulation
