#pragma once

#include "base_test_fixtures.h"
#include "components/stats_component.h"
#include "data/combat_unit_data.h"
#include "data/enums.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "gtest/gtest.h"
#include "utility/entity_helper.h"

namespace simulation
{
class AttachedEffectsSystemTest : public BaseTest
{
    typedef BaseTest Super;

public:
    // Simulate effect application using event
    void TryToApplyEffect(EntityID sender_id, EntityID receiver_id, const EffectData& effect_data) const
    {
        // Sender and receiver should be a valid combat units
        if (!EntityHelper::IsACombatUnit(*world, sender_id) && !EntityHelper::IsACombatUnit(*world, receiver_id))
        {
            assert(false);
        }

        EffectState effect_state;
        effect_state.sender_stats = world->GetFullStats(sender_id);
        effect_state.source_context.combat_unit_ability_type = AbilityType::kOmega;

        // Effect should be simulated through event to work properly
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(sender_id, receiver_id, effect_data, effect_state);
    }

protected:
    virtual void InitStats()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kEnergyCost, 10000_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        // Stop moving for most of the tests
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, GetMoveSpeedSubUnits());
        data.type_data.stats.Set(StatType::kHealthGainEfficiencyPercentage, 100_fp);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 10_fp);

        data.type_data.stats.Set(StatType::kEnergyRegeneration, 1_fp);
        data.type_data.stats.Set(StatType::kOnActivationEnergy, 2_fp);
    }

    virtual void InitAttackAbilities()
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void InitCombatUnitData()
    {
        InitStats();
        InitAttackAbilities();

        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-20, -40}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {25, 50}, data, red_entity);

        blue_entity_id = blue_entity->GetID();
        red_entity_id = red_entity->GetID();
    }

    virtual FixedPoint GetMoveSpeedSubUnits() const
    {
        return 0_fp;
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();

        // Listen to the apply effect events
        world->SubscribeToEvent(
            EventType::kTryToApplyEffect,
            [this](const Event& event)
            {
                events_apply_effect.emplace_back(event.Get<event_data::Effect>());
            });

        // Listen to the remove effect events
        world->SubscribeToEvent(
            EventType::kOnAttachedEffectRemoved,
            [this](const Event& event)
            {
                events_remove_effect.emplace_back(event.Get<event_data::Effect>());
            });

        // Listen to the blocked effects
        world->SubscribeToEvent(
            EventType::kEffectPackageBlocked,
            [this](const Event& event)
            {
                events_effect_package_blocked.emplace_back(event.Get<event_data::EffectPackage>());
            });
    }

    EntityID blue_entity_id = kInvalidEntityID;
    EntityID red_entity_id = kInvalidEntityID;
    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    CombatUnitData data;

    std::vector<event_data::Effect> events_apply_effect;
    std::vector<event_data::Effect> events_remove_effect;

    std::vector<event_data::EffectPackage> events_effect_package_blocked;
};

class AttachedEffectsSystemWithMovementTest : public AttachedEffectsSystemTest
{
    typedef AttachedEffectsSystemTest Super;

protected:
    FixedPoint GetMoveSpeedSubUnits() const override
    {
        return 5000_fp;
    }
};

class AttachedEffect_OverlapTest : public AttachedEffectsSystemTest
{
    typedef AttachedEffectsSystemTest Super;
};

class AttachedEffect_SameDebuff_OverlapTest : public AttachedEffect_OverlapTest
{
    typedef AttachedEffect_OverlapTest Super;

protected:
    void RunTest(
        const std::string& effect1_ability_name,
        const std::string& effect2_ability_name,
        const FixedPoint expected_value)
    {
        //// Get the relevant components
        auto& red_stats_component = red_entity->Get<StatsComponent>();
        const int duration_buff_ms = 300;

        // set sane values
        red_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 0_fp);
        red_stats_component.SetCurrentHealth(100_fp);

        // Calculate live stats
        StatsData red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 0_fp);
        EXPECT_EQ(red_live_stats.Get(StatType::kWillpowerPercentage), 0_fp);
        ASSERT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 100_fp);

        // Add debuff
        {
            auto effect_data =
                EffectData::CreateDebuff(StatType::kAttackSpeed, EffectExpression::FromValue(10_fp), duration_buff_ms);
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(blue_entity_id);
            effect_state.source_context.ability_name = effect1_ability_name;

            // Add the effect
            GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
        }

        // Should compute to the current stats even before the TimeStep
        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 90_fp);

        {
            const auto priority_effect_data =
                EffectData::CreateDebuff(StatType::kAttackSpeed, EffectExpression::FromValue(60_fp), duration_buff_ms);

            EffectState priority_effect_state{};
            priority_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
            priority_effect_state.source_context.ability_name = effect2_ability_name;

            // Add the priority effect which should take over
            GetAttachedEffectsHelper()
                .AddAttachedEffect(*red_entity, blue_entity_id, priority_effect_data, priority_effect_state);
        }

        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), expected_value);

        for (int i = 0; i < 4; i++)
        {
            red_live_stats = world->GetLiveStats(red_entity_id);
            EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), expected_value) << "i = " << i;
            world->TimeStep();
        }

        // Should have fired end event
        ASSERT_EQ(events_remove_effect.size(), 2);
        ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);

        // Should no longer be active
        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 100_fp);
    }
};

class AttachedEffect_SameDebuff_DifferentEntities_OverlapTest : public AttachedEffect_OverlapTest
{
    typedef AttachedEffect_OverlapTest Super;

protected:
    void RunTest(
        const std::string& effect1_ability_name,
        const std::string& effect2_ability_name,
        const FixedPoint expected_value)
    {
        //// Get the relevant components
        auto& red_stats_component = red_entity->Get<StatsComponent>();

        // set sane values
        red_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPureDamage, 0_fp);
        red_stats_component.GetMutableTemplateStats().Set(StatType::kVulnerabilityPercentage, 200_fp);
        red_stats_component.SetCurrentHealth(100_fp);

        // Calculate live stats
        StatsData red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 200_fp);
        EXPECT_EQ(red_live_stats.Get(StatType::kWillpowerPercentage), 0_fp);

        // Add debuff
        {
            const auto effect_data =
                EffectData::CreateDebuff(StatType::kVulnerabilityPercentage, EffectExpression::FromValue(100_fp), 300);
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(blue_entity_id);
            effect_state.source_context.ability_name = effect1_ability_name;

            // Add the effect
            GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
        }

        // Should compute to the current stats even before the TimeStep
        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 300_fp);

        {
            const auto effect_data =
                EffectData::CreateDebuff(StatType::kVulnerabilityPercentage, EffectExpression::FromValue(80_fp), 300);
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(blue_entity2_id);
            effect_state.source_context.ability_name = effect2_ability_name;

            // Add the second effect which should fire since it's from a new entity
            GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity2_id, effect_data, effect_state);
        }

        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), expected_value);

        for (int i = 0; i < 3; i++)
        {
            red_live_stats = world->GetLiveStats(red_entity_id);
            EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), expected_value);
            world->TimeStep();
        }

        // Second effect only should be active now
        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), expected_value);
        world->TimeStep();

        // Should have fired end event
        ASSERT_EQ(events_remove_effect.size(), 2);
        ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);

        // Should no longer be active
        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 200_fp);
    }

    void InitCombatUnitData() override
    {
        Super::InitCombatUnitData();

        // Spawn second blue combat unit
        SpawnCombatUnit(Team::kBlue, {5, 5}, data, blue_entity2);
        blue_entity2_id = blue_entity2->GetID();
    }

    EntityID blue_entity2_id = kInvalidEntityID;
    Entity* blue_entity2 = nullptr;
};

class AttachedEffect_MultipleDOTs_OverlapTest : public AttachedEffect_OverlapTest
{
    typedef AttachedEffect_OverlapTest Super;

protected:
    virtual void RunTest(
        const std::string& effect1_ability_name,
        const std::string& effect2_ability_name,
        const FixedPoint expected_health,
        const FixedPoint expected_energy)
    {
        // Get the relevant components
        auto& red_stats_component = red_entity->Get<StatsComponent>();

        // Get the Stats Data
        auto& blue_stats_component = blue_entity->Get<StatsComponent>();

        // Set sane values
        red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 2000_fp);
        red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
        blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
        red_stats_component.SetCurrentHealth(2000_fp);
        red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 100_fp);

        // Update current stats
        ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);

        // Add an effect Wrap it in an attached effect
        {
            const auto effect_data = EffectData::CreateDamageOverTime(
                EffectDamageType::kPhysical,
                EffectExpression::FromValue(500_fp),
                1000,
                kDefaultAttachedEffectsFrequencyMs);
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(blue_entity_id);
            effect_state.source_context.ability_name = effect1_ability_name;

            GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
        }

        {
            const auto effect_data2 = EffectData::CreateDamageOverTime(
                EffectDamageType::kPhysical,
                EffectExpression::FromValue(600_fp),
                1000,
                kDefaultAttachedEffectsFrequencyMs);
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(blue_entity_id);
            effect_state.source_context.ability_name = effect2_ability_name;

            GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data2, effect_state);
        }

        auto red_live_stats = world->GetLiveStats(red_entity_id);
        auto blue_live_stats = world->GetLiveStats(blue_entity_id);

        for (int i = 0; i < 10; i++)
        {
            // time step the world
            world->TimeStep();
        }

        red_live_stats = world->GetLiveStats(red_entity_id);
        blue_live_stats = world->GetLiveStats(blue_entity_id);

        EXPECT_EQ(red_stats_component.GetCurrentHealth(), expected_health);
        EXPECT_EQ(red_stats_component.GetCurrentEnergy(), expected_energy);
    }
};

class AttachedEffectsSystemWithAttackEveryTimeStep : public AttachedEffectsSystemTest
{
    typedef AttachedEffectsSystemTest Super;

protected:
    void InitStats() override
    {
        Super::InitStats();

        // Add attack ability
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";
            ability.total_duration_ms = 100;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

            skill1.deployment.pre_deployment_delay_percentage = 0;
        }

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kEnergyResist, 0_fp);
        data.type_data.stats.Set(StatType::kPhysicalResist, 0_fp);
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        // No critical attacks
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
        // No Dodge
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        // No Miss
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);

        // Once every time step
        data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

        data.type_data.stats.Set(StatType::kResolve, 0_fp);
        data.type_data.stats.Set(StatType::kGrit, 0_fp);
        data.type_data.stats.Set(StatType::kWillpowerPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kTenacityPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 25_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

        data_low_health = data;
        data_low_health.type_data.stats.Set(StatType::kMaxHealth, 10_fp);
    }

    void InitCombatUnitData() override
    {
        InitStats();

        // Spawn tree combat units
        SpawnCombatUnit(Team::kRed, {10, 10}, data, red_entity);
        SpawnCombatUnit(Team::kBlue, {10, 15}, data_low_health, blue_entity);
        SpawnCombatUnit(Team::kBlue, {25, 25}, data, blue_entity2);

        red_entity_id = red_entity->GetID();
        blue_entity_id = blue_entity->GetID();
        blue_entity2_id = blue_entity2->GetID();
    }

    CombatUnitData data_low_health;
    EntityID blue_entity2_id = kInvalidEntityID;
    Entity* blue_entity2 = nullptr;
};

class AttachedEffectsSystemFirstBlueCloser : public AttachedEffectsSystemTest
{
    typedef AttachedEffectsSystemTest Super;

protected:
    virtual int AttackRange()
    {
        return 1;
    }

    void InitCombatUnitData() override
    {
        InitStats();

        // Add attack ability
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";
            ability.total_duration_ms = 1000;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

            skill1.deployment.pre_deployment_delay_percentage = 100;
        }

        data.type_data.stats.Set(StatType::kAttackRangeUnits, FixedPoint::FromInt(AttackRange()));
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);

        CombatUnitData blue_data = data;
        // blue entities should not move
        blue_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

        // Spawn tree combat units
        SpawnCombatUnit(Team::kRed, {10, 10}, data, red_entity);
        SpawnCombatUnit(Team::kBlue, {10, 5}, blue_data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {10, 55}, blue_data, blue_entity2);

        red_entity_id = red_entity->GetID();
        blue_entity_id = blue_entity->GetID();
        blue_entity2_id = blue_entity2->GetID();
    }

    EntityID blue_entity2_id = kInvalidEntityID;
    Entity* blue_entity2 = nullptr;
};

class AttachedEffectsSystemFirstBlueCloserRanged : public AttachedEffectsSystemFirstBlueCloser
{
    typedef AttachedEffectsSystemFirstBlueCloser Super;

protected:
    int AttackRange() override
    {
        return 60;
    }
};

}  // namespace simulation
