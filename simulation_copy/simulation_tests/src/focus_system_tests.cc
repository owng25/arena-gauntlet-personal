#include "base_test_fixtures.h"
#include "components/combat_synergy_component.h"
#include "components/decision_component.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "data/effect_data.h"
#include "ecs/event_types_data.h"
#include "gtest/gtest.h"
#include "systems/focus_system.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
class FocusSystemTest : public BaseTest
{
public:
    bool UseSynergiesData() const override
    {
        return true;
    }
};

TEST_F(FocusSystemTest, NoTargets)
{
    // Add blue entity with position, target and stat components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);

    // TimeStep the world
    world->TimeStep();

    // Get the target component
    auto& blue_focus_component = blue_entity.Get<FocusComponent>();

    // Target should not be set because there is no enemy
    EXPECT_EQ(blue_focus_component.IsFocusSet(), false);
    EXPECT_EQ(blue_focus_component.GetFocus(), nullptr);
}

TEST_F(FocusSystemTest, SingleTarget)
{
    // Add blue entity with position, target and stat components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with position, target and stat components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();

    // Set position of entity at 20, 20
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(20, 20);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component = blue_entity.Get<FocusComponent>();
    auto& red_focus_component = red_entity.Get<FocusComponent>();

    // Entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), &blue_entity);
}

TEST_F(FocusSystemTest, SingleTargetRedHasPositiveStateUntargetable)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();

    // Stats
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 50_fp);

    // Add omega abilities
    {
        auto& ability = data.type_data.omega_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }
    data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    // auto& blue_attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();

    world->TimeStep();  // 1. kFindFocus

    // Entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);

    // Red has the positive state untargetable applied
    const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 1000);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), effect_data, effect_state);

    // TimeStep the world
    world->TimeStep();

    // Blue should not have a target
    EXPECT_EQ(blue_focus_component.IsFocusSet(), false);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), false);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), nullptr);

    // Red should still have it
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);

    // TimeStep the world
    world->TimeStep();

    // Red should still have a target
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, SingleTargetRedBecomesInactive1)
{
    // Add blue entity with position, target and stat components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    auto& blue_focus_component = blue_entity.Add<FocusComponent>();
    blue_focus_component.SetRefocusType(FocusComponent::RefocusType::kNever);
    blue_entity.Add<StatsComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with position, target and stat components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    auto& red_focus_component = red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();

    // Set position of entity at 20, 20
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(20, 20);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // TimeStep the world
    world->TimeStep();

    // Entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), &blue_entity);

    // Deactivate the red
    red_entity.Deactivate();

    // Expect target to still be set this time step but not active
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), false);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity);

    // Listen event about target being stuck
    std::vector<event_data::Focus> events_receiver_never_deactivated;
    world->SubscribeToEvent(
        EventType::kFocusNeverDeactivated,
        [&events_receiver_never_deactivated](const Event& event)
        {
            events_receiver_never_deactivated.emplace_back(event.Get<event_data::Focus>());
        });

    // TimeStep the world
    world->TimeStep();

    ASSERT_EQ(events_receiver_never_deactivated.size(), 1) << "Target never deactivate should have fired";
    ASSERT_EQ(events_receiver_never_deactivated.at(0).entity_id, blue_entity.GetID())
        << "Target never deactivated did not fire for blue";

    // Blue still has the target set because the target system can't retarget if
    // FocusComponent::RefocusType::kNever is set
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), false);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity);

    EXPECT_EQ(red_focus_component.IsFocusSet(), false);
    EXPECT_EQ(red_focus_component.IsFocusActive(), false);
    EXPECT_EQ(red_focus_component.GetFocus().get(), nullptr);
}

TEST_F(FocusSystemTest, SingleTargetRedBecomesInactive2)
{
    // Add blue entity with position, target and stat components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with position, target and stat components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();

    // Set position of entity at 20, 20
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(20, 20);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component = blue_entity.Get<FocusComponent>();
    auto& red_focus_component = red_entity.Get<FocusComponent>();

    // Entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), &blue_entity);

    // Deactivate the red
    red_entity.Deactivate();

    // Expect target to still be set this time step but not active
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), false);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity);

    // TimeStep the world
    world->TimeStep();

    // Expects to not have a target
    EXPECT_EQ(blue_focus_component.IsFocusSet(), false);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), false);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), nullptr);
    EXPECT_EQ(red_focus_component.IsFocusSet(), false);
    EXPECT_EQ(red_focus_component.IsFocusActive(), false);
    EXPECT_EQ(red_focus_component.GetFocus().get(), nullptr);
}

TEST_F(FocusSystemTest, SingleTargetRedHasPlaneChangeAirborne)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();

    // Stats
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 50_fp);

    // Add attack ability
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.targeting.guidance = MakeEnumSet(GuidanceType::kGround);
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.guidance = MakeEnumSet(GuidanceType::kGround);
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();

    world->TimeStep();  // 1. kFindFocus

    // Entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);

    // Red has the plane change airborne applied
    const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kAirborne, 1000);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), effect_data, effect_state);

    // TimeStep the world
    world->TimeStep();

    // Blue should not have a target
    EXPECT_EQ(blue_focus_component.IsFocusSet(), false);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), false);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), nullptr);

    // Red should still have it
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);

    // TimeStep the world
    world->TimeStep();

    // Blue should still not have a target
    EXPECT_EQ(blue_focus_component.IsFocusSet(), false);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), false);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), nullptr);

    // Red should still have a target
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, MultipleTargetsRed1HasPlaneChangeAirborne)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();

    // Stats
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 50_fp);

    // Add attack ability
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.targeting.guidance = MakeEnumSet(GuidanceType::kGround);
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.guidance = MakeEnumSet(GuidanceType::kGround);
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity1);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {30, 30}, data, red_entity2);

    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity1->Get<FocusComponent>();

    world->TimeStep();  // 1. kFindFocus

    // Entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity1);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);

    // Red 1 has the plane change airborne applied
    const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kAirborne, 1000);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(red_entity1->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity1, red_entity1->GetID(), effect_data, effect_state);

    // TimeStep the world
    world->TimeStep();

    // Blue should now target red 2
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity2);

    // Red 1 should still have it
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);

    // TimeStep the world
    world->TimeStep();

    // Red should still have a target
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, AirborneGuidanceTargetOnGround)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();

    // Stats
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 50_fp);

    // Add attack ability
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.targeting.guidance = MakeEnumSet(GuidanceType::kGround, GuidanceType::kAirborne);
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.guidance = MakeEnumSet(GuidanceType::kGround, GuidanceType::kAirborne);
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();

    world->TimeStep();  // 1. kFindFocus

    // Entities should have focus
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);

    // Red has the plane change airborne applied
    const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kAirborne, 1000);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), effect_data, effect_state);

    // TimeStep the world
    world->TimeStep();

    // Blue should now have a target
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);

    // Red should now have a focus
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);

    // TimeStep the world
    world->TimeStep();

    // Blue should still target red
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.IsFocusActive(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);

    // Red should now have a focus
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.IsFocusActive(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, SingleTargetRogue)
{
    // Add blue entity with position, target and stat components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with position, target, stat and combat class components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();
    auto red_synergy_component = red_entity.Add<CombatSynergyComponent>();

    // Set position of entity at 20, 20
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(20, 20);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // Make it a rogue
    red_synergy_component.SetCombatClass(CombatClass::kRogue);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component = blue_entity.Get<FocusComponent>();
    auto& red_focus_component = red_entity.Get<FocusComponent>();

    // Entities should target each other
    // Rogue makes no difference as there is no other valid target
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), &blue_entity);
}

TEST_F(FocusSystemTest, FocusTaunted)
{
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);

    // Spawn blue
    Entity* blue_entity1 = nullptr;
    SpawnCombatUnit(Team::kBlue, {5, 5}, data, blue_entity1);
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity2);

    // Spawn red
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity1);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {25, 25}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component1 = blue_entity1->Get<FocusComponent>();
    auto& blue_focus_component2 = blue_entity2->Get<FocusComponent>();
    auto& red_focus_component1 = red_entity1->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Entities should target closest enemy
    // Blue 1 is at 5, 5 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocusID(), red_entity1->GetID());
    // Blue 2 is at 10, 10 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocusID(), red_entity1->GetID());
    // Red 1 is at 20, 20 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component1.GetFocusID(), blue_entity2->GetID());
    // Red 2 is at 25, 25 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocusID(), blue_entity2->GetID());

    auto taunted_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity1->GetID());
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity1->GetID(),
        red_entity1->GetID(),
        taunted_effect_data,
        effect_state);

    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocusID(), red_entity1->GetID());
    // Blue 2 is at 10, 10 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocusID(), red_entity1->GetID());
    // Red1 is taunted by blue1
    EXPECT_EQ(red_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component1.GetFocusID(), blue_entity1->GetID());
    // Red 2 is at 25, 25 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocusID(), blue_entity2->GetID());

    // TimeStep the world
    world->TimeStep();

    // Check that Red1 is still focused on blue1
    EXPECT_EQ(red_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component1.GetFocusID(), blue_entity1->GetID());
}

TEST_F(FocusSystemTest, MultipleTargets)
{
    // Add blue entities with position, target and stat components
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);

    // Spawn blue
    Entity* blue_entity1 = nullptr;
    SpawnCombatUnit(Team::kBlue, {5, 5}, data, blue_entity1);
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity2);

    // Spawn red
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity1);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {25, 25}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component1 = blue_entity1->Get<FocusComponent>();
    auto& blue_focus_component2 = blue_entity2->Get<FocusComponent>();
    auto& red_focus_component1 = red_entity1->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Entities should target closest enemy
    // Blue 1 is at 5, 5 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocus().get(), red_entity1);
    // Blue 2 is at 10, 10 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocus().get(), red_entity1);
    // Red 1 is at 20, 20 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component1.GetFocus().get(), blue_entity2);
    // Red 2 is at 25, 25 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity2);
}

TEST_F(FocusSystemTest, MultipleTargetsWithRogues)
{
    // Data for entities that are not rogues
    CombatUnitData non_rogue_data = CreateCombatUnitData();
    non_rogue_data.radius_units = 1;
    non_rogue_data.type_data.stats.Set(StatType::kAttackRangeUnits, 500_fp);
    non_rogue_data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);
    non_rogue_data.type_data.combat_affinity = CombatAffinity::kFire;
    non_rogue_data.type_data.combat_class = CombatClass::kBehemoth;

    // Data for rogues
    CombatUnitData rogue_data = CreateCombatUnitData();
    rogue_data.radius_units = 1;
    rogue_data.type_data.stats.Set(StatType::kAttackRangeUnits, 500_fp);
    rogue_data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);
    rogue_data.type_data.combat_affinity = CombatAffinity::kFire;
    rogue_data.type_data.combat_class = CombatClass::kPhantom;

    // Spawn blue
    Entity* blue_entity1 = nullptr;
    SpawnCombatUnit(Team::kBlue, {5, 5}, non_rogue_data, blue_entity1);
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, rogue_data, blue_entity2);

    // Spawn red
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, rogue_data, red_entity1);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {25, 25}, non_rogue_data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component1 = blue_entity1->Get<FocusComponent>();
    auto& blue_focus_component2 = blue_entity2->Get<FocusComponent>();
    auto& red_focus_component1 = red_entity1->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Entities should target closest enemy, but preferably not a rogue
    // Blue 1 is at 5, 5 and red 1 is at 20, 20 - closest, but is a rogue
    // Blue 1 is at 5, 5 and red 2 is at 25, 25 - closest that is not a rogue
    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocus().get(), red_entity2);
    // Blue 2 is at 10, 10 and red 1 is at 20, 20 - closest and both are rogues
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocus().get(), red_entity1);
    // Red 1 is at 20, 20 and blue 2 is at 10, 10 - closest and both are rogues
    EXPECT_EQ(red_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component1.GetFocus().get(), blue_entity2);
    // Red 2 is at 25, 25 and blue 2 is at 10, 10 - closest, but is a rogue
    // Red 2 is at 25, 25 and blue 1 is at 5, 5 - closest that is not a rogue
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity1);
}

TEST_F(FocusSystemTest, MultipleTargetsWithRoguesOutOfRange)
{
    // Data for entities that are not rogues
    CombatUnitData non_rogue_data = CreateCombatUnitData();
    non_rogue_data.radius_units = 1;
    non_rogue_data.type_data.stats.Set(StatType::kAttackRangeUnits, 1_fp);
    non_rogue_data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);
    non_rogue_data.type_data.combat_affinity = CombatAffinity::kFire;
    non_rogue_data.type_data.combat_class = CombatClass::kBehemoth;

    // Data for rogues
    CombatUnitData rogue_data = CreateCombatUnitData();
    rogue_data.radius_units = 1;
    rogue_data.type_data.stats.Set(StatType::kAttackRangeUnits, 1_fp);
    rogue_data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);
    rogue_data.type_data.combat_affinity = CombatAffinity::kFire;
    rogue_data.type_data.combat_class = CombatClass::kVanguard;

    // Spawn blue
    Entity* blue_entity1 = nullptr;
    SpawnCombatUnit(Team::kBlue, {5, 5}, non_rogue_data, blue_entity1);
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, rogue_data, blue_entity2);

    // Spawn red
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, rogue_data, red_entity1);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {25, 25}, non_rogue_data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component1 = blue_entity1->Get<FocusComponent>();
    auto& blue_focus_component2 = blue_entity2->Get<FocusComponent>();
    auto& red_focus_component1 = red_entity1->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Entities should target closest enemy
    // Don't care about rogues because nothing is in range
    // Blue 1 is at 5, 5 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocus().get(), red_entity1);
    // Blue 2 is at 10, 10 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocus().get(), red_entity1);
    // Red 1 is at 20, 20 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component1.GetFocus().get(), blue_entity2);
    // Red 2 is at 25, 25 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity2);
}

TEST_F(FocusSystemTest, TargetUnreachable)
{
    // Add blue entity with position, target and stat components

    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    blue_entity.Add<StatsComponent>();
    blue_entity.Add<DecisionComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with position, target and stat components

    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();

    // Set position of entity at 20, 20
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(20, 20);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // Add another red entity with position, target and stat components
    auto& red_entity2 = world->AddEntity(Team::kRed);
    red_entity2.Add<PositionComponent>();
    red_entity2.Add<FocusComponent>();
    red_entity2.Add<StatsComponent>();

    // Set position of entity at 50, 50
    auto& red_position_component2 = red_entity2.Get<PositionComponent>();
    red_position_component2.SetPosition(50, 50);
    red_position_component2.SetRadius(1);
    red_position_component2.SetTakingSpace(true);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component = blue_entity.Get<FocusComponent>();
    auto& red_focus_component = red_entity.Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2.Get<FocusComponent>();

    // Closest entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), &blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), &blue_entity);

    // Force a target switch for blue
    world->BuildAndEmitEvent<EventType::kFocusUnreachable>(blue_entity.GetID(), red_entity.GetID());

    // TimeStep the world
    world->TimeStep();

    // Blue focus should change
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), &red_entity2);

    // Rest should remain the same
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), &blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), &blue_entity);
}

TEST_F(FocusSystemTest, RefocusStun)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    CombatUnitData blue_data = CreateCombatUnitData();
    blue_data.radius_units = 1;
    blue_data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    blue_data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{5, 10}, blue_data, blue_entity);

    // Add red entities
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{10, 20}, data, red_entity);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{25, 50}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Closest entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);

    // Stun blue
    Stun(*blue_entity, 100);

    // Make entity closer to other target
    auto& blue_position_component = blue_entity->Get<PositionComponent>();
    blue_position_component.SetPosition(50, 50);

    // TimeStep the world twice to let stun expire
    world->TimeStep();
    world->TimeStep();

    // Focus should be reset
    EXPECT_EQ(blue_focus_component.IsFocusSet(), false);

    // TimeStep again to set new focus
    world->TimeStep();

    // Blue focus should change
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity2);

    // Rest should remain the same
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, RefocusDisplacement)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    CombatUnitData blue_data = CreateCombatUnitData();
    blue_data.radius_units = 1;
    blue_data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    blue_data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{5, 10}, blue_data, blue_entity);

    // Add red entities
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{10, 20}, data, red_entity);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{25, 50}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Closest entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);

    // Knock up blue for 1 time step
    const auto displacement_effect_data = EffectData::CreateDisplacement(EffectDisplacementType::kKnockUp, 100);
    EffectState displacement_effect_state{};
    displacement_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*blue_entity, red_entity->GetID(), displacement_effect_data, displacement_effect_state);

    // Make entity closer to other target
    auto& blue_position_component = blue_entity->Get<PositionComponent>();
    blue_position_component.SetPosition(50, 50);

    // TimeStep the world twice to let displacement finish
    world->TimeStep();
    world->TimeStep();

    // Focus should be reset
    EXPECT_EQ(blue_focus_component.IsFocusSet(), false);

    // TimeStep again to set new focus
    world->TimeStep();

    // Blue focus should change
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity2);

    // Rest should remain the same
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, RefocusMovementOmega)
{
    AbilitiesData omega_abilities{};
    {
        auto& ability = omega_abilities.AddAbility();
        ability.total_duration_ms = 1500;

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Simple Dash Attack";
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = false;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 1;
        skill1.deployment.type = SkillDeploymentType::kDash;

        // Set the dash data
        skill1.dash.apply_to_all = false;
        skill1.percentage_of_ability_duration = 100;

        // Tiny amount of damage
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(1_fp));

        // Instant
        skill1.deployment.pre_deployment_delay_percentage = 0;
    }

    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);
    CombatUnitData blue_data = CreateCombatUnitData();
    blue_data.radius_units = 1;
    blue_data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    blue_data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);
    blue_data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    blue_data.type_data.omega_abilities = omega_abilities;

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{5, 10}, blue_data, blue_entity);

    // Add red entities
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{15, 25}, data, red_entity);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{30, 55}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Give blue energy to do omega
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    blue_stats_component.SetCurrentEnergy(blue_stats_component.GetEnergyCost());

    // Get the target components
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Closest entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);

    // TimeStep for the dash to complete
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Focus should be reset
    EXPECT_EQ(blue_focus_component.IsFocusSet(), false);

    // TimeStep again to set new focus
    world->TimeStep();

    // Blue focus should change
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity2);

    // Rest should remain the same
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, NoRefocusMovementOmegaStillInRange)
{
    AbilitiesData omega_abilities{};
    {
        auto& ability = omega_abilities.AddAbility();
        ability.total_duration_ms = 1500;

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Simple Dash Attack";
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = false;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 1;
        skill1.deployment.type = SkillDeploymentType::kDash;

        // Set the dash data
        skill1.dash.apply_to_all = false;
        skill1.percentage_of_ability_duration = 100;

        // Tiny amount of damage
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(1_fp));

        // Instant
        skill1.deployment.pre_deployment_delay_percentage = 0;
    }

    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);
    CombatUnitData blue_data = CreateCombatUnitData();
    blue_data.radius_units = 1;
    blue_data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    blue_data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);
    blue_data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    blue_data.type_data.stats.Set(StatType::kAttackRangeUnits, 27_fp);
    blue_data.type_data.omega_abilities = omega_abilities;

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{5, 10}, blue_data, blue_entity);

    // Add red entities
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{15, 25}, data, red_entity);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{30, 55}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Give blue energy to do omega
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    blue_stats_component.SetCurrentEnergy(blue_stats_component.GetEnergyCost());

    // Get the target components
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Closest entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);

    // TimeStep for the dash to be between two entities
    TimeStepForNTimeSteps(9);

    // Focus should not be reset
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);

    // TimeStep again to try set new focus
    world->TimeStep();

    // Blue focus should not change because still in range
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);

    // Rest should also remain the same
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, RefocusUntargettable)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{5, 10}, data, blue_entity);

    // Add red entities
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{10, 20}, data, red_entity);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{25, 50}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Closest entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);

    // Make red untargettable
    const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 100);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), effect_data, effect_state);

    // TimeStep to set new focus
    world->TimeStep();

    // Blue focus should change
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity2);

    // Rest should remain the same
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);
}

TEST_F(FocusSystemTest, FocusFocused)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);

    // Add blue entity
    Entity* blue_entity1 = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{5, 5}, data, blue_entity1);
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{10, 10}, data, blue_entity2);

    // Add red entities
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{20, 20}, data, red_entity1);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{25, 25}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component1 = blue_entity1->Get<FocusComponent>();
    auto& blue_focus_component2 = blue_entity2->Get<FocusComponent>();
    auto& red_focus_component1 = red_entity1->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Entities should target closest enemy
    // Blue 1 is at 5, 5 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocusID(), red_entity1->GetID());
    // Blue 2 is at 10, 10 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocusID(), red_entity1->GetID());
    // Red 1 is at 20, 20 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component1.GetFocusID(), blue_entity2->GetID());
    // Red 2 is at 25, 25 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocusID(), blue_entity2->GetID());

    auto focused_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kFocused, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity1->GetID());
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity1->GetID(),
        red_entity1->GetID(),
        focused_effect_data,
        effect_state);

    // TimeStep the world
    world->TimeStep();

    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocusID(), red_entity1->GetID());
    // Blue 2 is at 10, 10 and red 1 is at 20, 20 - closest
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocusID(), red_entity1->GetID());
    // Red1 is focused by blue1
    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocusID(), red_entity1->GetID());
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocusID(), red_entity1->GetID());
    // Red 2 is at 25, 25 and blue 2 is at 10, 10 - closest
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocusID(), blue_entity2->GetID());
}

TEST_F(FocusSystemTest, PreferFacingTargets)
{
    // Add blue entities with position, target and stat components
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);

    // Spawn blue
    Entity* blue_entity1 = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -30}, data, blue_entity1);
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {30, -30}, data, blue_entity2);

    // Spawn red
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, {-30, 30}, data, red_entity1);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {0, 30}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component1 = blue_entity1->Get<FocusComponent>();
    auto& blue_focus_component2 = blue_entity2->Get<FocusComponent>();
    auto& red_focus_component1 = red_entity1->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Entities should target enemy with angle closest to forward
    // Blue 1 is at 0, -30 and red 1 is at -30, 30 - facing each other
    EXPECT_EQ(blue_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component1.GetFocus().get(), red_entity1);
    // Blue 2 is at 30, -30 and red 2 is at 0, 30 - facing each other
    EXPECT_EQ(blue_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component2.GetFocus().get(), red_entity2);
    // Red 1 is at -30, 30 and blue 1 is at 0, -30 - facing each other
    EXPECT_EQ(red_focus_component1.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component1.GetFocus().get(), blue_entity1);
    // Red 2 is at 0, 30 and blue 2 is at 30, -30 - facing each other
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity2);
}

TEST_F(FocusSystemTest, RefocusSignificantlyCloserTarget)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMaxHealth, 5000000_fp);
    data.type_data.stats.Set(StatType::kCurrentHealth, 5000000_fp);

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{10, 10}, data, blue_entity);

    // Add red entities
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{5, 5}, data, red_entity);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{45, 45}, data, red_entity2);

    // TimeStep the world
    world->TimeStep();

    // Get the target components
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    auto& red_focus_component2 = red_entity2->Get<FocusComponent>();

    // Closest entities should target each other
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity);
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);

    // Move entity far away from focus
    auto& blue_position_component = blue_entity->Get<PositionComponent>();
    blue_position_component.SetPosition(60, 60);

    // TimeStep to set new focus
    world->TimeStep();

    // Blue focus should change
    EXPECT_EQ(blue_focus_component.IsFocusSet(), true);
    EXPECT_EQ(blue_focus_component.GetFocus().get(), red_entity2);

    // Rest should remain the same
    EXPECT_EQ(red_focus_component.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component.GetFocus().get(), blue_entity);
    EXPECT_EQ(red_focus_component2.IsFocusSet(), true);
    EXPECT_EQ(red_focus_component2.GetFocus().get(), blue_entity);
}

}  // namespace simulation
