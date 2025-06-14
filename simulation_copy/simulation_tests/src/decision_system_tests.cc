#include "base_test_fixtures.h"
#include "components/decision_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "gtest/gtest.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
class DecisionSystemTest : public BaseTest
{
};

TEST_F(DecisionSystemTest, FindTarget)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();
    blue_entity.Add<DecisionComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetTakingSpace(true);

    // TimeStep the world
    world->TimeStep();

    // Get the AI component
    auto& blue_decision_component = blue_entity.Get<DecisionComponent>();

    // Should be looking for a target
    EXPECT_EQ(blue_decision_component.GetNextAction(), DecisionNextAction::kFindFocus);
}

TEST_F(DecisionSystemTest, FindTargetSelfNotActive)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();
    blue_entity.Add<DecisionComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetTakingSpace(true);

    // Deactivate this component
    blue_entity.Deactivate();

    // TimeStep the world
    world->TimeStep();

    // Get the AI component
    auto& blue_decision_component = blue_entity.Get<DecisionComponent>();

    // Expects to be none as the entity is not active
    EXPECT_EQ(blue_decision_component.GetNextAction(), DecisionNextAction::kNone);
}

TEST_F(DecisionSystemTest, FindTargetTargetNotActive)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();
    blue_entity.Add<DecisionComponent>();
    blue_entity.Add<MovementComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);
    auto& blue_stats_component = blue_entity.Get<StatsComponent>();
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 5_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 100_fp);
    blue_stats_component.SetCurrentEnergy(0_fp);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();
    red_entity.Add<DecisionComponent>();
    red_entity.Add<MovementComponent>();

    // Set position of entity at 20, 20
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(20, 20);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);
    auto& red_stats_component = red_entity.Get<StatsComponent>();
    red_stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 5_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 100_fp);
    red_stats_component.SetCurrentEnergy(0_fp);

    // Get the AI components
    auto& blue_decision_component = blue_entity.Get<DecisionComponent>();
    auto& red_decision_component = red_entity.Get<DecisionComponent>();

    // TimeStep the world
    world->TimeStep();
    EXPECT_EQ(blue_decision_component.GetNextAction(), DecisionNextAction::kMovement);
    EXPECT_EQ(red_decision_component.GetNextAction(), DecisionNextAction::kMovement);

    // Deactivate the red
    red_entity.Deactivate();

    // Should be finding  target now as the red_entity is not active
    world->TimeStep();
    EXPECT_EQ(blue_decision_component.GetNextAction(), DecisionNextAction::kFindFocus);
}

TEST_F(DecisionSystemTest, MoveToTarget)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();
    blue_entity.Add<DecisionComponent>();
    blue_entity.Add<MovementComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Set up range and energy
    auto& blue_stats_component = blue_entity.Get<StatsComponent>();
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 5_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 100_fp);
    blue_stats_component.SetCurrentEnergy(0_fp);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();
    red_entity.Add<DecisionComponent>();
    red_entity.Add<MovementComponent>();

    // Set position of entity at 20, 20
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(20, 20);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // Set up range
    auto& red_stats_component = red_entity.Get<StatsComponent>();
    red_stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 5_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 100_fp);
    red_stats_component.SetCurrentEnergy(0_fp);

    // Get the Decision components
    auto& blue_decision_component = blue_entity.Get<DecisionComponent>();
    auto& red_decision_component = red_entity.Get<DecisionComponent>();

    // TimeStep the world
    world->TimeStep();

    // Should be moving because they are out of range
    EXPECT_EQ(blue_decision_component.GetNextAction(), DecisionNextAction::kMovement);
    EXPECT_EQ(red_decision_component.GetNextAction(), DecisionNextAction::kMovement);
}

TEST_F(DecisionSystemTest, AttackAbility)
{
    // We need to populate the abilities with some data otherwise
    // it does not make sense to use a specific ability if it does not
    // event exist in the first place
    AbilitiesData abilities{};
    {
        auto& ability = abilities.AddAbility();
        ability.total_duration_ms = 1000;
        auto& skill = ability.AddSkill();
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
    }

    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 30_fp);
    data.type_data.stats.Set(StatType::kOmegaRangeUnits, 30_fp);
    data.type_data.attack_abilities = abilities;
    data.type_data.omega_abilities = abilities;

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{10, 10}, data, blue_entity);

    // Add red entities
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{20, 20}, data, red_entity);

    // Get the AI components
    const auto& blue_decision_component = blue_entity->Get<DecisionComponent>();
    const auto& red_decision_component = red_entity->Get<DecisionComponent>();

    // TimeStep the world twice, need to set the target first
    world->TimeStep();

    // Within range, should be attacking each other
    EXPECT_EQ(blue_decision_component.GetNextAction(), DecisionNextAction::kAttackAbility);
    EXPECT_EQ(red_decision_component.GetNextAction(), DecisionNextAction::kAttackAbility);
}

TEST_F(DecisionSystemTest, AttackAbilityRangeBoost)
{
    // We need to populate the abilities with some data otherwise
    // it does not make sense to use a specific ability if it does not
    // event exist in the first place
    AbilitiesData attack_abilities{};
    {
        auto& ability = attack_abilities.AddAbility();
        ability.total_duration_ms = 1000;
        auto& skill = ability.AddSkill();
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
    }
    AbilitiesData omega_abilities{};
    {
        auto& ability = omega_abilities.AddAbility();
        ability.total_duration_ms = 1000;
        auto& skill = ability.AddSkill();
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
    }

    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 10_fp);
    data.type_data.attack_abilities = attack_abilities;
    data.type_data.omega_abilities = omega_abilities;

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    // TimeStep the world to set target
    world->TimeStep();

    // Add an attached effect to boost the range
    auto effect_data = EffectData::CreateBuff(StatType::kAttackRangeUnits, EffectExpression::FromValue(10_fp), 100);
    EffectState effect_state{};
    const EntityID blue_entity_id = blue_entity->GetID();
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // TimeStep the world to make decision
    world->TimeStep();

    // Get the AI components
    auto& blue_decision_component = blue_entity->Get<DecisionComponent>();
    auto& red_decision_component = red_entity->Get<DecisionComponent>();

    // Blue within range due to buff, red not in range due to no buff
    EXPECT_EQ(blue_decision_component.GetNextAction(), DecisionNextAction::kAttackAbility);
    EXPECT_NE(red_decision_component.GetNextAction(), DecisionNextAction::kAttackAbility);
}

TEST_F(DecisionSystemTest, OmegaAbility)
{
    // We need to populate the abilities with some data otherwise
    // it does not make sense to use a specific ability if it does not
    // event exist in the first place
    AbilitiesData abilities{};
    {
        auto& ability = abilities.AddAbility();
        ability.total_duration_ms = 1000;
        auto& skill = ability.AddSkill();
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
    }

    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 100_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 30_fp);
    data.type_data.stats.Set(StatType::kOmegaRangeUnits, 30_fp);
    data.type_data.attack_abilities = abilities;
    data.type_data.omega_abilities = abilities;

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{10, 10}, data, blue_entity);

    // Add red entities
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{20, 20}, data, red_entity);

    // Get the AI components
    const auto& blue_decision_component = blue_entity->Get<DecisionComponent>();
    const auto& red_decision_component = red_entity->Get<DecisionComponent>();

    // TimeStep the world twice, need to set the target first
    world->TimeStep();

    // Both can do omega abilities due to max energy
    EXPECT_EQ(blue_decision_component.GetNextAction(), DecisionNextAction::kOmegaAbility);
    EXPECT_EQ(red_decision_component.GetNextAction(), DecisionNextAction::kOmegaAbility);
}

TEST_F(DecisionSystemTest, FindFocusRange)
{
    // Need ability for range check to work
    AbilitiesData attack_abilities{};
    {
        auto& ability = attack_abilities.AddAbility();
        ability.total_duration_ms = 1000;

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Simple Dummy Attack";
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = true;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 1;
        skill1.deployment.type = SkillDeploymentType::kDirect;

        // Tiny amount of damage
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(1_fp));

        // Instant
        skill1.deployment.pre_deployment_delay_percentage = 0;
    }

    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 3;
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 5_fp);
    data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    data.type_data.attack_abilities = attack_abilities;

    // Moving unit data
    CombatUnitData moving_data = CreateCombatUnitData();
    moving_data.radius_units = 3;
    moving_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 10000_fp);
    moving_data.type_data.stats.Set(StatType::kAttackRangeUnits, 2_fp);
    moving_data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);
    moving_data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{4, 4}, data, blue_entity);

    // Add red entities
    // Entity 1 is slightly closer than entity 2, but 2 can move closer
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{8, 15}, data, red_entity1);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{-20, 30}, moving_data, red_entity2);

    // TimeStep the world to get things going
    world->TimeStep();

    // Confirm red 1 is focus of blue
    const auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    ASSERT_EQ(blue_focus_component.GetFocus().get(), red_entity1);

    // TimeStep to move and update decisions (3 time steps)
    for (int i = 0; i < 3; i++)
    {
        world->TimeStep();
    }

    // Focus should have changed by now
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity2);
}

}  // namespace simulation
