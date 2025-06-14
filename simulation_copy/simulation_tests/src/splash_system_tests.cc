#include <memory>

#include "base_test_fixtures.h"
#include "components/focus_component.h"
#include "components/stats_component.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"
#include "gtest/gtest.h"
#include "utility/entity_helper.h"

namespace simulation
{
class SplashSystemTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();
        data2 = CreateCombatUnitData();

        InitStats();
        InitAbilities();
    }

    virtual void InitStats()
    {
        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);

        // Only increase it by a little
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 105_fp);
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 0_fp);
        data.type_data.stats.Set(StatType::kAttackEnergyDamage, 10_fp);
        data.type_data.stats.Set(StatType::kAttackPureDamage, 0_fp);

        // Instant
        data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

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
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);

        // Stats
        data2.radius_units = 1;
        data2.type_data.combat_affinity = CombatAffinity::kFire;
        data2.type_data.combat_class = CombatClass::kBehemoth;
        data2.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);

        // Only increase it by a little
        data2.type_data.stats.Set(StatType::kCritAmplificationPercentage, 105_fp);
        data2.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        data2.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data2.type_data.stats.Set(StatType::kAttackPhysicalDamage, 0_fp);
        data2.type_data.stats.Set(StatType::kAttackEnergyDamage, 10_fp);
        data2.type_data.stats.Set(StatType::kAttackPureDamage, 0_fp);

        // Instant
        data2.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

        data2.type_data.stats.Set(StatType::kWillpowerPercentage, 11_fp);
        data2.type_data.stats.Set(StatType::kTenacityPercentage, 12_fp);
        // 15^2 == 225
        // square distance between blue and red is 200
        data2.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);

        data2.type_data.stats.Set(StatType::kGrit, 10_fp);
        data2.type_data.stats.Set(StatType::kResolve, 20_fp);

        // Resists
        data2.type_data.stats.Set(StatType::kPhysicalResist, 10_fp);
        data2.type_data.stats.Set(StatType::kEnergyResist, 5_fp);

        data2.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data2.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data2.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data2.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
    }

    virtual void InitAbilities()
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Splash Ability";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Splash Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.targeting.group = AllegianceType::kEnemy;
            skill1.percentage_of_ability_duration = 100;
            skill1.deployment.pre_deployment_delay_percentage = 0;

            AddSplashPropagation(skill1.effect_package.attributes.propagation);
            skill1.effect_package.effects = GetSkillEffects();
        }

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void AddSplashPropagation(EffectPackagePropagationAttributes& propagation) const
    {
        propagation.type = EffectPropagationType::kSplash;
        propagation.splash_radius_units = GetSplashRadius();
        propagation.effect_package = std::make_shared<EffectPackage>();
        for (const auto& effect_data : GetPropagationEffects())
        {
            propagation.effect_package->AddEffect(effect_data);
        }
    }

    virtual int GetSplashRadius() const
    {
        return 4;
    }

    virtual std::vector<EffectData> GetSkillEffects() const
    {
        std::vector<EffectData> effects;

        auto effect = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(111_fp));
        effect.lifetime.overlap_process_type = EffectOverlapProcessType::kStacking;
        effects.emplace_back(effect);

        return effects;
    }

    virtual std::vector<EffectData> GetPropagationEffects() const
    {
        return {EffectData::CreateNegativeState(EffectNegativeState::kRoot, kTimeInfinite)};
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {2, 2}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {5, 5}, data2, red_entity);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
        SpawnCombatUnits();

        world->SubscribeToEvent(
            EventType::kTryToApplyEffect,
            [this](const Event& event)
            {
                events_apply_effect.emplace_back(event.Get<event_data::Effect>());
            });

        world->SubscribeToEvent(
            EventType::kSplashCreated,
            [this](const Event& event)
            {
                events_splash_created.emplace_back(event.Get<event_data::SplashCreated>());
            });

        world->SubscribeToEvent(
            EventType::kSplashDestroyed,
            [this](const Event& event)
            {
                events_splash_destroyed.emplace_back(event.Get<event_data::SplashDestroyed>());
            });

        world->SubscribeToEvent(
            EventType::kZoneCreated,
            [this](const Event& event)
            {
                events_zone_created.emplace_back(event.Get<event_data::ZoneCreated>());
            });

        world->SubscribeToEvent(
            EventType::kZoneDestroyed,
            [this](const Event& event)
            {
                events_zone_destroyed.emplace_back(event.Get<event_data::ZoneDestroyed>());
            });

        world->SubscribeToEvent(
            EventType::kOnAttachedEffectRemoved,
            [this](const Event& event)
            {
                events_remove_effect.emplace_back(event.Get<event_data::Effect>());
            });
    }

    void ApplyShieldTo(const Entity& entity, FixedPoint value)
    {
        const auto entity_id = entity.GetID();

        ShieldData shield_data;
        shield_data.sender_id = entity_id;
        shield_data.receiver_id = entity_id;
        shield_data.value = value;

        EntityFactory::SpawnShieldAndAttach(*world, entity.GetTeam(), shield_data);
    }

    std::vector<event_data::Effect> events_apply_effect;
    std::vector<event_data::SplashCreated> events_splash_created;
    std::vector<event_data::ZoneCreated> events_zone_created;
    std::vector<event_data::SplashDestroyed> events_splash_destroyed;
    std::vector<event_data::ZoneDestroyed> events_zone_destroyed;
    std::vector<event_data::Effect> events_remove_effect;
    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    Entity* red_entity2 = nullptr;
    CombatUnitData data;
    CombatUnitData data2;
};

class SplashSystemTestWithEmpower : public SplashSystemTest
{
    typedef SplashSystemTest Super;

    void InitAbilities() override
    {
        // Add attack ability
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Attack Ability";

            // Skills
            auto& skill = ability.AddSkill();
            skill.name = "Attack Skill";
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.targeting.group = AllegianceType::kEnemy;
            skill.percentage_of_ability_duration = 100;
            skill.deployment.pre_deployment_delay_percentage = 0;
            skill.effect_package.effects = GetSkillEffects();
        }

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

        // Add omega which empowers attack ability
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Omega Ability";

            // Skills
            auto& skill = ability.AddSkill();
            skill.name = "Omega Skill";
            skill.targeting.type = SkillTargetingType::kSelf;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.percentage_of_ability_duration = 100;

            EffectData effect;
            effect.type_id.type = EffectType::kEmpower;
            effect.lifetime.is_consumable = true;
            effect.lifetime.activations_until_expiry = 1;
            effect.lifetime.duration_time_ms = 10000;  // Should be ignored
            effect.lifetime.activated_by = AbilityType::kAttack;
            AddSplashPropagation(effect.attached_effect_package_attributes.propagation);
            skill.effect_package.effects = {effect};
        }

        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

class SplashSystemTestWithFrost : public SplashSystemTest
{
    typedef SplashSystemTest Super;

    std::vector<EffectData> GetSkillEffects() const override
    {
        return {EffectData::CreateCondition(EffectConditionType::kFrost, kDefaultAttachedEffectsFrequencyMs)};
    }
};

class SplashSystemTestOutOfRange : public SplashSystemTest
{
    typedef SplashSystemTest Super;

protected:
    int GetSplashRadius() const override
    {
        return 1;
    }
};

// In this test the skill also brings blind effect with it
class SplashSystemTestWithOriginal : public SplashSystemTest
{
    typedef SplashSystemTest Super;

protected:
    void AddSplashPropagation(EffectPackagePropagationAttributes& propagation) const override
    {
        Super::AddSplashPropagation(propagation);
        propagation.add_original_effect_package = true;
    }

    std::vector<EffectData> GetSkillEffects() const override
    {
        std::vector<EffectData> effects = Super::GetSkillEffects();
        effects.push_back(EffectData::CreateNegativeState(EffectNegativeState::kBlind, 500));
        return effects;
    }
};

// In this test the skill original effect is replaced by spawned splash
class SplashSystemTestReplaceOriginal : public SplashSystemTestWithOriginal
{
    typedef SplashSystemTestWithOriginal Super;

protected:
    void AddSplashPropagation(EffectPackagePropagationAttributes& propagation) const override
    {
        Super::AddSplashPropagation(propagation);
        propagation.skip_original_effect_package = true;
    }
};

class SplashSystemTestIgnoreFocus : public SplashSystemTest
{
    typedef SplashSystemTest Super;

protected:
    void AddSplashPropagation(EffectPackagePropagationAttributes& propagation) const override
    {
        Super::AddSplashPropagation(propagation);
        propagation.ignore_first_propagation_receiver = true;
    }
};

class SplashSystemTestWithExpression : public SplashSystemTest
{
    typedef SplashSystemTest Super;

protected:
    std::vector<EffectData> GetPropagationEffects() const override
    {
        // 2% of receiver health * 3% of sender health
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                2_fp,
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromStatPercentage(
                3_fp,
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender)};

        return {
            EffectData::CreateNegativeState(EffectNegativeState::kRoot, kTimeInfinite),
            EffectData::CreateDamage(EffectDamageType::kEnergy, expression)};
    }
};

TEST_F(SplashSystemTest, SplashPropagation)
{
    world->TimeStep();

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    // Blue entity applied effect which dealt 111 energy damage to red entity
    // and spawned one splash entity (which will be activated on the next time step only)
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(EvaluateNoStats(events_apply_effect.at(0).data.GetExpression()), 111_fp);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state to red entity
    // AND one implicitly created instand damage effect
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_apply_effect[1].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[1].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone destroyed at the next time step
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);
}

TEST_F(SplashSystemTest, SplashPropagationWithShield)
{
    // Add shields
    constexpr FixedPoint shield_amount = 50_fp;
    ApplyShieldTo(*red_entity, shield_amount);

    world->TimeStep();

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    // Blue entity applied effect which dealt 111 energy damage to red entity
    // and spawned one splash entity (which will be activated on the next time step only)
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(EvaluateNoStats(events_apply_effect.at(0).data.GetExpression()), 111_fp);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    EXPECT_EQ(red_entity->Get<StatsComponent>().GetCurrentHealth(), "908.048"_fp + shield_amount);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state to red entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_apply_effect[1].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[1].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone destroyed at the next time step
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);
}

TEST_F(SplashSystemTestWithOriginal, SplashPropagationWithOriginal)
{
    // Confirm initial state of movable
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    // Add shields
    constexpr FixedPoint shield_amount = 50_fp;
    ApplyShieldTo(*red_entity, shield_amount);

    world->TimeStep();

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    // Blue entity applied skill with
    //  - 111 energy damage effect
    //  - blind negative state effect lasting 500 ms
    //  - spawns splash entity at target location with
    //     - infinite root negative state effect
    //     - includes original effect package (instant damage and blind)
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(EvaluateNoStats(events_apply_effect.at(0).data.GetExpression()), 111_fp);
    EXPECT_EQ(events_apply_effect.at(1).data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect.at(1).data.type_id.negative_state, EffectNegativeState::kBlind);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    EXPECT_EQ(red_entity->Get<StatsComponent>().GetCurrentHealth(), "908.048"_fp + shield_amount);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state to red entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 5);
    EXPECT_EQ(events_apply_effect[2].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[2].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_apply_effect[3].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(EvaluateNoStats(events_apply_effect[3].data.GetExpression()), 111_fp);
    EXPECT_EQ(events_apply_effect[4].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[4].data.type_id.negative_state, EffectNegativeState::kBlind);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone destroyed at the next time step
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 5);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);
}

TEST_F(SplashSystemTestReplaceOriginal, SplashSystemTestReplaceOriginal)
{
    // Confirm initial state of movable
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    // Add shields
    constexpr FixedPoint shield_amount = 50_fp;
    ApplyShieldTo(*red_entity, shield_amount);

    world->TimeStep();

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    // Blue entity applied skill with
    //  - original
    //  - spawns splash entity at target location with
    //     - infinite root negative state effect
    //     - includes original effect package (instant damage and blind)
    ASSERT_EQ(events_apply_effect.size(), 0);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 0);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state to red entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 3);
    EXPECT_EQ(events_apply_effect[0].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[0].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_apply_effect[1].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(EvaluateNoStats(events_apply_effect[1].data.GetExpression()), 111_fp);
    EXPECT_EQ(events_apply_effect[2].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[2].data.type_id.negative_state, EffectNegativeState::kBlind);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    EXPECT_EQ(red_entity->Get<StatsComponent>().GetCurrentHealth(), "908.048"_fp + shield_amount);

    // zone destroyed at the next time step
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 3);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);
}

TEST_F(SplashSystemTestWithFrost, SplashPropagationWithFrost)
{
    // Confirm initially red entity is not frosted
    ASSERT_FALSE(EntityHelper::IsFrosted(*red_entity));

    // Add shields
    constexpr FixedPoint shield_amount = 50_fp;
    ApplyShieldTo(*red_entity, shield_amount);

    world->TimeStep();

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    // Blue entity attacks red entity and applies frost effect
    // Also spawnes one splash entity (which will be activated at the next time step)
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kCondition);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.condition_type, EffectConditionType::kFrost);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state to red entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_apply_effect[1].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[1].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    ASSERT_TRUE(EntityHelper::IsFrosted(*red_entity));
    ASSERT_FALSE(EntityHelper::IsFrosted(*blue_entity));

    // zone destroyed at the next time step
    // applied effects count remains the same
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);

    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kFrost);
    const int num_time_steps = Time::MsToTimeSteps(dot_config.duration_ms);
    for (int i = 0; i < num_time_steps - 4; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Should be frosted
        ASSERT_TRUE(EntityHelper::IsFrosted(*red_entity)) << " i = " << i;

        // Blue should not be frosted
        ASSERT_FALSE(EntityHelper::IsFrosted(*blue_entity)) << " i = " << i;
    }

    world->TimeStep();

    ASSERT_FALSE(EntityHelper::IsFrosted(*red_entity));
    ASSERT_FALSE(EntityHelper::IsFrosted(*blue_entity));
}

TEST_F(SplashSystemTestIgnoreFocus, SplashPropagationIgnoreFocus)
{
    // Have the second red entity that will receive propagataion effects
    SpawnCombatUnit(Team::kRed, {7, 7}, data2, red_entity2);

    world->TimeStep();

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    // Blue entity applied effect which dealt 111 energy damage to red entity
    // and spawned one splash entity (which will be activated on the next time step only)
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(EvaluateNoStats(events_apply_effect.at(0).data.GetExpression()), 111_fp);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    EXPECT_EQ(red_entity->Get<StatsComponent>().GetCurrentHealth(), "908.048"_fp);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state but ignore red entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_apply_effect[1].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[1].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_apply_effect[1].receiver_id, red_entity2->GetID());
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone destroyed at the next time step
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);
}

TEST_F(SplashSystemTestOutOfRange, SplashPropagationOutOfRange)
{
    // Have the second red entity that will be too far from its
    // teammate to receive ramage from zone. Expected to have
    // no difference with SplashPropagation test
    SpawnCombatUnit(Team::kRed, {10, 10}, data2, red_entity2);
    world->TimeStep();

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    // Blue entity applied effect which dealt 111 energy damage to red entity
    // and spawned one splash entity (which will be activated on the next time step only)
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(EvaluateNoStats(events_apply_effect.at(0).data.GetExpression()), 111_fp);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    EXPECT_EQ(red_entity->Get<StatsComponent>().GetCurrentHealth(), "908.048"_fp);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state to red entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_apply_effect[1].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[1].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone destroyed at the next time step
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);
}

TEST_F(SplashSystemTestWithExpression, SplashPropagationWithExpression)
{
    world->TimeStep();

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    auto evaluate = [&](const EffectExpression& expression) -> FixedPoint
    {
        const ExpressionEvaluationContext context{world.get(), blue_entity->GetID(), red_entity->GetID()};
        return expression.Evaluate(context);
    };

    // Blue entity applied effect which dealt 111 energy damage to red entity
    // and spawned one splash entity (which will be activated on the next time step only)
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(evaluate(events_apply_effect.at(0).data.GetExpression()), 111_fp);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    EXPECT_EQ(red_entity->Get<StatsComponent>().GetCurrentHealth(), "908.048"_fp);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state to red entity
    // AND one damage effect with expression that depends on sender and receiver health
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 3);
    EXPECT_EQ(events_apply_effect[1].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[1].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_apply_effect[2].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(events_apply_effect[2].data.type_id.damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(evaluate(events_apply_effect[2].data.GetExpression()), 600_fp);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone destroyed at the next time step
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 3);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);
}

TEST_F(SplashSystemTestWithEmpower, EmpowerAttackAbilitWithSplashPropagation)
{
    auto& blue_stats = blue_entity->Get<StatsComponent>();
    blue_stats.SetCurrentEnergy(blue_stats.GetEnergyCost());
    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();

    world->TimeStep();

    // Blue entity activates omega which empower the next attack
    ASSERT_EQ(events_apply_effect.size(), 1);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kEmpower);
    EXPECT_EQ(events_splash_created.size(), 0);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // On the second time step blue entity uses attack ability but empowered with splash propagation
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_apply_effect.at(1).data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(EvaluateNoStats(events_apply_effect.at(1).data.GetExpression()), 111_fp);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 0);
    EXPECT_EQ(events_zone_created.size(), 0);
    EXPECT_EQ(events_zone_destroyed.size(), 0);
    EXPECT_EQ(red_entity->Get<StatsComponent>().GetCurrentHealth(), "908.048"_fp);

    // Stun blue entity to prevent it from further attacks
    // (makes test a little bit simpler)
    Stun(*blue_entity);

    // Splash that was created on the previous time step should spawn a zone entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone entity should apply negative state to red entity
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 3);
    EXPECT_EQ(events_apply_effect[2].data.type_id.type, EffectType::kNegativeState);
    EXPECT_EQ(events_apply_effect[2].data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 0);

    // zone destroyed at the next time step
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 3);
    EXPECT_EQ(events_splash_created.size(), 1);
    EXPECT_EQ(events_splash_destroyed.size(), 1);
    EXPECT_EQ(events_zone_created.size(), 1);
    EXPECT_EQ(events_zone_destroyed.size(), 1);
}

}  // namespace simulation
