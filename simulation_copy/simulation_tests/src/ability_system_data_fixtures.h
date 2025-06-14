#pragma once

#include "base_test_fixtures.h"
#include "components/decision_component.h"
#include "components/stats_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "factories/entity_factory.h"
#include "gtest/gtest.h"
#include "systems/ability_system.h"
#include "utility/entity_helper.h"

namespace simulation
{
class AbilitySystemBaseTest : public BaseTest
{
    using Super = BaseTest;

protected:
    void SetUp() override
    {
        Super::SetUp();

        // Listen to events
        world->SubscribeToEvent(
            EventType::kAbilityActivated,
            [this](const Event& event)
            {
                events_ability_activated.emplace_back(event.Get<event_data::AbilityActivated>());
            });
        world->SubscribeToEvent(
            EventType::kAbilityDeactivated,
            [this](const Event& event)
            {
                events_ability_deactivated.emplace_back(event.Get<event_data::AbilityDeactivated>());
            });

        world->SubscribeToEvent(
            EventType::kEffectPackageReceived,
            [this](const Event& event)
            {
                events_effect_package_received.emplace_back(event.Get<event_data::EffectPackage>());
            });
        world->SubscribeToEvent(
            EventType::kEffectPackageMissed,
            [this](const Event& event)
            {
                events_effect_package_missed.emplace_back(event.Get<event_data::EffectPackage>());
            });
        world->SubscribeToEvent(
            EventType::kAbilityInterrupted,
            [this](const Event& event)
            {
                events_ability_interrupted.emplace_back(event.Get<event_data::AbilityInterrupted>());
            });
        world->SubscribeToEvent(
            EventType::kSkillDeploying,
            [this](const Event& event)
            {
                events_skill_deployed.emplace_back(event.Get<event_data::Skill>());
            });
    }

    static void ForcedActivateAbility(const Entity& entity, AbilitySystem& ability_system, AbilityStatePtr& ability)
    {
        // Try to perform targeting
        if (ability_system.TryInitSkillTargeting(entity, ability, true))
        {
            for (auto& skill : ability->skills)
            {
                ability_system.TargetAndDeploySkill(entity, ability, skill);
            }
        }
    }

protected:
    std::vector<event_data::EffectPackage> events_effect_package_missed;
    std::vector<event_data::EffectPackage> events_effect_package_received;
    std::vector<event_data::AbilityActivated> events_ability_activated;
    std::vector<event_data::AbilityDeactivated> events_ability_deactivated;
    std::vector<event_data::AbilityInterrupted> events_ability_interrupted;
    std::vector<event_data::Skill> events_skill_deployed;
};

class AbilitySystemTest : public AbilitySystemBaseTest
{
    typedef AbilitySystemBaseTest Super;

public:
    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;

protected:
    virtual bool UseInstantTimers() const
    {
        return false;
    }
    virtual bool UseDamage() const
    {
        return true;
    }

    virtual void InitCombatUnitData()
    {
        InitStats();
        InitAttackAbilities();
        InitOmegaAbilities();
        InitInnateAbilities();
    }

    virtual void InitStats()
    {
        data = CreateCombatUnitData();

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

        // Once every second
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);

        data.type_data.stats.Set(StatType::kResolve, 0_fp);
        data.type_data.stats.Set(StatType::kGrit, 0_fp);
        data.type_data.stats.Set(StatType::kWillpowerPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kTenacityPercentage, 0_fp);
        // 15^2 == 225
        // square distance between blue and red is 200
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 25_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    }

    virtual void InitAttackAbilities()
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            // 200 ms
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;

            skill1.SetDefaults(AbilityType::kAttack);

            // More than the shield so we affect the health too
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromValue(UseDamage() ? 100_fp : 0_fp));

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void InitOmegaAbilities()
    {
        // Add omega abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Omega Attack Chain";
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddEffect(
                EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(120_fp), kTimeInfinite));

            auto& skill2 = ability.AddSkill();
            skill2.name = "Omega Attack Damage";
            skill2.targeting.type = SkillTargetingType::kCurrentFocus;
            skill2.deployment.type = SkillDeploymentType::kDirect;
            skill2.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(UseDamage() ? 200_fp : 0_fp));

            ability.total_duration_ms = UseInstantTimers() ? 0 : 1000;
            skill1.percentage_of_ability_duration = UseInstantTimers() ? 0 : 60;
            skill2.percentage_of_ability_duration = UseInstantTimers() ? 0 : 40;
            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 10;
            skill2.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void InitInnateAbilities() {}

    virtual void SpawnCombatUnits()
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {10, 20}, data, red_entity);
    }

    virtual void AddShields()
    {
        // Add shields to all currently spawned combat units
        // NOTE: We create a copy here because we modify the vector while we loop
        const auto entities = world->GetAll();
        for (const auto& entity : entities)
        {
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            ShieldData shield_data;
            shield_data.sender_id = entity->GetID();
            shield_data.receiver_id = entity->GetID();
            shield_data.duration_ms = kTimeInfinite;
            shield_data.value = 10_fp;

            EntityFactory::SpawnShieldAndAttach(*world, entity->GetTeam(), shield_data);
        }
    }

    void SetHealthVeryHigh()
    {
        auto& blue_stats_component = blue_entity->Get<StatsComponent>();
        auto& red_stats_component = red_entity->Get<StatsComponent>();
        auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
        auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

        constexpr auto max_health = 10000_fp;
        blue_template_stats.Set(StatType::kMaxHealth, max_health).Set(StatType::kCurrentHealth, max_health);
        red_template_stats.Set(StatType::kMaxHealth, max_health).Set(StatType::kCurrentHealth, max_health);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
        SpawnCombatUnits();
    }

    CombatUnitData data;
};

// Variant without Decision System
class AbilitySystemTestNoDecisionSystem : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    void SetUp() override
    {
        Super::SetUp();
        blue_entity->Remove<DecisionComponent>();
        red_entity->Remove<DecisionComponent>();
    }
};

// Variant where there are no effects added to the attack, by default all three damage types should
// apply
class AbilitySystemTestWithAllAttackTypes : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;

            skill1.AddDamageEffect(
                EffectDamageType::kEnergy,
                EffectExpression::FromSenderLiveStat(StatType::kAttackEnergyDamage));
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromSenderLiveStat(StatType::kAttackPhysicalDamage));
            skill1.AddDamageEffect(
                EffectDamageType::kPure,
                EffectExpression::FromSenderLiveStat(StatType::kAttackPureDamage));

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);  // make entities not die
        // Ignore movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 100_fp);
        data.type_data.stats.Set(StatType::kAttackEnergyDamage, 100_fp);
        data.type_data.stats.Set(StatType::kAttackPureDamage, 100_fp);
    }
};

// Variant where there is 1 effect added to the attack, by default missing damage types should
// apply
class AbilitySystemTestWithOneAttackType : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return false;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(150_fp));
            // More than the shield so we affect the health too

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);  // make entities not die
        // Ignore movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 100_fp);
        data.type_data.stats.Set(StatType::kAttackEnergyDamage, 100_fp);
        data.type_data.stats.Set(StatType::kAttackPureDamage, 100_fp);
    }
};

class AbilitySystemTestWithPurestAttackType : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return false;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;

            // Effects
            skill1.AddDamageEffect(
                EffectDamageType::kEnergy,
                EffectExpression::FromSenderLiveStat(StatType::kAttackEnergyDamage));
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromSenderLiveStat(StatType::kAttackPhysicalDamage));
            skill1.AddDamageEffect(
                EffectDamageType::kPure,
                EffectExpression::FromSenderLiveStat(StatType::kAttackPureDamage));
            auto& damage_effect =
                skill1.AddDamageEffect(EffectDamageType::kPurest, EffectExpression::FromValue(100_fp));
            damage_effect.attributes.to_shield_percentage = 100_fp;
        }

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);  // make entities not die
        // Ignore movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 100_fp);
        data.type_data.stats.Set(StatType::kAttackEnergyDamage, 100_fp);
        data.type_data.stats.Set(StatType::kAttackPureDamage, 100_fp);
    }
};

// Variant for overflow stat testing
class AbilitySystemTestWithInterrupt : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return false;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(
                EffectDamageType::kEnergy,
                EffectExpression::FromSenderLiveStat(StatType::kAttackEnergyDamage));
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromSenderLiveStat(StatType::kAttackPhysicalDamage));
            skill1.AddDamageEffect(
                EffectDamageType::kPure,
                EffectExpression::FromSenderLiveStat(StatType::kAttackPureDamage));
            skill1.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);  // make entities not die
        // Ignore movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 150_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 20_fp);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 100_fp);
        data.type_data.stats.Set(StatType::kHitChancePercentage, 100_fp);
    }
};

// Variant where the attack abilities can start at the beginning of the game every second
class AbilitySystemTestWithStartAttackEvery1Second : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    // Empty on purpose
    void InitOmegaAbilities() override {}

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
    }
};

class AbilitySystemTest_AttackSpeedOverflowToAplification : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    // Empty on purpose
    void InitOmegaAbilities() override {}

    FixedPoint GetWorldMaxAttackSpeed() const override
    {
        return 100_fp;
    }
};

// Variant where the attack abilities can start at the beginning of the game every second
class AbilitySystemTestWithStartAttackEvery1SecondCooldown : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";
            ability.activation_trigger_data.activation_cooldown_ms = 2000;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            // 200 ms
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            // More than the shield so we affect the health too
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromValue(UseDamage() ? 100_fp : 0_fp));

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    bool UseInstantTimers() const override
    {
        return true;
    }

    // Empty on purpose
    void InitOmegaAbilities() override {}

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
    }
};

// Variant where the attack abilities can start at the beginning of every 0.53 seconds
class AbilitySystemTestWithStartAttackEvery0Point53Seconds : public AbilitySystemTestWithStartAttackEvery1Second
{
    typedef AbilitySystemTestWithStartAttackEvery1Second Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 189_fp);
    }
};

class AbilitySystemTestWithOverflowResets : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 189_fp);
    }

    void InitOmegaAbilities() override
    {
        // Add omega abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Omega Attack Chain";
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddEffect(
                EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(120_fp), kTimeInfinite));

            ability.total_duration_ms = UseInstantTimers() ? 0 : 700;
            skill1.percentage_of_ability_duration = UseInstantTimers() ? 0 : 100;
            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 10;
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

// Variant where the attack abilities can start at the beginning of every 1.6 seconds
class AbilitySystemTestWithStartAttackEvery1Point6Seconds : public AbilitySystemTestWithStartAttackEvery1Second
{
    typedef AbilitySystemTestWithStartAttackEvery1Second Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 60_fp);
    }
};

// Variant where the attack abilities can start at the beginning of every 2 seconds
class AbilitySystemTestWithStartAttackEvery2Seconds : public AbilitySystemTestWithStartAttackEvery1Second
{
    typedef AbilitySystemTestWithStartAttackEvery1Second Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 50_fp);
    }
};

// Variant where the attack abilities can start at the beginning of the game every time step

class AbilitySystemTestWithStartAttackEveryTimeStep : public AbilitySystemTestWithStartAttackEvery1Second
{
    typedef AbilitySystemTestWithStartAttackEvery1Second Super;

protected:
    void InitStats() override
    {
        Super::InitStats();

        // Every time step
        data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);
    }
};

// Variant where the attack abilities can start at the beginning of every 1.6 seconds
class AbilitySystemTestWithVeryHighAttackSpeed : public AbilitySystemTestWithStartAttackEvery1Second
{
    typedef AbilitySystemTestWithStartAttackEvery1Second Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 4500_fp);
    }

    FixedPoint GetWorldMaxAttackSpeed() const override
    {
        return 5000_fp;
    }
};

// Variant where the attack ability's speed changes
class AbilitySystemTestWithSimpleSpeedChanges : public AbilitySystemTestWithStartAttackEvery1Second
{
    typedef AbilitySystemTestWithStartAttackEvery1Second Super;

protected:
    bool UseInstantTimers() const override
    {
        return false;
    }

    void InitAttackAbilities() override
    {
        Super::InitAttackAbilities();

        // Edit attack abilities
        {
            auto& ability = data.type_data.attack_abilities.abilities[0];
            ability.get()->skills[0].get()->deployment.pre_deployment_delay_percentage = 40;
        }
    }

    void InitStats() override
    {
        Super::InitStats();
        // Attack every 500ms
        data.type_data.stats.Set(StatType::kAttackSpeed, 200_fp);
    }
};

// Variant where the attack ability's speed changes
class AbilitySystemTestWithComplexSpeedChanges : public AbilitySystemTestWithStartAttackEvery1Second
{
    typedef AbilitySystemTestWithStartAttackEvery1Second Super;

protected:
    bool UseInstantTimers() const override
    {
        return false;
    }

    void InitAttackAbilities() override
    {
        Super::InitAttackAbilities();

        // Edit attack abilities
        {
            auto& ability = data.type_data.attack_abilities.abilities[0];
            ability.get()->skills[0].get()->deployment.pre_deployment_delay_percentage = 40;
        }
    }

    void InitStats() override
    {
        Super::InitStats();
        // Attack every 0.53s
        data.type_data.stats.Set(StatType::kAttackSpeed, 189_fp);
    }
};

// Variant where the omega ability can fire at the beginning of the game
class AbilitySystemTestWithStartOmega : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return false;
    }

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kStartingEnergy, data.type_data.stats.Get(StatType::kEnergyCost));
    }
};

// AbilitySystemTestWithStartOmega but instant
class AbilitySystemTestWithStartOmegaInstant : public AbilitySystemTestWithStartOmega
{
    typedef AbilitySystemTestWithStartOmega Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }
};

// Start omega ability instant but no damage
class AbilitySystemTestWithStartOmegaInstantNoDamage : public AbilitySystemTestWithStartOmegaInstant
{
    typedef AbilitySystemTestWithStartOmegaInstant Super;

protected:
    bool UseDamage() const override
    {
        return false;
    }
};

// Just one skill for the omega ability
class AbilitySystemTestWithStartOmegaOneSkill : public AbilitySystemTestWithStartOmega
{
    typedef AbilitySystemTestWithStartOmega Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            // More than the shield so we affect the health too
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromValue(UseDamage() ? 100_fp : 0_fp));

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitOmegaAbilities() override
    {
        // Add omega abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Foo";

            auto& skill1 = ability.AddSkill();
            skill1.name = "Omega Attack Damage";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(UseDamage() ? 200_fp : 0_fp));

            ability.total_duration_ms = UseInstantTimers() ? 0 : 500;
            skill1.percentage_of_ability_duration = UseInstantTimers() ? 0 : kMaxPercentage;
            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 10;
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

// Just one skill for the omega ability
class AbilitySystemTestWithStartOmegaZoneSkill : public AbilitySystemTestWithStartOmega
{
    typedef AbilitySystemTestWithStartOmega Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 125_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 25_fp);
    }

    void InitOmegaAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Zone Ability";
            // Timers are instant 0

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Zone Omega";
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kZone;

            // Set the zone data
            skill1.zone.shape = ZoneEffectShape::kHexagon;
            skill1.zone.radius_units = 20;
            skill1.zone.max_radius_units = 20;
            skill1.zone.width_units = 40;
            skill1.zone.height_units = 20;
            skill1.zone.direction_degrees = 0;
            skill1.zone.duration_ms = 100;
            skill1.zone.frequency_ms = 100;
            skill1.zone.apply_once = false;

            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp));
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

// Just one skill for the omega ability
class AbilitySystemTestWithStartOmegaDirectSkill : public AbilitySystemTestWithStartOmega
{
    typedef AbilitySystemTestWithStartOmega Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 125_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 25_fp);
    }

    void InitOmegaAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Zone Ability";
            // Timers are instant 0

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Zone Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;

            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp));
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

// Start omega ability instant but no damage
class AbilitySystemTestWithStartOmegaOneSkillNoDamage : public AbilitySystemTestWithStartOmegaOneSkill
{
    typedef AbilitySystemTestWithStartOmegaOneSkill Super;

protected:
    bool UseDamage() const override
    {
        return false;
    }
};

// Variant where the omega ability would fire and make us untargetable
// TODO(vampy): Test this case
class AbilitySystemTestWithStartOmegaPositiveStateUntargetable : public AbilitySystemTestWithStartOmegaInstant
{
    typedef AbilitySystemTestWithStartOmegaInstant Super;

protected:
    void InitOmegaAbilities() override
    {
        // Add omega abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Make ourselves untargetable";
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddEffect(EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 200));

            auto& skill2 = ability.AddSkill();
            skill2.name = "Omega Attack Damage";
            skill2.targeting.type = SkillTargetingType::kCurrentFocus;
            skill2.deployment.type = SkillDeploymentType::kDirect;
            skill2.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

            ability.total_duration_ms = UseInstantTimers() ? 0 : 1000;
            skill1.percentage_of_ability_duration = UseInstantTimers() ? 0 : 60;
            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 10;
            skill2.percentage_of_ability_duration = UseInstantTimers() ? 0 : 40;
            skill2.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

class AbilitySystemTestWithStartOmegaAlwaysCrit : public AbilitySystemTestWithStartOmegaInstant
{
    typedef AbilitySystemTestWithStartOmegaInstant Super;

protected:
    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);
        data.type_data.stats.Set(StatType::kResolve, 0_fp);
        data.type_data.stats.Set(StatType::kEnergyResist, 0_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 150_fp);
    }

    void InitOmegaAbilities() override
    {
        // Add omega abilities
        // Instant
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Physical Damage that can crit";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.effect_package.attributes.can_crit = true;
            skill1.effect_package.attributes.always_crit = true;
            skill1.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

class ScaledEnergyGainTest : public AbilitySystemBaseTest
{
    typedef AbilitySystemBaseTest Super;

public:
    const FixedPoint world_scale = 1000_fp;

protected:
    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();
        data2 = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kMaxHealth, 50_fp * world_scale);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp * world_scale);

        // Stats
        data2.radius_units = 1;
        data2.type_data.combat_affinity = CombatAffinity::kFire;
        data2.type_data.combat_class = CombatClass::kBehemoth;
        data2.type_data.stats.Set(StatType::kMaxHealth, 100_fp * world_scale);
        data2.type_data.stats.Set(StatType::kEnergyCost, 100_fp * world_scale);

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn 2 combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {15, 30}, data2, red_entity);

        ASSERT_FALSE(world->AreAllies(blue_entity->GetID(), red_entity->GetID()));
    }

    FixedPoint GetWorldStatsConstantsScale() const override
    {
        return world_scale;
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
    }

    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    CombatUnitData data;
    CombatUnitData data2;
};

class AbilitySystemTestWithOmegaRange : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            // 200 ms
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            // More than the shield so we affect the health too
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromValue(UseDamage() ? 100_fp : 0_fp));

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitOmegaAbilities() override
    {
        // Add omega abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Foo";

            auto& skill2 = ability.AddSkill();
            skill2.name = "Omega Attack Damage";
            skill2.targeting.type = SkillTargetingType::kCurrentFocus;
            skill2.deployment.type = SkillDeploymentType::kDirect;
            skill2.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(UseDamage() ? 200_fp : 0_fp));

            ability.total_duration_ms = UseInstantTimers() ? 0 : 1000;
            skill2.percentage_of_ability_duration = UseInstantTimers() ? 0 : 40;
            skill2.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);  // make entities not die
        // Ignore movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 5_fp);
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 1_fp);
    }
};

class AbilitySystemTestRefundHealthAndEnergy : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
            skill1.effect_package.attributes.refund_health = EffectExpression::FromValue(50_fp);
            skill1.effect_package.attributes.refund_energy = EffectExpression::FromValue(50_fp);
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitStats() override
    {
        Super::InitStats();
        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);
    }
};

class AbilitySystemTestBonusAndAmpDamage : public AbilitySystemBaseTest
{
    typedef AbilitySystemBaseTest Super;

protected:
    virtual void InitAttackAbilityData(
        EffectDamageType damage_type,
        const EffectPackageAttributes& effect_package_attributes)
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(damage_type, EffectExpression::FromValue(50_fp));
        skill1.effect_package.attributes = effect_package_attributes;
    }

    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kMaxHealth, 500_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn 2 combat units
        SpawnCombatUnit(Team::kBlue, {2, 11}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {5, 30}, data, red_entity);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
    }

    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    CombatUnitData data;
};

class AbilitySystemTestEffectPackage : public AbilitySystemBaseTest
{
    typedef AbilitySystemBaseTest Super;

protected:
    virtual void InitAttackAbilityData(
        const EffectData& effect_data,
        const EffectPackageAttributes& effect_package_attributes)
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(effect_data);
        skill1.effect_package.attributes = effect_package_attributes;
    }

    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kMaxHealth, 500_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 500_fp);

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn 2 combat units
        SpawnCombatUnit(Team::kBlue, {2, 11}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {5, 30}, data, red_entity);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
    }

    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    CombatUnitData data;
};

class AbilitySystemTestOmegaAfterFirstAttack : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    void InitStats() override
    {
        Super::InitStats();

        // Every time step
        data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

        // Omega should be activated after first attack
        data.type_data.stats.Set(StatType::kStartingEnergy, data.type_data.stats.Get(StatType::kEnergyCost) - 1_fp);

        // We don't need a range or focus for omega
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 0_fp);

        // Blue and red have the same initial stats
        red_data = data;
        blue_data = data;
    }

    void InitAttackAbilities() override
    {
        // Red attack ability
        {
            auto& ability = red_data.type_data.attack_abilities.AddAbility();
            auto& skill = ability.AddSkill();
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
            skill.deployment.pre_deployment_delay_percentage = 0;
        }
        red_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

        // Blue attack ability
        {
            auto& ability = blue_data.type_data.attack_abilities.AddAbility();
            auto& skill = ability.AddSkill();
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
            skill.deployment.pre_deployment_delay_percentage = 0;
        }
        blue_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitOmegaAbilities() override
    {
        // red omega ability
        {
            auto& ability = red_data.type_data.omega_abilities.AddAbility();
            constexpr int duration = 2000;
            ability.total_duration_ms = duration;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            // skill1.AddEffect(EffectData::CreateHeal(EffectHealType::kPure, EffectExpression::FromValue(100)));
            skill1.AddEffect(EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, duration * 2));
        }
        red_data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;

        // Blue omega ability
        {
            auto& ability = blue_data.type_data.omega_abilities.AddAbility();
            constexpr int duration = 500;
            ability.total_duration_ms = duration;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddEffect(EffectData::CreateHeal(EffectHealType::kPure, EffectExpression::FromValue(100_fp)));
        }
        blue_data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SpawnCombatUnits() override
    {
        // Spawn 2 combat units
        SpawnCombatUnit(Team::kBlue, {10, 10}, blue_data, blue_entity);
        SpawnCombatUnit(Team::kRed, {10, 13}, red_data, red_entity);
    }

    CombatUnitData blue_data;
    CombatUnitData red_data;
};

class AbilitySystemEmpowerTest : public AbilitySystemBaseTest
{
    typedef AbilitySystemBaseTest Super;

public:
    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;

protected:
    virtual bool UseInstantTimers() const
    {
        return false;
    }
    virtual bool UseDamage() const
    {
        return true;
    }

    virtual void InitCombatUnitData()
    {
        InitStats();
        InitAttackAbilities();
        InitInnateAbilities();
    }

    virtual void InitStats()
    {
        data = CreateCombatUnitData();

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

        // Once every second
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);

        data.type_data.stats.Set(StatType::kResolve, 0_fp);
        data.type_data.stats.Set(StatType::kGrit, 0_fp);
        data.type_data.stats.Set(StatType::kWillpowerPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kTenacityPercentage, 0_fp);
        // 15^2 == 225
        // square distance between blue and red is 200
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 25_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    }

    virtual void InitAttackAbilities()
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            // 200 ms
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            // More than the shield so we affect the health too
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromValue(UseDamage() ? 100_fp : 0_fp));

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
            skill1.is_critical = true;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void InitInnateAbilities() {}

    virtual void SpawnCombatUnits()
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {10, 20}, data, red_entity);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
        SpawnCombatUnits();
    }

    CombatUnitData data;
};

class AbilitySystemTestOmegaAfterFirstAttackWithSlowAbility : public AbilitySystemTestOmegaAfterFirstAttack
{
    typedef AbilitySystemTestOmegaAfterFirstAttack Super;

protected:
    void InitAttackAbilities() override
    {
        // Red attack ability
        {
            auto& ability = red_data.type_data.attack_abilities.AddAbility();
            auto& skill = ability.AddSkill();
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
            skill.deployment.pre_deployment_delay_percentage = 50;
        }
        red_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

        // Blue attack ability
        {
            auto& ability = blue_data.type_data.attack_abilities.AddAbility();
            ability.total_duration_ms = 200;
            auto& skill = ability.AddSkill();
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
            skill.deployment.pre_deployment_delay_percentage = 0;
        }
        blue_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

}  // namespace simulation
