#include <optional>

#include "ability_system_data_fixtures.h"
#include "components/attached_effects_component.h"
#include "components/decision_component.h"
#include "components/focus_component.h"
#include "components/stats_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "gtest/gtest.h"
#include "systems/ability_system.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
TEST_F(AbilitySystemBaseTest, ApplyEffectPackageWithAttack)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();

    // Stats
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);

    // Add attack abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
        skill1.effect_package.attributes.always_crit = true;
        skill1.effect_package.attributes.use_hit_chance = true;
    }
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Add Entities
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Listen to fainted event
    std::vector<event_data::Fainted> events_fainted;
    world->SubscribeToEvent(
        EventType::kFainted,
        [&events_fainted](const Event& event)
        {
            events_fainted.emplace_back(event.Get<event_data::Fainted>());
        });

    // Call function with minimum hit chance
    blue_template_stats.Set(StatType::kHitChancePercentage, kMinPercentageFP);
    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    ASSERT_EQ(events_effect_package_received.size(), 0) << "Attack should have NOT fired";
    ASSERT_EQ(events_effect_package_missed.size(), 1) << "Attack should have missed";
    ASSERT_EQ(events_fainted.size(), 0) << "Fainted event should have NOT fired";

    // Call function with maximum hit chance and critical chance
    events_effect_package_missed.clear();
    blue_template_stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        attack_ability->skills.at(0).CheckAllIfIsCritical(),
        red_entity->GetID());

    ASSERT_EQ(events_effect_package_missed.size(), 0) << "Attack should have NOT missed";
    ASSERT_EQ(events_effect_package_received.size(), 1) << "Attack should have fired";
    ASSERT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Attack should have hit the red";
    ASSERT_TRUE(events_effect_package_received[0].is_critical) << "Attack ability should have been critical";
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp) << "Red health should be 50 (100 + 50 critical)";
    ASSERT_EQ(events_fainted.size(), 0) << "Fainted event should have NOT fired";
}

TEST_F(AbilitySystemBaseTest, ResetAttackSequenceAfterTheStun)
{
    Entity *red_entity{}, *blue_entity{}, *blue_entity_2{};
    EventHistory<EventType::kAbilityActivated> activated(*world);
    EventHistory<EventType::kAbilityDeactivated> deactivated(*world);

    {
        CombatUnitData unit_data = CreateCombatUnitData();

        // Stats
        unit_data.radius_units = 1;
        unit_data.type_data.combat_affinity = CombatAffinity::kFire;
        unit_data.type_data.combat_class = CombatClass::kBehemoth;
        unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kCurrentHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kAttackRangeUnits, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kAttackSpeed, 500_fp);

        // Add attack abilities
        unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

        for (size_t i = 0; i != 10; ++i)
        {
            auto& ability = unit_data.type_data.attack_abilities.AddAbility();
            ability.total_duration_ms = 100;
            auto& skill1 = ability.AddSkill();
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(1_fp));
        }

        SpawnCombatUnit(Team::kRed, {20, 20}, unit_data, red_entity);
        ASSERT_NE(red_entity, nullptr);
    }

    // Spawn blue entity which will be stunned for the whole combat
    {
        CombatUnitData unit_data = CreateCombatUnitData();

        // Stats
        unit_data.radius_units = 1;
        unit_data.type_data.combat_affinity = CombatAffinity::kFire;
        unit_data.type_data.combat_class = CombatClass::kBehemoth;
        unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kCurrentHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kAttackRangeUnits, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kAttackSpeed, 500_fp);

        SpawnCombatUnit(Team::kBlue, {10, 10}, unit_data, blue_entity);
        ASSERT_NE(blue_entity, nullptr);
        Stun(*blue_entity);
    }

    // Let red entity do a few attacks
    do  // NOLINT
    {
        world->TimeStep();
    } while (!activated.IsEmpty() && activated.events.back().ability_index != 5);

    // Spawn another blue entity which stuns red entity for 200ms
    {
        CombatUnitData unit_data = CreateCombatUnitData();

        // Stats
        unit_data.radius_units = 1;
        unit_data.type_data.combat_affinity = CombatAffinity::kFire;
        unit_data.type_data.combat_class = CombatClass::kBehemoth;
        unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kCurrentHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kAttackRangeUnits, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

        {
            auto& ability = unit_data.type_data.attack_abilities.AddAbility();
            ability.total_duration_ms = 100;
            auto& skill1 = ability.AddSkill();
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddEffect(EffectData::CreateNegativeState(EffectNegativeState::kStun, 800));
        }

        SpawnCombatUnit(Team::kBlue, {30, 30}, unit_data, blue_entity_2);
        ASSERT_NE(blue_entity_2, nullptr);
    }

    auto red_has_stun = [&]()
    {
        return red_entity->Get<AttachedEffectsComponent>().HasNegativeState(EffectNegativeState::kStun);
    };

    while (!red_has_stun()) world->TimeStep();

    Stun(*blue_entity_2);

    activated.Clear();
    // Wait util previously applied stun fades
    while (red_has_stun())
    {
        world->TimeStep();
    }

    // Wait for the next activation
    while (activated.IsEmpty())
    {
        world->TimeStep();
    }

    ASSERT_EQ(activated.events.front().ability_index, 0);
}

TEST_F(AbilitySystemBaseTest, ApplyEffectPackageWithOmega)
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

    // Add Entities
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Listen to fainted event
    std::vector<event_data::Fainted> events_fainted;
    world->SubscribeToEvent(
        EventType::kFainted,
        [&events_fainted](const Event& event)
        {
            events_fainted.emplace_back(event.Get<event_data::Fainted>());
        });

    // Call function with minimum hit chance
    // Should always fire for omega abilities
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kHitChancePercentage, kMinPercentageFP);
    auto& omega_state = blue_abilities_component.GetStateOmegaAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        omega_state,
        omega_state->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    ASSERT_EQ(events_effect_package_received.size(), 1) << "Omega Ability should have fired";
    ASSERT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID())
        << "Omega Ability should have hit the entity";
    ASSERT_EQ(events_fainted.size(), 1) << "Fainted event not called";
    ASSERT_EQ(events_fainted[0].entity_id, red_entity->GetID()) << "Red did not faint";
}

TEST_F(AbilitySystemTestNoDecisionSystem, SimpleTimeStep)
{
    // Check if state was initialized
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    auto& blue_attack_abilities = blue_abilities_component.GetStateAttackAbilities();
    auto& blue_omega_abilities = blue_abilities_component.GetStateOmegaAbilities();
    auto& red_attack_abilities = red_abilities_component.GetStateAttackAbilities();
    auto& red_omega_abilities = red_abilities_component.GetStateOmegaAbilities();

    // Check if we choose an ability it is at least the right domain
    ASSERT_EQ(blue_attack_abilities.size(), 1);

    // No target yet
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    ASSERT_FALSE(blue_focus_component.IsFocusSet());
    ASSERT_FALSE(red_focus_component.IsFocusSet());

    // Expect abilities to not have changed yet
    ASSERT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(blue_omega_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(red_omega_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_FALSE(EntityHelper::HasEnoughEnergyForOmega(*blue_entity));

    // TimeStep, do all the things, we don't have a decision system to stop us
    world->TimeStep();

    // Expect target is set
    ASSERT_TRUE(blue_focus_component.IsFocusSet());
    ASSERT_TRUE(red_focus_component.IsFocusSet());

    // Should not time step yet
    ASSERT_EQ(blue_omega_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(red_omega_abilities[0]->GetCurrentAbilityTimeMs(), 0);

    // Should activate the first ability so the time step count in that state should be 0
    ASSERT_TRUE(blue_attack_abilities[0]->is_active);
    ASSERT_TRUE(red_attack_abilities[0]->is_active);
    ASSERT_TRUE(blue_abilities_component.HasActiveAbility());
    ASSERT_TRUE(red_abilities_component.HasActiveAbility());

    // Check duration
    ASSERT_EQ(events_ability_activated.size(), 2);
    {
        const auto& ability_event_activated = events_ability_activated.at(0);

        // Get sender omega abilities state
        const EntityID sender_id = ability_event_activated.sender_id;
        ASSERT_TRUE(world->HasEntity(sender_id));
        const auto& sender_entity = world->GetByID(sender_id);
        auto& sender_abilities_components = sender_entity.Get<AbilitiesComponent>();
        std::vector<AbilityStatePtr>& sender_omega_abilities_state =
            sender_abilities_components.GetStateAttackAbilities();

        // Get the ability state
        const size_t ability_index = ability_event_activated.ability_index;
        ASSERT_TRUE(ability_index < sender_omega_abilities_state.size());
        const auto& ability_state = sender_omega_abilities_state.at(ability_index);

        // 100 ms is 1 time step
        // Check if we set the data correctly
        ASSERT_EQ(ability_state->skills.size(), 1);
        ASSERT_EQ(ability_state->total_duration_ms, 1000);
        // Only one skill per ability should match the total
        ASSERT_EQ(ability_state->skills[0].duration_ms, 1000);
        ASSERT_EQ(ability_state->skills[0].pre_deployment_delay_ms, 200);
    }

    // TimeStep again, now the active attack ability should step
    world->TimeStep();

    ASSERT_TRUE(blue_attack_abilities[0]->is_active);
    ASSERT_TRUE(red_attack_abilities[0]->is_active);
    ASSERT_TRUE(blue_abilities_component.HasActiveAbility());
    ASSERT_TRUE(red_abilities_component.HasActiveAbility());

    ASSERT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), 100);
    ASSERT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), 100);
}

TEST_F(AbilitySystemTest, SimpleTimeStep)
{
    // Check if state was initialized
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    auto& blue_attack_abilities = blue_abilities_component.GetStateAttackAbilities();
    auto& blue_omega_abilities = blue_abilities_component.GetStateOmegaAbilities();
    auto& red_attack_abilities = red_abilities_component.GetStateAttackAbilities();
    auto& red_omega_abilities = red_abilities_component.GetStateOmegaAbilities();

    auto& blue_decision_component = blue_entity->Get<DecisionComponent>();

    // Expect abilities to not have changed yet
    ASSERT_EQ(blue_attack_abilities.size(), 1);
    ASSERT_EQ(blue_attack_abilities[0]->skills.size(), 1);
    auto& skill1_state = blue_attack_abilities[0]->skills[0];

    // No target yet
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    ASSERT_FALSE(blue_focus_component.IsFocusSet());
    ASSERT_FALSE(red_focus_component.IsFocusSet());

    // Should activate the first ability
    ASSERT_FALSE(blue_attack_abilities[0]->is_active);
    ASSERT_FALSE(red_attack_abilities[0]->is_active);
    ASSERT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(blue_omega_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(red_omega_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_FALSE(blue_abilities_component.HasActiveAbility());
    ASSERT_FALSE(red_abilities_component.HasActiveAbility());
    ASSERT_FALSE(EntityHelper::HasEnoughEnergyForOmega(*blue_entity));

    // TimeStep again, should be in range
    ASSERT_EQ(skill1_state.state, SkillStateType::kNone);

    // TimeStep, move to find target
    ASSERT_EQ(DecisionNextAction::kNone, blue_decision_component.GetNextAction());
    world->TimeStep();  // 1. kAttackAbility

    // Expect target is set as the target system is after the decision system
    ASSERT_TRUE(blue_focus_component.IsFocusSet());
    ASSERT_TRUE(red_focus_component.IsFocusSet());

    // ASSERT_TRUE(blue_focus_component.IsFocusSet());
    // ASSERT_TRUE(red_focus_component.IsFocusSet());
    // ASSERT_EQ(DecisionNextAction::kMoveToFocus,
    // blue_decision_component.GetNextAction());
    ASSERT_EQ(DecisionNextAction::kAttackAbility, blue_decision_component.GetNextAction());

    // Should be active as the ability sytem is last
    ASSERT_TRUE(blue_abilities_component.HasActiveAbility());
    ASSERT_TRUE(red_abilities_component.HasActiveAbility());

    // These should not time step yet
    ASSERT_FALSE(blue_omega_abilities[0]->is_active);
    ASSERT_FALSE(red_omega_abilities[0]->is_active);
    ASSERT_EQ(blue_omega_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(red_omega_abilities[0]->GetCurrentAbilityTimeMs(), 0);

    // Should activate the first ability
    ASSERT_TRUE(blue_attack_abilities[0]->is_active);
    ASSERT_TRUE(red_attack_abilities[0]->is_active);
    ASSERT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);

    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);

    {
        const auto& ability_event_activated = events_ability_activated.at(0);

        // Get sender omega abilities state
        const EntityID sender_id = ability_event_activated.sender_id;
        ASSERT_TRUE(world->HasEntity(sender_id));
        const auto& sender_entity = world->GetByID(sender_id);
        auto& sender_abilities_components = sender_entity.Get<AbilitiesComponent>();
        std::vector<AbilityStatePtr>& sender_omega_abilities_state =
            sender_abilities_components.GetStateAttackAbilities();

        // Get the ability state
        const size_t ability_index = ability_event_activated.ability_index;
        ASSERT_TRUE(ability_index < sender_omega_abilities_state.size());
        const auto& ability_state = sender_omega_abilities_state.at(ability_index);

        // 100 ms is 1 time step
        // Check if we set the data correctly
        ASSERT_EQ(ability_state->skills.size(), 1);
        ASSERT_EQ(ability_state->total_duration_ms, 1000);
        // Only one skill per ability should match the total
        ASSERT_EQ(ability_state->skills[0].duration_ms, 1000);
        ASSERT_EQ(ability_state->skills[0].pre_deployment_delay_ms, 200);
    }

    // 2. Should transition from waiting to to deployment
    world->TimeStep();
    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);
    world->TimeStep();

    // Should be deploying now
    ASSERT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), 200);
    ASSERT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), 200);
    ASSERT_EQ(skill1_state.state, SkillStateType::kDeploying);
    ASSERT_FALSE(blue_attack_abilities[0]->IsFinished());

    // TimeStep until we finished the skill
    int current_time_ms = blue_attack_abilities[0]->GetCurrentAbilityTimeMs();
    while (current_time_ms < skill1_state.duration_ms - kMsPerTimeStep)
    {
        world->TimeStep();
        current_time_ms += kMsPerTimeStep;
        ASSERT_EQ(skill1_state.state, SkillStateType::kDeploying);
        ASSERT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), current_time_ms);
        ASSERT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), current_time_ms);
        ASSERT_FALSE(blue_attack_abilities[0]->IsFinished());
    }

    // Last time step should deactivate abilities
    // and activate the next ones
    world->TimeStep();
    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);
    EXPECT_TRUE(blue_attack_abilities[0]->is_active);
    EXPECT_TRUE(red_attack_abilities[0]->is_active);
    EXPECT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    EXPECT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    EXPECT_FALSE(blue_attack_abilities[0]->IsFinished());
}

TEST_F(AbilitySystemTest, AbilitiesEventsFire)
{
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_mutable_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_mutable_stats = red_stats_component.GetMutableTemplateStats();

    red_mutable_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kAttackDodgeChancePercentage, kMinPercentageFP)
        .Set(StatType::kEnergyResist, 10_fp)
        .Set(StatType::kPhysicalResist, 20_fp)
        .Set(StatType::kCritChancePercentage, 0_fp)
        .Set(StatType::kGrit, 10_fp);

    blue_mutable_stats.Set(StatType::kMaxHealth, 100_fp)
        .Set(StatType::kCurrentHealth, 100_fp)
        .Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp)
        .Set(StatType::kEnergyResist, 10_fp)
        .Set(StatType::kPhysicalResist, 20_fp)
        .Set(StatType::kCritChancePercentage, 0_fp)
        .Set(StatType::kGrit, 10_fp)
        .Set(StatType::kCurrentEnergy, 0_fp);

    auto& blue_attack_abilities = blue_abilities_component.GetStateAttackAbilities();
    auto& red_attack_abilities = red_abilities_component.GetStateAttackAbilities();

    // Check if we set the data correctly
    ASSERT_EQ(blue_attack_abilities.size(), 1);
    ASSERT_EQ(blue_attack_abilities[0]->skills.size(), 1);
    auto& skill1_state = blue_attack_abilities[0]->skills[0];

    ASSERT_FALSE(blue_abilities_component.HasActiveAbility());
    ASSERT_FALSE(red_abilities_component.HasActiveAbility());

    // Listen to skill waiting event
    std::vector<event_data::Skill> events_skill_waiting;
    world->SubscribeToEvent(
        EventType::kSkillWaiting,
        [&events_skill_waiting](const Event& event)
        {
            events_skill_waiting.emplace_back(event.Get<event_data::Skill>());
        });

    ASSERT_EQ(skill1_state.state, SkillStateType::kNone);
    // Active ability
    world->TimeStep();  // 1. kFindFocus, kAttackAbility

    ASSERT_TRUE(blue_abilities_component.HasActiveAbility());
    ASSERT_TRUE(red_abilities_component.HasActiveAbility());
    ASSERT_EQ(events_ability_activated.size(), 2) << "Ability activation event not fired";
    ASSERT_EQ(events_skill_waiting.size(), 2) << "Skill waiting event not fired";
    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);

    events_ability_activated.clear();
    events_skill_waiting.clear();

    // Waiting
    world->TimeStep();  // 2.

    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);

    // Listen to skill deployment event
    // NOTE: because our ability can not channel we are just executing it
    std::vector<event_data::Skill> events_skill_deploying;
    world->SubscribeToEvent(
        EventType::kSkillDeploying,
        [&events_skill_deploying](const Event& event)
        {
            events_skill_deploying.emplace_back(event.Get<event_data::Skill>());
        });

    // Listen to health changed
    std::vector<event_data::StatChanged> events_health_changed;
    world->SubscribeToEvent(
        EventType::kHealthChanged,
        [&events_health_changed](const Event& event)
        {
            events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Listen to energy changed
    std::vector<event_data::StatChanged> events_energy_changed;
    world->SubscribeToEvent(
        EventType::kEnergyChanged,
        [&events_energy_changed](const Event& event)
        {
            events_energy_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Listen to fainted event
    std::vector<event_data::Fainted> events_fainted;
    world->SubscribeToEvent(
        EventType::kFainted,
        [&events_fainted](const Event& event)
        {
            events_fainted.emplace_back(event.Get<event_data::Fainted>());
        });

    const FixedPoint blue_before_energy = blue_stats_component.GetCurrentEnergy();
    const FixedPoint red_before_energy = red_stats_component.GetCurrentEnergy();

    // Should deploy because it waited 2 time steps for predeploy
    events_energy_changed.clear();
    world->TimeStep();  // 3.
    ASSERT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), 200);
    ASSERT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), 200);

    ASSERT_EQ(events_skill_waiting.size(), 0) << "Skill waiting event SHOULD have NOT fired";
    ASSERT_EQ(events_skill_deploying.size(), 2) << "Skill deploying event SHOULD have fired";
    ASSERT_EQ(skill1_state.state, SkillStateType::kDeploying);
    events_skill_deploying.clear();

    // General ability events
    ASSERT_EQ(2, events_effect_package_received.size()) << "Attack hit events not fired on blue and red";
    ASSERT_EQ(events_effect_package_missed.size(), 0) << "Ability attack miss event SHOULD HAVE not fired";
    ASSERT_EQ(red_entity->GetID(), events_effect_package_received[0].receiver_id)
        << "Ability attack hit event not fired on the correct target from blue";
    ASSERT_EQ(blue_entity->GetID(), events_effect_package_received[1].receiver_id)
        << "Ability attack hit event not fired on the correct target from red";

    // Effect events
    ASSERT_EQ(2, events_health_changed.size()) << "Health changed events not fired on blue and red";

    ASSERT_EQ(4, events_energy_changed.size()) << "Energy changed events should have fired for 2"
                                                  "attack abilities and 2 pre mitigated damages";

    ASSERT_EQ(0, events_fainted.size()) << "No entity should have fainted";

    //
    // Test new stats
    //

    // Test delta is larger than 0
    ASSERT_GT(events_energy_changed.at(0).delta, 0_fp);
    ASSERT_GT(events_energy_changed.at(1).delta, 0_fp);
    ASSERT_GT(events_energy_changed.at(2).delta, 0_fp);
    ASSERT_GT(events_energy_changed.at(3).delta, 0_fp);
    ASSERT_GT(blue_stats_component.GetCurrentEnergy(), blue_before_energy) << "Energy should have increase for blue";
    ASSERT_GT(red_stats_component.GetCurrentEnergy(), red_before_energy) << "Energy should have increase for red";

    // Health should have been reduced
    // 10 grit, 10% physical resist
    ASSERT_EQ(425_fp, red_stats_component.GetCurrentHealth());
    ASSERT_EQ(25_fp, blue_stats_component.GetCurrentHealth());

    // Clear previous events
    events_effect_package_received.clear();
    events_effect_package_received.clear();
    events_health_changed.clear();
    events_energy_changed.clear();
    events_fainted.clear();

    // TimeStep until we finished the skill
    int current_time_ms = blue_attack_abilities[0]->GetCurrentAbilityTimeMs();
    while (current_time_ms < skill1_state.duration_ms - kMsPerTimeStep)
    {
        world->TimeStep();
        current_time_ms += kMsPerTimeStep;
        ASSERT_EQ(skill1_state.state, SkillStateType::kDeploying);
        ASSERT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), current_time_ms);
        ASSERT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), current_time_ms);
        ASSERT_FALSE(blue_attack_abilities[0]->IsFinished());
        ASSERT_FALSE(red_attack_abilities[0]->IsFinished());
    }

    // Listen to finished event
    std::vector<event_data::Skill> events_skill_finished;
    world->SubscribeToEvent(
        EventType::kSkillFinished,
        [&events_skill_finished](const Event& event)
        {
            events_skill_finished.emplace_back(event.Get<event_data::Skill>());
        });

    std::optional<event_data::BattleFinished> event_game_finished;
    world->SubscribeToEvent(
        EventType::kBattleFinished,
        [&event_game_finished](const Event& event)
        {
            event_game_finished = event.Get<event_data::BattleFinished>();
        });

    // Last time step should deactivate abilities and activate them again
    world->TimeStep();
    EXPECT_EQ(skill1_state.state, SkillStateType::kWaiting);
    EXPECT_TRUE(blue_attack_abilities[0]->is_active);
    EXPECT_TRUE(red_attack_abilities[0]->is_active);
    EXPECT_EQ(blue_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    EXPECT_EQ(red_attack_abilities[0]->GetCurrentAbilityTimeMs(), 0);
    EXPECT_FALSE(blue_attack_abilities[0]->IsFinished());

    ASSERT_EQ(events_skill_finished.size(), 2) << "Both Skills should have finished";
    ASSERT_EQ(events_ability_deactivated.size(), 2) << "Both Abilities should have deactiated";

    ASSERT_EQ(0, events_effect_package_received.size()) << "No attack hit events should have fired";
    ASSERT_EQ(0, events_health_changed.size()) << "No Health changed events should have fired";
    ASSERT_EQ(0, events_energy_changed.size()) << "No Energy changed events should have fired";
    ASSERT_EQ(0, events_fainted.size()) << "No entity should have fainted";
    ASSERT_FALSE(event_game_finished.has_value()) << "There should be no winner yet";

    // Clear previous events
    events_skill_finished.clear();
    events_ability_deactivated.clear();

    //
    // Faint blue by doing another attack cycle
    //
    world->TimeStep();  // 12 kWaiting
    world->TimeStep();  // 13 kWaiting
    world->TimeStep();  // 14 kDeploying
    world->TimeStep();  // 15 Let focus reset

    ASSERT_EQ(1, events_fainted.size()) << "Red should have fainted";
    ASSERT_EQ(blue_entity->GetID(), events_fainted[0].entity_id) << "Blue should have fainted";
    ASSERT_EQ(red_entity->GetTeam(), event_game_finished->winning_team) << "Red entity should have won the game";

    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    auto& red_focus_component = red_entity->Get<FocusComponent>();
    ASSERT_TRUE(red_entity->IsActive());
    ASSERT_FALSE(blue_entity->IsActive());
    ASSERT_FALSE(blue_focus_component.IsFocusSet());
    ASSERT_FALSE(red_focus_component.IsFocusSet());
}

TEST_F(AbilitySystemTest, FinishesWaitingSkillIfTargetBecomesInactive)
{
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    auto& blue_attack_abilities = blue_abilities_component.GetStateAttackAbilities();

    // Check if we set the data correctly
    ASSERT_EQ(blue_attack_abilities.size(), 1);
    ASSERT_EQ(blue_attack_abilities[0]->skills.size(), 1);
    auto& skill1_state = blue_attack_abilities[0]->skills[0];

    ASSERT_FALSE(blue_abilities_component.HasActiveAbility());
    ASSERT_FALSE(red_abilities_component.HasActiveAbility());
    ASSERT_EQ(skill1_state.state, SkillStateType::kNone);

    // Active ability
    world->TimeStep();  // 1. kFindFocus, kAttackAbility
    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);
    ASSERT_TRUE(blue_abilities_component.HasActiveAbility());
    ASSERT_TRUE(red_abilities_component.HasActiveAbility());
    ASSERT_EQ(events_ability_activated.size(), 2) << "Ability activation event not fired";

    world->TimeStep();  // 2.
    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);

    std::vector<event_data::Skill> events_skill_finished;
    world->SubscribeToEvent(
        EventType::kSkillFinished,
        [&events_skill_finished](const Event& event)
        {
            events_skill_finished.emplace_back(event.Get<event_data::Skill>());
        });

    // Force deactivate the red entity
    red_entity->Deactivate();

    // Should deactivate and fire finished event
    world->TimeStep();  // 3
    ASSERT_EQ(events_skill_finished.size(), 1) << "Should have deactivated skill";
    ASSERT_EQ(events_ability_deactivated.size(), 1)
        << "Ability deactivated event not fired even tho the red is no longer active";
    ASSERT_TRUE(blue_entity->IsActive());
    ASSERT_FALSE(red_entity->IsActive());
    ASSERT_FALSE(blue_abilities_component.HasActiveAbility());
}

TEST_F(AbilitySystemTest, FinishesFinishedAbilityIfTargetBecomesInactive)
{
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    auto& blue_attack_abilities = blue_abilities_component.GetStateAttackAbilities();

    // Check if we set the data correctly
    ASSERT_EQ(blue_attack_abilities.size(), 1);
    ASSERT_EQ(blue_attack_abilities[0]->skills.size(), 1);
    auto& skill1_state = blue_attack_abilities[0]->skills[0];

    ASSERT_EQ(skill1_state.state, SkillStateType::kNone);
    ASSERT_FALSE(blue_abilities_component.HasActiveAbility());
    ASSERT_FALSE(red_abilities_component.HasActiveAbility());

    // Active ability
    world->TimeStep();  // 1. kFindFocus, kAttackAbility
    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);
    ASSERT_TRUE(blue_abilities_component.HasActiveAbility());
    ASSERT_TRUE(red_abilities_component.HasActiveAbility());
    ASSERT_EQ(events_ability_activated.size(), 2) << "Ability activation event not fired";

    world->TimeStep();  // 2.
    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);

    std::vector<event_data::Skill> events_skill_finished;
    world->SubscribeToEvent(
        EventType::kSkillFinished,
        [&events_skill_finished](const Event& event)
        {
            events_skill_finished.emplace_back(event.Get<event_data::Skill>());
        });

    // In the meantime the red entity was deactivated by another entity
    red_entity->Deactivate();

    // 3. TimeStep again, Should deactivate the ability and set the ability to finished state
    world->TimeStep();
    ASSERT_EQ(events_skill_finished.size(), 1) << "Should have deactivated skill";
    ASSERT_EQ(events_ability_deactivated.size(), 1)
        << "Ability deactivated event not fired even tho the red is no longer active";
    ASSERT_TRUE(blue_entity->IsActive());
    ASSERT_FALSE(red_entity->IsActive());
    ASSERT_FALSE(blue_abilities_component.HasActiveAbility());
    events_skill_finished.clear();
    events_ability_deactivated.clear();

    // Simulate forced state that can happen if an combat unit finishes an ability in a time step
    // But the target becomes invalid right in the next time step
    // Activate back the entity to simulate this
    red_entity->Activate();

    // Fewer time steps we have to do
    // blue_attack_abilities[0]->attack_cooldown_time_steps = 0;

    // Activate ability again
    world->TimeStep();  // 4. Find and activate
    ASSERT_TRUE(blue_abilities_component.HasActiveAbility());
    // On Activate the skill should be back to Waitin
    ASSERT_EQ(skill1_state.state, SkillStateType::kWaiting);
    events_skill_finished.clear();
    events_ability_deactivated.clear();

    // But the in the meantime the red entity was deactivated by another entity
    red_entity->Deactivate();

    // 5. Should deactivate and fire finished event
    world->TimeStep();
    ASSERT_TRUE(blue_entity->IsActive());
    ASSERT_FALSE(red_entity->IsActive());
    ASSERT_FALSE(blue_abilities_component.HasActiveAbility());
    ASSERT_EQ(events_skill_finished.size(), 1) << "Should have deactivated skill";
    ASSERT_EQ(events_ability_deactivated.size(), 1)
        << "Ability deactivated event not fired even tho the red is no longer active";
}

TEST_F(AbilitySystemTestWithStartOmega, DeactivateAndActivateDifferentTimeSteps)
{
    // Activate always happen in a different time step
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    std::vector<event_data::StatChanged> events_energy_changed;
    world->SubscribeToEvent(
        EventType::kEnergyChanged,
        [&events_energy_changed](const Event& event)
        {
            events_energy_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Make entities not die
    SetHealthVeryHigh();

    auto& blue_omega_abilities = blue_abilities_component.GetStateOmegaAbilities();
    ASSERT_EQ(blue_omega_abilities.size(), 1);

    world->TimeStep();  // 1. Decision kFindFocus and kOmegaAbility activate
    ASSERT_EQ(events_ability_activated.size(), 2) << "Omega abilities should have activated";

    // Energy was changed twice, once for each activation of an omega
    ASSERT_EQ(events_energy_changed.size(), 2);
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kOmega);
    events_ability_activated.clear();

    TimeStepForNTimeSteps(9);
    // 7 world time steps

    ASSERT_EQ(0, events_ability_deactivated.size()) << "Should have not deactivate yet";
    ASSERT_EQ(0, events_ability_activated.size()) << "Should have not activated yet";
    world->TimeStep();  // 8. kFinished

    // Should not deactivate and activate in the same time step
    ASSERT_EQ(events_ability_activated.size(), 0);
    ASSERT_EQ(events_ability_deactivated.size(), 2);
    ASSERT_NE(events_ability_deactivated.size(), events_ability_activated.size())
        << "Should have NOT deactivated and activated abilities in the same time step";
}

TEST_F(AbilitySystemTestWithStartAttackEvery1Second, AttackEverySecond)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    // Make entities not die
    SetHealthVeryHigh();

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    // 1s
    TimeStepForSeconds(1);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 2s
    TimeStepForSeconds(1);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 3s
    TimeStepForSeconds(1);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemTest_AttackSpeedOverflowToAplification, AmplificationFromAttackSpeedOverflow)
{
    SetHealthVeryHigh();

    ASSERT_EQ(world->GetLiveStats(*blue_entity).Get(StatType::kAttackSpeed), 100_fp);
    ASSERT_EQ(world->GetLiveStats(*red_entity).Get(StatType::kAttackSpeed), 100_fp);
    ASSERT_EQ(world->GetMaxAttackSpeed(), 100_fp);

    EventHistory<EventType::kOnDamage> damage_received(*world);

    for (size_t i = 0; i != 10; ++i)
    {
        TimeStepForSeconds(1);

        ASSERT_EQ(damage_received.Size(), 2);
        EXPECT_EQ(damage_received[0].damage_value, 100_fp);
        EXPECT_EQ(damage_received[1].damage_value, 100_fp);
        damage_received.Clear();

        ASSERT_EQ(events_ability_activated.size(), 2);
        EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
        EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
        events_ability_activated.clear();
    }

    // Apply attack speed buff
    {
        auto effect_data =
            EffectData::CreateBuff(StatType::kAttackSpeed, EffectExpression::FromValue(300_fp), kTimeInfinite);
        EffectState effect_state{};

        effect_state.sender_stats = world->GetFullStats(*blue_entity);
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

        effect_state.sender_stats = world->GetFullStats(*red_entity);
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), effect_data, effect_state);

        ASSERT_EQ(world->GetLiveStats(*blue_entity).Get(StatType::kAttackSpeed), 400_fp);
        ASSERT_EQ(world->GetLiveStats(*red_entity).Get(StatType::kAttackSpeed), 400_fp);
    }

    for (size_t i = 0; i != 10; ++i)
    {
        TimeStepForSeconds(1);

        ASSERT_EQ(damage_received.Size(), 2);
        EXPECT_EQ(damage_received[0].damage_value, 400_fp);
        EXPECT_EQ(damage_received[1].damage_value, 400_fp);
        damage_received.Clear();

        ASSERT_EQ(events_ability_activated.size(), 2);
        EXPECT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
        EXPECT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
        events_ability_activated.clear();
    }
}

TEST_F(AbilitySystemTestWithStartAttackEvery0Point53Seconds, AttackEvery0Point53Seconds)
{
    // Activate always happen in a different time step
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Make entities not die
    SetHealthVeryHigh();

    // 0.1s should fire attacks
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 0) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 0.7s should fire attacks 600 ms after the last ones because we always count time past
    // the duration of the ability, so the 529ms ability will take 600ms the first time.
    TimeStepForMs(600);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // Since the ability used 600ms, but it was supposed to only take 529ms, we've overflowed by 600 - 529 = 71ms
    // This means that we're already 71ms into the next attack, so we attack 71ms to the current time of the attack
    // as a starting value.
    // Note that the total duration of the ability will always be 529ms for every attack in the series.
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 71) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 1.2s should fire attacks 500 ms after the last ones
    // This happens because we started at 71ms for the new attack, so the remaining time is 529 - 71 = 458ms
    TimeStepForMs(500);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 500ms have passed, but the attack was only going to take 458ms (calculations above), so we've overflowed
    // by 500 - 458 = 42ms. Therefore the new current time for the attack starts at 42ms.
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 42) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 1.7s should fire attacks 500 ms after the last ones
    // This happens because we started at 42ms for the new attack, so the remaining time is 529 - 42 = 487ms
    TimeStepForMs(500);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 500ms have passed, but the attack was only going to take 487ms (calculations above), so we've overflowed
    // by 500 - 487 = 13ms. Therefore the new current time for the attack starts at 13ms.
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 13) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 2.1s No attacks should fire attacks 500 ms after the last ones because
    // overflow causes previous ability to take 100ms more
    // This is because the attack duration is 529ms and we started at 13ms, so 516ms had to pass before
    // the next attack starts. This means that we have to wait 600ms to go past the 516ms.
    TimeStepForMs(500);
    ASSERT_EQ(events_ability_activated.size(), 0) << "Shouldn't have fired for either entity";

    // 2.2s should fire attacks 600 ms after the last ones because
    // overflow causes previous ability to take 100ms more
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 600ms have passed, but the attack was only going to take 516ms (calculations above), so we've overflowed
    // by 500 - 516 = 84ms. Therefore the new current time for the attack starts at 84ms.
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 84) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";
}

TEST_F(AbilitySystemTestWithOverflowResets, OmegaOverflowResets)
{
    // Activate always happen in a different time step
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    // Make entities not die
    SetHealthVeryHigh();

    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();
    red_template_stats.Set(StatType::kEnergyCost, 20_fp);

    // 0.1s should fire attacks
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_NE(blue_abilities_component.GetActiveAbility(), nullptr);
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->ability_type, AbilityType::kAttack);
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529);
    ASSERT_NE(red_abilities_component.GetActiveAbility(), nullptr);
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->ability_type, AbilityType::kAttack);
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 0);
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->total_duration_ms, 529);

    // 0.7s should fire attacks 600 ms after the last ones because we always count time past
    // the duration of the ability, so the 529ms ability will take 600ms the first time.
    TimeStepForMs(600);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated[0].ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated[1].ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 71);
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529);
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 71);
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->total_duration_ms, 529);

    {
        TimeStepForMs(100);
        ASSERT_EQ(events_ability_activated.size(), 0);

        auto blue_ability = blue_abilities_component.GetActiveAbility();
        EXPECT_EQ(blue_ability->ability_type, AbilityType::kAttack);
        ASSERT_EQ(blue_ability->GetCurrentAbilityTimeMs(), 171);
        EXPECT_GE(blue_stats_component.GetCurrentEnergy(), 20_fp);

        auto red_ability = red_abilities_component.GetActiveAbility();
        EXPECT_EQ(red_ability->ability_type, AbilityType::kAttack);
        ASSERT_EQ(red_ability->GetCurrentAbilityTimeMs(), 171);
        EXPECT_EQ(red_stats_component.GetCurrentEnergy(), 20_fp) << "Current energy must be clamped to energy cost";
    }

    {
        // At this time step the ability system detects that red entity has enough energy for omega
        TimeStepForMs(100);
        ASSERT_EQ(events_ability_activated.size(), 1) << "Expect red entity to interrupt its attack and do omega";
        EXPECT_EQ(events_ability_activated[0].ability_type, AbilityType::kOmega);
        EXPECT_EQ(events_ability_activated[0].sender_id, red_entity->GetID());

        auto blue_ability = blue_abilities_component.GetActiveAbility();
        EXPECT_EQ(blue_ability->ability_type, AbilityType::kAttack);
        ASSERT_EQ(blue_ability->GetCurrentAbilityTimeMs(), 271);

        auto red_ability = red_abilities_component.GetActiveAbility();
        EXPECT_EQ(red_ability->ability_type, AbilityType::kOmega);
        ASSERT_EQ(red_ability->GetCurrentAbilityTimeMs(), 0);
        EXPECT_EQ(red_ability->total_duration_ms, 700);
        EXPECT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);
    }

    {
        events_ability_activated.clear();
        TimeStepForMs(300);
        ASSERT_EQ(events_ability_activated.size(), 1);
        EXPECT_EQ(events_ability_activated[0].ability_type, AbilityType::kAttack);
        EXPECT_EQ(events_ability_activated[0].sender_id, blue_entity->GetID());

        auto blue_ability = blue_abilities_component.GetActiveAbility();
        EXPECT_EQ(blue_ability->ability_type, AbilityType::kAttack);
        ASSERT_EQ(blue_ability->GetCurrentAbilityTimeMs(), 42)
            << "42ms from the previous attack overflow should have transitioned to the next attack";
    }
}

TEST_F(AbilitySystemTestWithOverflowResets, InterruptOverflowResets)
{
    // Activate always happen in a different time step
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    // Make entities not die
    SetHealthVeryHigh();

    // 0.1s should fire attacks
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 0) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 0) << "Current time incorrect!";
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 0.7s should fire attacks 600 ms after the last ones because of overflow
    TimeStepForMs(600);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 71) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 71) << "Current time incorrect!";
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 0.8s Stun red
    TimeStepForMs(100);

    Stun(*red_entity, 400);

    // 1.2s should fire attack for blue entity
    TimeStepForMs(400);
    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have only fired once for blue entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 42) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 1.4s should fire attack for red as it comes out of stun
    // The important part of this tests is that the attack duration has been reset to 529
    // so the stun wiped out the overflow time based on the attack speed
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have only fired once for red entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // Attack time should be reset to default since this is first attack after stun
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 0) << "Current time incorrect!";
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 1.7s should fire attack 500 ms after the last one for blue entity
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have only fired once for blue entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 13) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";

    // 2.0s Red does another regular attack
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have only fired once for red entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 71) << "Current time incorrect!";
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->total_duration_ms, 529) << "Duration incorrect!";
}

TEST_F(AbilitySystemTestWithStartAttackEvery2Seconds, AttackEvery2Seconds)
{
    // Make entities not die
    SetHealthVeryHigh();

    // 2s
    TimeStepForSeconds(2);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);

    events_ability_activated.clear();

    // 4s
    TimeStepForSeconds(2);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 6s
    TimeStepForSeconds(2);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemTestWithStartAttackEvery1Point6Seconds, AttackEvery1Point6Seconds)
{
    // Make entities not die
    SetHealthVeryHigh();

    // 1.6s
    TimeStepForMs(1600);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 3.2s
    TimeStepForMs(1600);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 4.8 s
    TimeStepForMs(1600);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemTestWithStartAttackEveryTimeStep, AttackNegativeStateBlind)
{
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
    ASSERT_FALSE(EntityHelper::IsBlinded(*blue_entity));

    // Blind for 3 time steps
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 300);

    // Red
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), effect_data, effect_state);
    }

    // Blue
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity->GetID(), effect_data, effect_state);
    }

    // Is blinded
    ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));
    ASSERT_TRUE(EntityHelper::IsBlinded(*blue_entity));

    for (int i = 0; i < 3; i++)
    {
        events_effect_package_missed.clear();
        events_effect_package_received.clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // // Should not fire
        ASSERT_EQ(events_effect_package_received.size(), 0) << "i = " << i;
        ASSERT_EQ(events_effect_package_missed.size(), 2) << "i = " << i;

        // Should be blinded
        ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));
        ASSERT_TRUE(EntityHelper::IsBlinded(*blue_entity));
    }
    ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));

    events_effect_package_missed.clear();
    events_effect_package_received.clear();

    // TimeStep again, eliminate the negative effect
    world->TimeStep();
    ASSERT_EQ(events_effect_package_received.size(), 0);
    ASSERT_EQ(events_effect_package_missed.size(), 2);
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
    ASSERT_FALSE(EntityHelper::IsBlinded(*blue_entity));

    // Attacks should have hit
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
    ASSERT_FALSE(EntityHelper::IsBlinded(*blue_entity));
    ASSERT_EQ(events_effect_package_received.size(), 2);
    ASSERT_EQ(events_effect_package_missed.size(), 2);
}

TEST_F(AbilitySystemTestWithVeryHighAttackSpeed, VeryHighAttackSpeed)
{
    // Make entities not die
    SetHealthVeryHigh();

    for (int i = 0; i < 50; ++i)
    {
        // 0.1s
        TimeStepForMs(100);
        ASSERT_EQ(events_ability_activated.size(), 2) << "Should have fired once for both entities";
        ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
        ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
        events_ability_activated.clear();
    }
}

TEST_F(AbilitySystemTestWithSimpleSpeedChanges, SimpleSpeedChanges)
{
    EntityID blue_entity_id = blue_entity->GetID();
    EntityID red_entity_id = red_entity->GetID();

    // Make entities not die
    SetHealthVeryHigh();

    // Add the effect which will slow red from attack speed 200 to 100, so 500ms between attacks
    // goes to 1000ms between attacks
    auto effect_data = EffectData::CreateDebuff(StatType::kAttackSpeed, EffectExpression::FromValue(100_fp), 1000);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // 0.1s should activate both attacks.
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have activated for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 0.3s should fire attack because attack deployment is at 40% of a 500ms attack for blue entity
    TimeStepForMs(200);
    ASSERT_EQ(events_skill_deployed.size(), 1) << "Should have only deployed once";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, blue_entity_id);
    events_skill_deployed.clear();

    // 0.5s should fire attack because attack deployment is at 40% of a 1000ms attack for red entity
    TimeStepForMs(200);
    ASSERT_EQ(events_skill_deployed.size(), 1) << "Should have only deployed once";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, red_entity_id);
    events_skill_deployed.clear();

    // 0.6s blue entity attacks again
    // red is slowed
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have only fired once for blue entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(0).sender_id, blue_entity_id);
    events_ability_activated.clear();

    // 0.8s should fire attack because attack deployment is at 40% of a 500ms attack for blue entity
    TimeStepForMs(200);
    ASSERT_EQ(events_skill_deployed.size(), 1) << "Should have only deployed once";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, blue_entity_id);
    events_skill_deployed.clear();

    // 1.1s should activate both attacks again.
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have activated for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // 1.3s Both entity attack deploy because they're in sync again
    TimeStepForMs(200);
    ASSERT_EQ(events_skill_deployed.size(), 2) << "Should have deployed for both entities";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, blue_entity_id);
    ASSERT_EQ(events_skill_deployed.at(1).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(1).sender_id, red_entity_id);
    events_skill_deployed.clear();

    // 1.4s Slow down red again
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
    TimeStepForMs(100);

    // 1.6s blue entity attacks again
    // red is slowed
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have only fired once for blue entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(0).sender_id, blue_entity_id);
    events_ability_activated.clear();

    // 1.8s red entity attacks again
    TimeStepForMs(200);
    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have only fired once for red entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(0).sender_id, red_entity_id);
    events_ability_activated.clear();

    // Blue entity should also have deployed the attack
    ASSERT_EQ(events_skill_deployed.size(), 1) << "Should have only deployed once";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, blue_entity_id);
    events_skill_deployed.clear();

    // 2.1s blue entity attacks again
    TimeStepForMs(300);
    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have only fired once for blue entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(0).sender_id, blue_entity_id);
    events_ability_activated.clear();

    // Red entity deploys attack because it's delayed
    TimeStepForMs(100);
    ASSERT_EQ(events_skill_deployed.size(), 1) << "Should have only deployed once";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, red_entity_id);
    events_skill_deployed.clear();
}

TEST_F(AbilitySystemTestWithComplexSpeedChanges, ComplexSpeedChanges)
{
    EntityID blue_entity_id = blue_entity->GetID();
    EntityID red_entity_id = red_entity->GetID();

    // Activate always happen in a different time step
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();

    // Make entities not die
    SetHealthVeryHigh();

    // 0.1s should activate both attacks.
    TimeStepForMs(100);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have activated for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 0) << "Current time incorrect!";
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 0) << "Current time incorrect!";

    {
        const auto effect_data =
            EffectData::CreateDebuff(StatType::kAttackSpeed, EffectExpression::FromValue(52_fp), 500);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "Debuff1";
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
    }

    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kAttackSpeed, EffectExpression::FromValue(96_fp), 500);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(red_entity_id);
        effect_state.source_context.ability_name = "Buff1";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity_id, effect_data, effect_state);
    }

    TimeStepForMs(100);

    // 0.2s
    // Speed changes take one time step to be applied, so no effect yet.
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 100) << "Current time incorrect!";
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 100) << "Current time incorrect!";

    {
        const auto effect_data =
            EffectData::CreateDebuff(StatType::kAttackSpeed, EffectExpression::FromValue(79_fp), 300);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "Debuff2";
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
    }

    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kAttackSpeed, EffectExpression::FromValue(116_fp), 300);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(red_entity_id);
        effect_state.source_context.ability_name = "Buff2";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity_id, effect_data, effect_state);
    }

    TimeStepForMs(100);

    // 0.3s
    // 189 - 52 = 137 attack speed. Time should be 100ms * 137 / 189 = 72
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 172) << "Current time incorrect!";
    // 189 + 96 = 285 attack speed. Time should be 100ms * 285 / 189 = 150
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 250) << "Current time incorrect!";

    // blue deploys because current time is over 211ms
    ASSERT_EQ(events_skill_deployed.size(), 1) << "Should have only deployed once";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, blue_entity_id);
    events_skill_deployed.clear();

    {
        const auto effect_data =
            EffectData::CreateDebuff(StatType::kAttackSpeed, EffectExpression::FromValue(102_fp), 300);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "Debuff3";
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
    }

    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kAttackSpeed, EffectExpression::FromValue(220_fp), 300);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(red_entity_id);
        effect_state.source_context.ability_name = "Buff3";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity_id, effect_data, effect_state);
    }

    TimeStepForMs(100);

    // 0.4s
    // 189 - 52 - 79 = 58 attack speed. Time should be 100ms * 58 / 189 = 72
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 202) << "Current time incorrect!";
    // 189 + 96 + 116 = 401 attack speed. Time should be 100ms * 401 / 189 = 212
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 462) << "Current time incorrect!";

    TimeStepForMs(100);

    // 0.5s
    // 189 - 52 - 79 - 102 = -44 attack speed. Time should be 100ms * 0 / 189 = 0
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 202) << "Current time incorrect!";
    // 189 + 96 + 116 + 220 = 621 attack speed. Time should be 100ms * 621 / 189 = 328
    // Previous current time was 462, so next will be 462 + 328 = 790ms
    // Ability should take 529ms, so overflow will be 790 - 529 = 261ms
    // So current time at start should be 261ms
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 261) << "Current time incorrect!";

    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have activated for one entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    // Blue entity deploys attack
    ASSERT_EQ(events_skill_deployed.size(), 1) << "Should have only deployed once";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, blue_entity_id);
    events_skill_deployed.clear();

    TimeStepForMs(100);

    // 0.6s
    // 189 - 52 - 79 - 102 = -44 attack speed. Time should be 100ms * 0 / 189 = 0
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 202) << "Current time incorrect!";
    // 189 + 96 + 116 + 220 = 621 attack speed. Time should be 100ms * 621 / 189 = 328
    // Previous current time was 261, so next will be 261 + 328 = 589ms
    // Ability should take 529ms, so overflow will be 589 - 529 = 60s
    // So current time at start should be 60ms
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 60) << "Current time incorrect!";

    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have activated for one entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();

    TimeStepForMs(100);

    // 0.7s
    // Attached effects start to expire
    // 189 - 52 - 102 = 35 attack speed. Time should be 100ms * 35 / 189 = 18
    // Previous current time was 202, so next will be 202 + 18 = 220ms
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 220) << "Current time incorrect!";
    // 189 + 96 + 220 = 505 attack speed. Time should be 100ms * 621 / 189 = 267
    // Previous current time was 60, so next will be 60 + 267 = 327ms
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 327) << "Current time incorrect!";

    // Should fire attacks

    ASSERT_EQ(events_skill_deployed.size(), 2) << "Should have deployed twice";
    ASSERT_EQ(events_skill_deployed.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(1).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_skill_deployed.at(0).sender_id, blue_entity_id);
    ASSERT_EQ(events_skill_deployed.at(1).sender_id, red_entity_id);
    events_skill_deployed.clear();

    TimeStepForMs(100);

    // 0.8s
    // All effects expire
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 320) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 427) << "Current time incorrect!";

    ASSERT_EQ(events_ability_activated.size(), 0) << "No skills should have activated";

    TimeStepForMs(100);

    // 0.9s
    // All effects expire
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 420) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 527) << "Current time incorrect!";

    ASSERT_EQ(events_ability_activated.size(), 0) << "No skills should have activated";

    TimeStepForMs(100);

    // 1.0s
    // All effects expire
    ASSERT_EQ(red_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 520) << "Current time incorrect!";
    ASSERT_EQ(blue_abilities_component.GetActiveAbility()->GetCurrentAbilityTimeMs(), 98) << "Current time incorrect!";

    ASSERT_EQ(events_ability_activated.size(), 1) << "Should have activated for one entity";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemTest, AbilitiesEventsFireMinDodge)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kMaxHealth, 1000_fp)
        .Set(StatType::kCurrentHealth, 1000_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP)
        .Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);

    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP)
        .Set(StatType::kAttackDodgeChancePercentage, kMinPercentageFP);

    // Listen to attack dodged
    // Should fire for red but not blue
    std::vector<event_data::EffectPackage> events_effect_package_dodged;
    world->SubscribeToEvent(
        EventType::kEffectPackageDodged,
        [&events_effect_package_dodged](const Event& event)
        {
            events_effect_package_dodged.emplace_back(event.Get<event_data::EffectPackage>());
        });

    // Listen to health changed
    std::vector<event_data::StatChanged> events_health_changed;
    world->SubscribeToEvent(
        EventType::kHealthChanged,
        [&events_health_changed](const Event& event)
        {
            events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Listen to energy changed
    std::vector<event_data::StatChanged> events_energy_changed;
    world->SubscribeToEvent(
        EventType::kEnergyChanged,
        [&events_energy_changed](const Event& event)
        {
            events_energy_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    std::vector<event_data::ApplyEnergyGain> events_energy_gained;
    world->SubscribeToEvent(
        EventType::kApplyEnergyGain,
        [&events_energy_gained](const Event& event)
        {
            events_energy_gained.emplace_back(event.Get<event_data::ApplyEnergyGain>());
        });

    const FixedPoint blue_before_energy = blue_stats_component.GetCurrentEnergy();
    const FixedPoint red_before_energy = red_stats_component.GetCurrentEnergy();

    // Deploy skill, because the way random overflow stats work, the first one won't work
    events_effect_package_dodged.clear();
    events_health_changed.clear();
    events_energy_changed.clear();
    events_energy_gained.clear();

    // Deploy skill
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Skills events
    EXPECT_EQ(1, events_health_changed.size()) << "Health changed events not fired on blue and red";
    EXPECT_EQ(3, events_energy_changed.size());
    EXPECT_EQ(3, events_energy_gained.size());

    EXPECT_GT(blue_stats_component.GetCurrentEnergy(), blue_before_energy) << "Energy should have increase for blue";
    EXPECT_GT(red_stats_component.GetCurrentEnergy(), red_before_energy) << "Energy should have increase for red";

    // Health should have been reduced
    EXPECT_EQ(400_fp, red_stats_component.GetCurrentHealth());

    // No health reduced from blue since they dodged
    EXPECT_EQ(1000_fp, blue_stats_component.GetCurrentHealth());

    ASSERT_EQ(1, events_effect_package_dodged.size()) << "blue entity should have dodged because of max dodge chance";
    ASSERT_EQ(blue_entity->GetID(), events_effect_package_dodged.at(0).receiver_id) << "blue entity should have dodged";
}

TEST_F(AbilitySystemTest, AbilitiesEventsFireMaxDodge)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    red_template_stats.Set(StatType::kMaxHealth, 1000_fp)
        .Set(StatType::kCurrentHealth, 1000_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP)
        .Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);

    blue_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP)
        .Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);

    // Listen to attack dodged
    // Should fire for red but not blue
    std::vector<event_data::EffectPackage> events_effect_package_dodged;
    world->SubscribeToEvent(
        EventType::kEffectPackageDodged,
        [&events_effect_package_dodged](const Event& event)
        {
            events_effect_package_dodged.emplace_back(event.Get<event_data::EffectPackage>());
        });

    // Listen to health changed
    std::vector<event_data::StatChanged> events_health_changed;
    world->SubscribeToEvent(
        EventType::kHealthChanged,
        [&events_health_changed](const Event& event)
        {
            events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Listen to energy changed
    std::vector<event_data::StatChanged> events_energy_changed;
    world->SubscribeToEvent(
        EventType::kEnergyChanged,
        [&events_energy_changed](const Event& event)
        {
            events_energy_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    std::vector<event_data::ApplyEnergyGain> events_energy_gained;
    world->SubscribeToEvent(
        EventType::kApplyEnergyGain,
        [&events_energy_gained](const Event& event)
        {
            events_energy_gained.emplace_back(event.Get<event_data::ApplyEnergyGain>());
        });

    const FixedPoint blue_before_energy = blue_stats_component.GetCurrentEnergy();
    const FixedPoint red_before_energy = red_stats_component.GetCurrentEnergy();

    // Deploy skill, because the way random overflow stats work, the first one won't work
    events_effect_package_dodged.clear();
    events_health_changed.clear();
    events_energy_changed.clear();
    events_energy_gained.clear();

    // Now it should dodge
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Skills events
    EXPECT_EQ(0, events_health_changed.size()) << "Health changed events not fired on blue and red";
    EXPECT_EQ(2, events_energy_changed.size()) << "Energy changed events should have fired for 2"
                                                  "attack abilities";
    // events_energy_gained confirmation
    EXPECT_EQ(2, events_energy_gained.size());

    EXPECT_GT(blue_stats_component.GetCurrentEnergy(), blue_before_energy) << "Energy should have increase for blue";
    EXPECT_GT(red_stats_component.GetCurrentEnergy(), red_before_energy) << "Energy should have increase for red";

    // Health should not have been reduced
    EXPECT_EQ(1000_fp, red_stats_component.GetCurrentHealth());
    // No health reduced from blue since they dodged
    EXPECT_EQ(500_fp, blue_stats_component.GetCurrentHealth());

    ASSERT_EQ(2, events_effect_package_dodged.size()) << "red & blue entity should have dodged";
    EXPECT_EQ(red_entity->GetID(), events_effect_package_dodged.at(0).receiver_id) << "red entity should have dodged";
    EXPECT_EQ(blue_entity->GetID(), events_effect_package_dodged.at(1).receiver_id) << "blue entity should have dodged";
}

TEST_F(AbilitySystemTest, ApplyEffectNegativeClumsy)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // Red should be able to attack at first step
    ASSERT_TRUE(EntityHelper::CanDodge(*red_entity));

    // Red should always dodge in theory
    red_template_stats.Set(StatType::kMaxHealth, 1000_fp)
        .Set(StatType::kCurrentHealth, 1000_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP)
        .Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);

    // Listen to attack dodged
    // Should fire for red but not blue
    std::vector<event_data::EffectPackage> events_effect_package_dodged;
    world->SubscribeToEvent(
        EventType::kEffectPackageDodged,
        [&events_effect_package_dodged](const Event& event)
        {
            events_effect_package_dodged.emplace_back(event.Get<event_data::EffectPackage>());
        });

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Apply the effect package on the red
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // Red should have dodged
    ASSERT_TRUE(EntityHelper::CanDodge(*red_entity));
    ASSERT_EQ(events_effect_package_dodged.size(), 1);
    ASSERT_EQ(events_effect_package_received.size(), 0);
    ASSERT_EQ(events_effect_package_dodged.at(0).sender_id, blue_entity->GetID());
    ASSERT_EQ(events_effect_package_dodged.at(0).receiver_id, red_entity->GetID());

    // Make the red to be clumsy
    events_effect_package_dodged.clear();
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kClumsy, 300);
    const EffectState effect_state{};
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), effect_data, effect_state);

    // Should not be able to dodge anymore
    ASSERT_FALSE(EntityHelper::CanDodge(*red_entity));

    // Send another attack
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    // Effect packaged should have been received
    ASSERT_FALSE(EntityHelper::CanDodge(*red_entity));
    ASSERT_EQ(events_effect_package_dodged.size(), 0);
    ASSERT_EQ(events_effect_package_received.size(), 1);
    ASSERT_EQ(events_effect_package_received.at(0).sender_id, blue_entity->GetID());
    ASSERT_EQ(events_effect_package_received.at(0).receiver_id, red_entity->GetID());
}

TEST_F(AbilitySystemTest, EnergyPerAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kEnergyCost, 10000_fp)
        .Set(StatType::kCurrentEnergy, 50_fp)
        .Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kEnergyGainEfficiencyPercentage, 0_fp);

    const auto before_energy = blue_stats_component.GetCurrentEnergy();
    ASSERT_EQ(before_energy, 50_fp);

    // Skill deploy
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());
    ASSERT_GE(events_ability_activated.size(), 1);
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);

    // Should have gained energy for attack ability + 5 premitigated damage
    ASSERT_EQ(
        blue_stats_component.GetCurrentEnergy(),
        Math::CalculateEnergyGain(
            before_energy + world->GetBaseEnergyGainPerAttack(),
            red_stats_component.GetBaseValueForType(StatType::kAttackSpeed)) +
            5_fp);
}

TEST_F(AbilitySystemTestWithStartOmegaInstantNoDamage, EnergyPerOmegaOnActivation)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kOnActivationEnergy, 25_fp)
        .Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kEnergyGainEfficiencyPercentage, 0_fp);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), blue_stats_component.GetBaseValueForType(StatType::kEnergyCost));

    // Activate
    ASSERT_TRUE(TimeStepUntilEventAbilityActivated());
    ASSERT_GE(events_ability_activated.size(), 1);
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kOmega);

    // Should have gained on activation energy
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 25_fp);
}

TEST_F(AbilitySystemTestWithStartOmegaInstantNoDamage, EnergyRegeneration)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kOnActivationEnergy, 0_fp)
        .Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp)
        .Set(StatType::kEnergyRegeneration, 2_fp);

    blue_template_stats.Set(StatType::kOnActivationEnergy, 0_fp)
        .Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp)
        .Set(StatType::kEnergyRegeneration, 1_fp);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), blue_stats_component.GetBaseValueForType(StatType::kEnergyCost));

    // Activate
    ASSERT_TRUE(TimeStepUntilEventAbilityActivated());
    ASSERT_GE(events_ability_activated.size(), 1);
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kOmega);

    // Should have gained energy because of energy rengeneration
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);
    world->TimeStep();
    const FixedPoint expected_attack_energy = 10_fp;
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), expected_attack_energy + 1_fp);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), expected_attack_energy + 2_fp);
    world->TimeStep();
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), expected_attack_energy + 2_fp);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), expected_attack_energy + 4_fp);
}

TEST_F(AbilitySystemTest, AbilitiesEventsMinEnergyEfficiency)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kCurrentEnergy, 50_fp).Set(StatType::kEnergyGainEfficiencyPercentage, 0_fp);
    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kAttackDodgeChancePercentage, kMinPercentageFP);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 50_fp);

    TimeStepForNTimeSteps(3);

    // No energy should be gained since efficiency is 0
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 50_fp);
}

TEST_F(AbilitySystemTest, AbilitiesEventsSomeEnergyEfficiency)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kCurrentEnergy, 50_fp).Set(StatType::kEnergyGainEfficiencyPercentage, 75_fp);
    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kAttackDodgeChancePercentage, kMinPercentageFP);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 50_fp);

    TimeStepForNTimeSteps(3);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), "61.250"_fp);
}

TEST_F(AbilitySystemTest, AbilitiesEventsMaxEnergyEfficiency)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kCurrentEnergy, 50_fp).Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kAttackDodgeChancePercentage, kMinPercentageFP);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 50_fp);

    std::vector<event_data::ApplyEnergyGain> events_energy_gained;
    world->SubscribeToEvent(
        EventType::kApplyEnergyGain,
        [&events_energy_gained](const Event& event)
        {
            events_energy_gained.emplace_back(event.Get<event_data::ApplyEnergyGain>());
        });

    TimeStepForNTimeSteps(3);

    // 10 energy gained from attacking, 5 from damage(red does 100 damage so 100/20 = 5)
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 65_fp);

    // 2 events from red 2 from blue
    ASSERT_EQ(events_energy_gained.size(), 4);
}

TEST_F(AbilitySystemBaseTest, HealthAndEnergyRegeneration)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();

    // Stats
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kEnergyCost, 35_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);
    data.type_data.stats.Set(StatType::kHealthRegeneration, 50_fp);
    data.type_data.stats.Set(StatType::kEnergyRegeneration, 10_fp);

    // Add Entities
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    blue_stats_component.SetCurrentHealth(50_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kHealthGainEfficiencyPercentage, 100_fp);

    // Confirm start health/energy
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);
    world->TimeStep();
    // HealthRegen = 50, EnergyRegen =10
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 10_fp);
    world->TimeStep();
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 150_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 20_fp);
    world->TimeStep();
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 200_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 30_fp);
    world->TimeStep();
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 200_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 35_fp);
}

TEST_F(AbilitySystemTestWithStartAttackEveryTimeStep, InvulnerableEnergyTest)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Make entities not die
    SetHealthVeryHigh();

    // Make energy cost very high
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 10000_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 10000_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentEnergy, 0_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentEnergy, 0_fp);

    StatsData blue_live_stats = world->GetLiveStats(blue_entity->GetID());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 10000_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    // Add invulnerability
    const auto invulnerable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 8000);
    EffectState invulnerable_effect_state{};
    invulnerable_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), invulnerable_effect_data, invulnerable_effect_state);

    for (int i = 0; i != 3; ++i)
    {
        const int second = i + 1;

        // ith second
        TimeStepForNTimeSteps(kTimeStepsPerSecond);
        ASSERT_EQ(events_ability_activated.size(), kTimeStepsPerSecond * 2) << "Should have only for every time step "
                                                                            << "second = " << second;
        ;
        ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack) << "second = " << second;
        ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack) << "second = " << second;
        events_ability_activated.clear();

        // blue should gain energy,
        // red should gain energy only from own attacks
        // red should not lose health due to invulnerable
        blue_live_stats = world->GetLiveStats(blue_entity->GetID());
        ASSERT_EQ(red_stats_component.GetCurrentHealth(), 10000_fp) << "second = " << second;
        ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), FixedPoint::FromInt(60 * second)) << "second = " << second;
        ASSERT_EQ(red_stats_component.GetCurrentEnergy(), FixedPoint::FromInt(10 * second)) << "second = " << second;
    }
}

TEST_F(AbilitySystemTestWithStartAttackEveryTimeStep, AttackNegativeStateStun)
{
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // Stun for 3 time steps
    Stun(*red_entity, 300);
    Stun(*blue_entity, 300);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    for (int i = 0; i < 3; i++)
    {
        events_ability_activated.clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire
        ASSERT_EQ(events_ability_activated.size(), 0) << "i = " << i;

        // Should not be able to activate attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    }
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // TimeStep again, eliminate the negative effect
    world->TimeStep();
    ASSERT_EQ(events_ability_activated.size(), 0);
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // Should have fired
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    ASSERT_EQ(events_ability_activated.size(), 2);
    for (const auto& event_data : events_ability_activated)
    {
        ASSERT_EQ(event_data.ability_type, AbilityType::kAttack);
    }
}

TEST_F(AbilitySystemTest, AttackAbilityWithBlind)
{
    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Add Entities
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);

    // Call function with maximum hit chance and critical chance
    events_effect_package_missed.clear();
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    ASSERT_EQ(events_effect_package_received.size(), 1) << "Attack should have hit";
    ASSERT_EQ(events_effect_package_missed.size(), 0) << "Attack should have NOT missed";

    // Add blind attached effect to blue entity
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 10);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    events_effect_package_received.clear();
    events_effect_package_missed.clear();

    // Try to activate attack while blind
    ability_system.ApplyEffectPackage(
        *blue_entity,
        attack_ability,
        attack_ability->skills.at(0).data->effect_package,
        false,
        red_entity->GetID());

    ASSERT_EQ(events_effect_package_received.size(), 0) << "Attack should have NOT hit";
    ASSERT_EQ(events_effect_package_missed.size(), 1) << "Attack should have missed";
}

TEST_F(AbilitySystemTestWithStartAttackEveryTimeStep, AttackNegativeStateDisarm)
{
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    // Blind for 3 time steps
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kDisarm, 300);

    // Red
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), effect_data, effect_state);
    }

    // Blue
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity->GetID(), effect_data, effect_state);
    }

    for (int i = 0; i < 3; i++)
    {
        events_ability_activated.clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire
        ASSERT_EQ(events_ability_activated.size(), 0) << "i = " << i;

        // Should not be able to attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    }

    events_ability_activated.clear();

    // TimeStep again, eliminate the negative effect
    world->TimeStep();
    ASSERT_EQ(events_ability_activated.size(), 0);
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // Attacks should still activate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    ASSERT_EQ(events_ability_activated.size(), 2);
}

TEST_F(AbilitySystemTestWithStartOmegaInstant, OmegaAbilityNegativeStateSilenced)
{
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    // Silenced for 3 time steps
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kSilenced, 300);

    // Red
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), effect_data, effect_state);
    }

    // Blue
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity->GetID(), effect_data, effect_state);
    }

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    events_ability_activated.clear();

    // TimeStep the world
    world->TimeStep();

    // Should attack each other
    ASSERT_EQ(events_ability_activated.size(), 2);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    for (int i = 0; i < 2; i++)
    {
        events_ability_activated.clear();

        // TimeStep the world (2 time steps)
        world->TimeStep();

        // Should not fire
        ASSERT_EQ(events_ability_activated.size(), 0) << "i = " << i;

        // Should not be able to attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));
    }

    // TimeStep again, eliminate the negative effect
    world->TimeStep();
    ASSERT_EQ(events_ability_activated.size(), 0);
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    // Should have fire
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));
}

TEST_F(AbilitySystemTestWithStartOmegaInstant, OmegaAbilityNegativeStateStun)
{
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // Stun for 3 time steps
    Stun(*red_entity, 300);
    Stun(*blue_entity, 300);

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    for (int i = 0; i < 3; i++)
    {
        events_ability_activated.clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire
        ASSERT_EQ(events_ability_activated.size(), 0) << "i = " << i;

        // Should not be able to attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

        // Should not be able to attack ability
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));
    }
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // TimeStep again, eliminate the negative effect
    world->TimeStep();
    ASSERT_EQ(events_ability_activated.size(), 0);
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    // Should have fired
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kAttack));

    ASSERT_EQ(events_ability_activated.size(), 2);
    for (const auto& event_data : events_ability_activated)
    {
        ASSERT_EQ(event_data.ability_type, AbilityType::kOmega);
    }
}

TEST_F(AbilitySystemTestWithStartOmegaPositiveStateUntargetable, OmegaPositiveStateUntargetable)
{
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*blue_entity, AbilityType::kOmega));

    world->TimeStep();
    ASSERT_EQ(events_ability_activated.size(), 2);
    ASSERT_EQ(events_ability_deactivated.size(), 2);
    ASSERT_EQ(events_ability_interrupted.size(), 1);
    ASSERT_EQ(events_skill_deployed.size(), 3);

    events_ability_activated.clear();
    events_ability_deactivated.clear();
    events_ability_interrupted.clear();
    events_skill_deployed.clear();

    // Nothing happens, both still Untargetable
    world->TimeStep();
    ASSERT_EQ(events_ability_activated.size(), 0);
    ASSERT_EQ(events_ability_deactivated.size(), 0);
    ASSERT_EQ(events_ability_interrupted.size(), 0);
    ASSERT_EQ(events_skill_deployed.size(), 0);
}

TEST_F(ScaledEnergyGainTest, EnergyScale)
{
    // Initialize the abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCombatStatCheck;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.targeting.stat_type = StatType::kMaxHealth;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.lowest = true;
        skill1.targeting.num = 3;
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    StatsData blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    StatsData red_live_stats = world->GetLiveStats(red_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kCurrentEnergy), 0_fp);
    ASSERT_EQ(red_live_stats.Get(StatType::kCurrentHealth), 100_fp * world_scale);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), data.type_data.stats.Get(StatType::kStartingEnergy));

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    // 1:1 this is 10 energy gain, due to 1000:1 scale on world it is 10,000
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 10_fp * world_scale);
    ASSERT_EQ(events_effect_package_received.size(), 1) << "skill should have fire";
    ASSERT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTest, AbilitiesEventsFireThorns)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP);

    // Set hit chance to min to show thorns will proc even if the entity does not attack
    blue_template_stats.Set(StatType::kMaxHealth, 300_fp)
        .Set(StatType::kCurrentHealth, 300_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP)
        .Set(StatType::kHitChancePercentage, kMinPercentageFP)
        .Set(StatType::kThorns, 100_fp);

    // Listen to health changed
    std::vector<event_data::StatChanged> events_health_changed;
    world->SubscribeToEvent(
        EventType::kHealthChanged,
        [&events_health_changed](const Event& event)
        {
            events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Deploy skill
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // 1 event from attack, 1 from thorns
    ASSERT_EQ(2, events_health_changed.size()) << "Health changed events not fired on blue and red";

    // Health should not have been reduced by attack ability
    // But should receive 100 damage from thorns
    ASSERT_EQ(400_fp, red_stats_component.GetCurrentHealth());

    // 100 damage done to blue from ability
    ASSERT_EQ(200_fp, blue_stats_component.GetCurrentHealth());
}

TEST_F(AbilitySystemTest, AbilitiesEventsFireThornsBoth)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // Set thorns for both entities
    red_template_stats.Set(StatType::kMaxHealth, 500_fp)
        .Set(StatType::kCurrentHealth, 500_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP)
        .Set(StatType::kThorns, 100_fp);

    blue_template_stats.Set(StatType::kMaxHealth, 300_fp)
        .Set(StatType::kCurrentHealth, 300_fp)
        .Set(StatType::kThorns, 100_fp)
        .Set(StatType::kHitChancePercentage, kMaxPercentageFP);

    // Listen to health changed
    std::vector<event_data::StatChanged> events_health_changed;
    world->SubscribeToEvent(
        EventType::kHealthChanged,
        [&events_health_changed](const Event& event)
        {
            events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Deploy skill
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // 2 events from attacks, 2 from thorns
    ASSERT_EQ(4, events_health_changed.size()) << "Health changed events not fired on blue and red";

    // 100 damage from attack ability, 100 damage from thorns
    ASSERT_EQ(300_fp, red_stats_component.GetCurrentHealth());

    // 100 damage from attack ability, 100 damage from thorns
    ASSERT_EQ(100_fp, blue_stats_component.GetCurrentHealth());
}

TEST_F(AbilitySystemTestWithAllAttackTypes, ApplyAllAttackDamageTypes)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99700_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99400_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99100_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 98800_fp);
}

TEST_F(AbilitySystemTestWithAllAttackTypes, ApplyAllAttackDamageTypesExceptEnergy)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kAttackEnergyDamage, 0_fp);

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99800_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99600_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99400_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99200_fp);
}

TEST_F(AbilitySystemTestWithPurestAttackType, PurestAttack)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Fire basic attack
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99600_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99200_fp);
}

TEST_F(AbilitySystemTestWithPurestAttackType, PurestAttackToShieldedEntity)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    {
        ShieldData shield_data;
        shield_data.sender_id = red_entity->GetID();
        shield_data.receiver_id = red_entity->GetID();
        shield_data.duration_ms = kTimeInfinite;
        shield_data.value = 600_fp;
        EntityFactory::SpawnShieldAndAttach(*world, red_entity->GetTeam(), shield_data);
    }

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 100'000_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Fire basic attack
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99'900_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99'800_fp);
}

TEST_F(AbilitySystemTestWithPurestAttackType, PurestAttackToInvulnerableEntity)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Add invulnerability
    const auto invulnerable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 80000);
    EffectState invulnerable_effect_state{};
    invulnerable_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), invulnerable_effect_data, invulnerable_effect_state);

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Fire basic attack
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99900_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 99800_fp);
}

TEST_F(AbilitySystemTestWithInterrupt, InterruptAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    blue_stats_component.SetHitChanceOverflowPercentage(kMaxPercentageFP);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 100000_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    Stun(*red_entity, 100);

    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 99800_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 99600_fp);
    EXPECT_EQ(events_ability_interrupted.size(), 1);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Attack ability interrupted so no damage occured
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 99600_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Red is now stunned so no damage again
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 99400_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Skill 2 fires for 200 damage
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 99400_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 99200_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 99200_fp);
}

TEST_F(AbilitySystemTestWithOmegaRange, OmegaRangeTestNoFire)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    // Make entities not die
    SetHealthVeryHigh();
    blue_stats_component.SetCurrentEnergy(blue_stats_component.GetEnergyCost());

    StatsData live_stats = world->GetLiveStats(blue_entity->GetID());

    // omega range should be five, it is set as 1 but is updated to 5 to match attack_range
    EXPECT_EQ(live_stats.Get(StatType::kOmegaRangeUnits), 5_fp);

    // 1s
    TimeStepForSeconds(1);
    ASSERT_EQ(events_ability_activated.size(), 0) << "Should not attack, should not have omega since out of range";
    events_ability_activated.clear();
}

TEST_F(AbilitySystemTestWithOmegaRange, OmegaRangeTestFire)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    // Make entities not die
    SetHealthVeryHigh();

    // Set range so omega can reach other entity, movement speed is 0
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 30_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kOmegaRangeUnits, 25_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kOmegaRangeUnits, 30_fp);

    blue_stats_component.SetCurrentEnergy(blue_stats_component.GetEnergyCost());

    // 1s
    TimeStepForSeconds(1);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have fired once for blue";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kOmega);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    events_ability_activated.clear();
}

TEST_F(AbilitySystemTest, AttributesForAbilityActivation_Ally)
{
    // Make red ally
    Entity* ally = nullptr;
    SpawnCombatUnit(Team::kBlue, {5, 5}, data, ally);

    AbilityActivationTriggerData ability_trigger_attributes;
    ability_trigger_attributes.ability_types = MakeEnumSet(AbilityType::kAttack);
    ability_trigger_attributes.sender_allegiance = AllegianceType::kAlly;
    ability_trigger_attributes.range_units = 20;
    ability_trigger_attributes.not_before_ms = 200;
    ability_trigger_attributes.not_after_ms = 400;

    AbilityData ability_data;
    ability_data.activation_trigger_data = ability_trigger_attributes;

    AbilityState ability_state;
    ability_state.data = std::make_shared<AbilityData>(ability_data);

    AbilityStateActivatorContext context;
    context.sender_entity_id = ally->GetID();
    context.sender_combat_unit_entity_id = ally->GetID();
    context.ability_type = AbilityType::kAttack;
    ability_state.activator_contexts.push_back(context);

    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    TimeStepForMs(300);

    const bool can_activate_ability_by_attributes =
        ability_system.CanActivateAbilityForTriggerData(*blue_entity, ability_state, world->GetTimeStepCount());
    ASSERT_TRUE(can_activate_ability_by_attributes);
}

TEST_F(AbilitySystemTest, AttributesForAbilityActivation_Focus)
{
    AbilityActivationTriggerData ability_trigger_attributes;
    ability_trigger_attributes.ability_types = MakeEnumSet(AbilityType::kAttack);
    ability_trigger_attributes.range_units = 20;
    ability_trigger_attributes.not_before_ms = 200;
    ability_trigger_attributes.not_after_ms = 400;
    ability_trigger_attributes.only_focus = true;

    AbilityData ability_data;
    ability_data.activation_trigger_data = ability_trigger_attributes;

    AbilityState ability_state;
    ability_state.data = std::make_shared<AbilityData>(ability_data);

    AbilityStateActivatorContext context;
    context.sender_entity_id = red_entity->GetID();
    context.sender_combat_unit_entity_id = red_entity->GetID();
    context.ability_type = AbilityType::kAttack;
    ability_state.activator_contexts.push_back(context);

    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    TimeStepForMs(300);

    const bool can_activate_ability_by_attributes =
        ability_system.CanActivateAbilityForTriggerData(*blue_entity, ability_state, world->GetTimeStepCount());
    ASSERT_TRUE(can_activate_ability_by_attributes);
}

TEST_F(AbilitySystemTestOmegaAfterFirstAttack, ActivateOmegaWhenTargetUntargerable)
{
    const auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    const auto& red_stats_component = red_entity->Get<StatsComponent>();

    world->TimeStep();

    // blue activate attack, red activate omega
    ASSERT_EQ(events_ability_activated.size(), 2);
    ASSERT_EQ(events_ability_activated.at(0).sender_id, blue_entity->GetID());
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).sender_id, red_entity->GetID());
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kOmega);
    events_ability_activated.clear();

    // Red just casted omega and spend all energy
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);

    // Red become untargetable
    ASSERT_FALSE(EntityHelper::IsTargetable(*red_entity));

    // Blue has full energy
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), blue_stats_component.GetEnergyCost());

    world->TimeStep();

    // Blue activate omega
    ASSERT_EQ(events_ability_activated.size(), 1);
    ASSERT_EQ(events_ability_activated.at(0).sender_id, blue_entity->GetID());
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kOmega);

    // Blue just casted omega and spend all energy
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);
}

TEST_F(AbilitySystemTestOmegaAfterFirstAttack, ApplyUntargetable)
{
    const auto& focus_component = blue_entity->Get<FocusComponent>();

    world->TimeStep();

    // Blue focus on red
    ASSERT_EQ(focus_component.GetFocusID(), red_entity->GetID());

    // Blue activates attack, red activates omega
    ASSERT_EQ(events_ability_activated.size(), 2);
    ASSERT_EQ(events_ability_activated[0].sender_id, blue_entity->GetID());
    ASSERT_EQ(events_ability_activated[0].ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated[1].sender_id, red_entity->GetID());
    ASSERT_EQ(events_ability_activated[1].ability_type, AbilityType::kOmega);

    // Red becomes untargetable
    ASSERT_FALSE(EntityHelper::IsTargetable(*red_entity));

    events_ability_activated.clear();
    events_ability_deactivated.clear();
    world->TimeStep();

    // Blue focuses untargetable red, interrupts (deactivates) attack and activates omega
    ASSERT_EQ(events_ability_deactivated.size(), 1);
    EXPECT_EQ(events_ability_deactivated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_deactivated[0].ability_type, AbilityType::kAttack);

    ASSERT_EQ(focus_component.GetFocusID(), red_entity->GetID());
    ASSERT_EQ(events_ability_activated.size(), 1);
    EXPECT_EQ(events_ability_activated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_activated[0].ability_type, AbilityType::kOmega);

    events_ability_activated.clear();
    events_ability_deactivated.clear();
    TimeStepForNTimeSteps(6);

    // Blue deactivated omega and doesn't activate anything else
    ASSERT_EQ(events_ability_deactivated.size(), 1);
    EXPECT_EQ(events_ability_deactivated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_deactivated[0].ability_type, AbilityType::kOmega);
    ASSERT_EQ(events_ability_activated.size(), 0);

    // Blue focus now clear
    EXPECT_EQ(focus_component.GetFocusID(), kInvalidEntityID);
}

TEST_F(AbilitySystemTestOmegaAfterFirstAttack, ApplyUntargetableWithExtra)
{
    const auto& focus_component = blue_entity->Get<FocusComponent>();

    // Add an extra entity
    Entity* extra_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {30, 30}, data, extra_entity);

    world->TimeStep();

    // Blue activates attack, red activates omega
    ASSERT_EQ(events_ability_activated.size(), 2);
    ASSERT_EQ(events_ability_activated[0].sender_id, blue_entity->GetID());
    ASSERT_EQ(events_ability_activated[0].ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated[1].sender_id, red_entity->GetID());
    ASSERT_EQ(events_ability_activated[1].ability_type, AbilityType::kOmega);

    // Red becomes untargetable
    ASSERT_FALSE(EntityHelper::IsTargetable(*red_entity));

    events_ability_activated.clear();
    events_ability_deactivated.clear();
    world->TimeStep();

    // Blue focuses untargetable red, interrupts (deactivates) attack and activates omega
    ASSERT_EQ(events_ability_deactivated.size(), 1);
    EXPECT_EQ(events_ability_deactivated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_deactivated[0].ability_type, AbilityType::kAttack);

    ASSERT_EQ(focus_component.GetFocusID(), red_entity->GetID());
    ASSERT_EQ(events_ability_activated.size(), 1);
    EXPECT_EQ(events_ability_activated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_activated[0].ability_type, AbilityType::kOmega);

    events_ability_activated.clear();
    events_ability_deactivated.clear();
    TimeStepForNTimeSteps(6);

    // Blue deactivated omega and doesn't activate anything else
    ASSERT_EQ(events_ability_deactivated.size(), 1);
    EXPECT_EQ(events_ability_deactivated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_deactivated[0].ability_type, AbilityType::kOmega);
    ASSERT_EQ(events_ability_activated.size(), 0);

    // Blue focus now on extra entity
    EXPECT_EQ(focus_component.GetFocusID(), extra_entity->GetID());
}

TEST_F(AbilitySystemTestOmegaAfterFirstAttackWithSlowAbility, ApplyUntargetableWithExtraDelayed)
{
    const auto& focus_component = blue_entity->Get<FocusComponent>();

    // Add an extra entity
    Entity* extra_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {30, 30}, data, extra_entity);

    world->TimeStep();

    // Blue activates attack, red activates omega
    ASSERT_EQ(events_ability_activated.size(), 2);
    ASSERT_EQ(events_ability_activated[0].sender_id, blue_entity->GetID());
    ASSERT_EQ(events_ability_activated[0].ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated[1].sender_id, red_entity->GetID());
    ASSERT_EQ(events_ability_activated[1].ability_type, AbilityType::kOmega);

    // Red becomes untargetable
    ASSERT_FALSE(EntityHelper::IsTargetable(*red_entity));

    events_ability_activated.clear();
    events_ability_deactivated.clear();
    world->TimeStep();

    // Blue focuses untargetable red, interrupts (deactivates) attack and activates omega
    ASSERT_EQ(events_ability_deactivated.size(), 1);
    EXPECT_EQ(events_ability_deactivated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_deactivated[0].ability_type, AbilityType::kAttack);

    ASSERT_EQ(focus_component.GetFocusID(), red_entity->GetID());
    ASSERT_EQ(events_ability_activated.size(), 1);
    EXPECT_EQ(events_ability_activated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_activated[0].ability_type, AbilityType::kOmega);

    events_ability_activated.clear();
    events_ability_deactivated.clear();
    TimeStepForNTimeSteps(6);

    // Blue deactivated omega and doesn't activate anything else
    ASSERT_EQ(events_ability_deactivated.size(), 1);
    EXPECT_EQ(events_ability_deactivated[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(events_ability_deactivated[0].ability_type, AbilityType::kOmega);
    ASSERT_EQ(events_ability_activated.size(), 0);

    // Blue focus now on extra entity
    EXPECT_EQ(focus_component.GetFocusID(), extra_entity->GetID());
}

TEST_F(AbilitySystemTestWithStartAttackEvery1SecondCooldown, TryToAttackEverySecond)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    // Make entities not die
    SetHealthVeryHigh();

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    for (int i = 0; i < 25; i++)
    {
        world->TimeStep();

        // Attack speed is 100 so attack should occur every 1 second,
        // Since attack cooldown is 20 so the attack will only go through every 2 seconds after the first attack
        if (i % 20 == 0 || i == 0)
        {
            ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities " << i;
            ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
            ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
            events_ability_activated.clear();
        }
        else
        {
            ASSERT_EQ(events_ability_activated.size(), 0) << "Should not for either entities" << i;
        }
    }
}

TEST_F(AbilitySystemTestWithStartOmegaOneSkill, ScaleOmegaDamage)
{
    const auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    const EntityID blue_entity_id = blue_entity->GetID();
    const auto& red_stats_component = red_entity->Get<StatsComponent>();
    const EntityID red_entity_id = red_entity->GetID();

    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    // Add buff to blue entity
    const auto buff_effect_data =
        EffectData::CreateBuff(StatType::kOmegaDamagePercentage, EffectExpression::FromValue(50_fp), kTimeInfinite);
    EffectState buff_effect_state{};
    buff_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, buff_effect_data, buff_effect_state);

    // Add debuff to red entity
    const auto debuff_effect_data =
        EffectData::CreateDebuff(StatType::kOmegaDamagePercentage, EffectExpression::FromValue(50_fp), kTimeInfinite);
    EffectState debuff_effect_state{};
    debuff_effect_state.sender_stats = world->GetFullStats(red_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, debuff_effect_data, debuff_effect_state);

    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_apply_effect.size(), 2);
    ASSERT_EQ(events_apply_effect.at(0).state.source_context.combat_unit_ability_type, AbilityType::kOmega);
    ASSERT_EQ(events_apply_effect.at(1).state.source_context.combat_unit_ability_type, AbilityType::kOmega);
    events_apply_effect.clear();

    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 900_fp);  // 1000 hp - 50% of 200 dmg
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 700_fp);   // 1000 hp - 150% of 200 dmg
}

TEST_F(AbilitySystemTest, VerifyAttackContext)
{
    TimeStepForSeconds(1);

    ASSERT_EQ(events_effect_package_received.size(), 2) << "Attack should have fired";
    ASSERT_EQ(events_effect_package_received[0].source_context.ability_name, "Foo");
    ASSERT_EQ(events_effect_package_received[0].source_context.skill_name, "Attack");
    ASSERT_EQ(events_effect_package_received[1].source_context.ability_name, "Foo");
    ASSERT_EQ(events_effect_package_received[1].source_context.skill_name, "Attack");
}

TEST_F(AbilitySystemBaseTest, AttackRooted)
{
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
    data.type_data.stats.Set(StatType::kCurrentHealth, 1000_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 1000_fp);
    data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

    // Add attack ability
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        ability.total_duration_ms = 100;

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(1_fp));
        skill1.AddEffect(EffectData::CreateNegativeState(EffectNegativeState::kRoot, kTimeInfinite));
    }

    auto apply_root = [&](Entity* entity, const int duration = kTimeInfinite)
    {
        const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, duration);

        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(entity->GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(*entity, entity->GetID(), effect_data, effect_state);
        ASSERT_TRUE(entity->Get<AttachedEffectsComponent>().HasNegativeState(EffectNegativeState::kRoot));
    };

    // Add Entities
    Entity *blue_entity = nullptr, *red_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

    // Root both entities
    apply_root(blue_entity);
    apply_root(red_entity);

    EventHistory<EventType::kOnDamage> on_damage(*world);
    for (size_t i = 1; i != 10; ++i)
    {
        world->TimeStep();
        ASSERT_EQ(on_damage.events.size(), i * 2);
    }
}

TEST_F(AbilitySystemBaseTest, TruesightAndTrueshot)
{
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
    data.type_data.stats.Set(StatType::kCurrentHealth, 1000_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 1000_fp);
    data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

    const auto add_effect = [&](const Entity* entity, const EffectData& effect_data)
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(*entity);
        GetAttachedEffectsHelper().AddAttachedEffect(*entity, entity->GetID(), effect_data, effect_state);
    };

    // Add attack ability
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(1_fp));
        skill1.effect_package.attributes.use_hit_chance = true;
    }

    // Blue entity has 100% dodge change and just does attacks
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
    add_effect(
        blue_entity,
        EffectData::CreateBuff(
            StatType::kAttackDodgeChancePercentage,
            EffectExpression::FromValue(kMaxPercentageFP),
            -1));
    ASSERT_EQ(kMaxPercentageFP, world->GetLiveStats(*blue_entity).Get(StatType::kAttackDodgeChancePercentage));

    // Red entity is blinded but has truesight
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
    add_effect(red_entity, EffectData::CreateNegativeState(EffectNegativeState::kBlind, -1));
    ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));
    add_effect(red_entity, EffectData::CreatePositiveState(EffectPositiveState::kTruesight, -1));
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

    EventHistory<EventType::kEffectPackageReceived> on_effect_package(*world);
    EventHistory<EventType::kOnDamage> on_damage(*world);
    EventHistory<EventType::kEffectPackageDodged> on_dodge(*world);
    EventHistory<EventType::kEffectPackageBlocked> on_block(*world);
    EventHistory<EventType::kEffectPackageMissed> on_miss(*world);

    world->TimeStep();

    ASSERT_TRUE(on_dodge.IsEmpty());
    ASSERT_TRUE(on_block.IsEmpty());
    ASSERT_TRUE(on_miss.IsEmpty());

    const auto blue_first = [&](const auto& a, const auto& b)
    {
        return world->GetEntityTeam(a.sender_id) < world->GetEntityTeam(b.sender_id);
    };

    ASSERT_EQ(on_effect_package.Size(), 2);
    std::sort(on_effect_package.events.begin(), on_effect_package.events.end(), blue_first);
    ASSERT_EQ(on_effect_package[0].sender_id, blue_entity->GetID());
    ASSERT_EQ(on_effect_package[1].sender_id, red_entity->GetID());

    ASSERT_EQ(on_damage.Size(), 2);
    std::sort(on_damage.events.begin(), on_damage.events.end(), blue_first);
    ASSERT_EQ(on_damage[0].sender_id, blue_entity->GetID());
    ASSERT_EQ(on_damage[1].sender_id, red_entity->GetID());
}

}  // namespace simulation
