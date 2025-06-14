#include <memory>

#include "base_test_fixtures.h"
#include "components/attached_entity_component.h"
#include "components/duration_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "data/effect_expression_custom_functions.h"
#include "factories/entity_factory.h"
#include "gtest/gtest.h"
#include "systems/attached_entity_system.h"
#include "test_data_loader.h"

namespace simulation
{
class ShieldSystemTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual FixedPoint GetAttackSpeed() const
    {
        return 1000_fp;
    }

    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 5_fp);
        data.type_data.stats.Set(StatType::kAttackEnergyDamage, 0_fp);

        // Once every tick
        data.type_data.stats.Set(StatType::kAttackSpeed, GetAttackSpeed());

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
        data.type_data.stats.Set(StatType::kMaxHealth, 500_fp);

        // Stop movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    }

    void SpawnCombatUnits(const int shield_duration_ms = -1)
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);

        // Add 5 shields
        for (int i = 0; i < 5; i++)
        {
            ShieldData blue_shield_data, red_shield_data;
            blue_shield_data.sender_id = blue_entity->GetID();
            blue_shield_data.receiver_id = blue_entity->GetID();

            red_shield_data.sender_id = red_entity->GetID();
            red_shield_data.receiver_id = red_entity->GetID();

            blue_shield_data.value = 10_fp;
            blue_shield_data.duration_ms = shield_duration_ms;
            red_shield_data.value = 10_fp;
            red_shield_data.duration_ms = shield_duration_ms;
            EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data);
            EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);
        }
    }

    void
    SpawnCombatUnitsEventSkill(const EventType event_type, const int shield_duration_ms, const bool apply_damage = true)
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);

        // Add 5 shields
        for (int i = 0; i < 5; i++)
        {
            ShieldData blue_shield_data, red_shield_data;
            blue_shield_data.sender_id = blue_entity->GetID();
            blue_shield_data.receiver_id = blue_entity->GetID();

            red_shield_data.sender_id = red_entity->GetID();
            red_shield_data.receiver_id = red_entity->GetID();

            blue_shield_data.value = 10_fp;
            blue_shield_data.duration_ms = shield_duration_ms;
            red_shield_data.value = 10_fp;
            red_shield_data.duration_ms = shield_duration_ms;

            // Add a blue skill
            // Heals self
            std::shared_ptr<SkillData> skill_data1 = SkillData::Create();
            skill_data1->targeting.type = SkillTargetingType::kSelf;
            skill_data1->deployment.type = SkillDeploymentType::kDirect;
            skill_data1->AddEffect(EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(15_fp)));
            blue_shield_data.event_skills[event_type] = std::move(skill_data1);

            // Add a red skill
            // Damages current focus
            if (apply_damage)
            {
                std::shared_ptr<SkillData> skill_data2 = SkillData::Create();
                skill_data2->targeting.type = SkillTargetingType::kCurrentFocus;
                skill_data2->deployment.type = SkillDeploymentType::kDirect;
                skill_data2->AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(20_fp));
                red_shield_data.event_skills[event_type] = std::move(skill_data2);
            }

            EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data);
            EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);
        }
    }

    virtual void SpawnCombatUnitsEventSkillWithCrit(const EventType event_type, const int shield_duration_ms)
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);

        ShieldData blue_shield_data, red_shield_data;
        blue_shield_data.sender_id = blue_entity->GetID();
        blue_shield_data.receiver_id = blue_entity->GetID();

        red_shield_data.sender_id = red_entity->GetID();
        red_shield_data.receiver_id = red_entity->GetID();

        blue_shield_data.value = 10_fp;
        blue_shield_data.duration_ms = shield_duration_ms;
        red_shield_data.value = 10_fp;
        red_shield_data.duration_ms = shield_duration_ms;

        // Add a red skill
        // Damages current focus
        std::shared_ptr<SkillData> skill_data1 = SkillData::Create();
        skill_data1->targeting.type = SkillTargetingType::kCurrentFocus;
        skill_data1->deployment.type = SkillDeploymentType::kDirect;
        skill_data1->AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));
        red_shield_data.event_skills[event_type] = std::move(skill_data1);

        EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data);
        EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);
    }

    void SpawnCombatUnitsWithShields(const int shield_duration_ms = -1)
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);

        // Add 5 shields
        for (int i = 0; i < 5; i++)
        {
            ShieldData blue_shield_data, red_shield_data;
            blue_shield_data.sender_id = blue_entity->GetID();
            blue_shield_data.receiver_id = blue_entity->GetID();

            red_shield_data.sender_id = red_entity->GetID();
            red_shield_data.receiver_id = red_entity->GetID();

            blue_shield_data.value = 10_fp;
            blue_shield_data.duration_ms = shield_duration_ms;
            red_shield_data.value = 10_fp;
            red_shield_data.duration_ms = shield_duration_ms;

            // Add buffs
            auto blue_buff = EffectData::CreateBuff(
                StatType::kOmegaPowerPercentage,
                EffectExpression::FromValue(50_fp),
                kTimeInfinite);
            blue_shield_data.effects.push_back(blue_buff);
            auto red_buff = EffectData::CreateBuff(
                StatType::kAttackPhysicalDamage,
                EffectExpression::FromValue(20_fp),
                kTimeInfinite);
            red_shield_data.effects.push_back(red_buff);

            EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data);
            EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);
        }
    }

    void SpawnCombatUnitsEmpower(const int shield_duration_ms = -1)
    {
        // Set up an attack for the empower
        CombatUnitData data_with_empower(data);
        auto& ability = data_with_empower.type_data.attack_abilities.AddAbility();
        ability.name = "Test Ability";
        auto& skill = ability.AddSkill();
        skill.name = "Test";
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.deployment.pre_deployment_delay_percentage = 0;
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        // This attack does no damage on its own - simpler for testing
        skill.AddDamageEffect(
            EffectDamageType::kPhysical,
            EffectExpression::FromSenderLiveStat(StatType::kAttackEnergyDamage));
        ability.skills.push_back(std::make_shared<SkillData>(skill));

        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data_with_empower, blue_entity);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);

        // Add 5 shields
        for (int i = 0; i < 5; i++)
        {
            ShieldData blue_shield_data, red_shield_data;
            blue_shield_data.sender_id = blue_entity->GetID();
            blue_shield_data.receiver_id = blue_entity->GetID();

            red_shield_data.sender_id = red_entity->GetID();
            red_shield_data.receiver_id = red_entity->GetID();

            blue_shield_data.value = 10_fp;
            blue_shield_data.duration_ms = shield_duration_ms;
            red_shield_data.value = 10_fp;
            red_shield_data.duration_ms = shield_duration_ms;

            // Add empower and effect of pure damage
            EffectData effect_data;
            effect_data.type_id.type = EffectType::kEmpower;
            effect_data.lifetime.is_consumable = false;
            effect_data.lifetime.duration_time_ms = kTimeInfinite;
            effect_data.lifetime.activated_by = AbilityType::kAttack;
            effect_data.attached_effects = {
                EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(15_fp))};
            blue_shield_data.effects.push_back(effect_data);

            EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data);
            EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);
        }
    }

    void SetUpEventListeners()
    {
        // Subscribe to all the events
        world->SubscribeToEvent(
            EventType::kShieldCreated,
            [this](const Event& event)
            {
                events_created_shield.emplace_back(event.Get<event_data::ShieldCreated>());
                shields_created++;
            });
        world->SubscribeToEvent(
            EventType::kShieldWasHit,
            [this](const Event& event)
            {
                events_shield_hit.emplace_back(event.Get<event_data::ShieldWasHit>());
                shields_hit++;
            });
        world->SubscribeToEvent(
            EventType::kShieldExpired,
            [this](const Event& event)
            {
                events_other_shield.emplace_back(event.Get<event_data::Shield>());
                shields_expired++;
            });
        world->SubscribeToEvent(
            EventType::kShieldDestroyed,
            [this](const Event& event)
            {
                const auto& event_data = event.Get<event_data::ShieldDestroyed>();
                events_destroyed_shield.emplace_back(event_data);
                shields_destroyed++;

                if (event_data.destruction_reason == DestructionReason::kExpired)
                {
                    shields_expired_reason++;
                }
                else if (event_data.destruction_reason == DestructionReason::kDamaged)
                {
                    shields_damaged_reason++;
                }
            });
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
        SetUpEventListeners();
    }

    Entity* blue_entity = nullptr;
    Entity* blue_entity2 = nullptr;
    Entity* blue_entity3 = nullptr;
    Entity* red_entity = nullptr;
    CombatUnitData data;

    // Listen to shield events
    std::vector<event_data::ShieldCreated> events_created_shield;
    std::vector<event_data::ShieldDestroyed> events_destroyed_shield;
    std::vector<event_data::Shield> events_other_shield;
    std::vector<event_data::ShieldWasHit> events_shield_hit;

    size_t GetSizeShieldEvents() const
    {
        return events_created_shield.size() + events_destroyed_shield.size() + events_other_shield.size() +
               events_shield_hit.size();
    }

    int shields_created = 0;
    int shields_hit = 0;
    int shields_expired = 0;
    int shields_expired_reason = 0;
    int shields_destroyed = 0;
    int shields_damaged_reason = 0;
};

class ShieldSystemTestWithEmpower : public ShieldSystemTest
{
    typedef ShieldSystemTest Super;

    // Add a delay of 100 ms between attacks
    FixedPoint GetAttackSpeed() const override
    {
        return 1000_fp;
    }
};

TEST_F(ShieldSystemTest, ApplyEffectToShieldsFullDestruction)
{
    // Spawn the Combat Units
    SpawnCombatUnits();

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    const auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(105_fp));

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

    // TimeStep
    world->TimeStep();

    // Manual calculation
    // 105 (effect) - 10 (grit) = 95 (10 armour, which makes the attack 90% effective) =
    // 86

    // All 5 shields should be destroyed
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should be reduced by only 36 because 50 was given to the shields
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "13.637"_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    // Check events
    // Should be 10 created + 5 hit + 0 destroyed + 2 Starting Shields
    EXPECT_EQ(GetSizeShieldEvents(), 15);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 5);
    EXPECT_EQ(shields_destroyed, 0);

    // TimeStep the world for destruction
    world->TimeStep();

    // Check events
    // Should be 10 created + 5 hit + 0 expired + 5 destroyed
    EXPECT_EQ(GetSizeShieldEvents(), 20);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 5);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 5);
    EXPECT_EQ(shields_expired_reason, 0);
    EXPECT_EQ(shields_damaged_reason, 5);

    // Let some time pass
    world->TimeStep();
    world->TimeStep();

    // Check events
    // Make sure shields didn't expire
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_expired_reason, 0);
}

TEST_F(ShieldSystemTest, ApplyEffectToShieldsPartialDestruction)
{
    SpawnCombatUnits();

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    const auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(40_fp));

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

    // TimeStep the world for destruction
    world->TimeStep();

    // Manual calculation
    // 40 (effect) - 10 (grit) = 30 (10 armour, which makes the attack 90% effective) =
    // 27

    // 2 shields should be destroyed and 1 damaged
    ASSERT_EQ(world->GetShieldTotal(*red_entity), "22.728"_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    // Check events
    // Should be 10 created + 5 hit + 0 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 15);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 5);
    EXPECT_EQ(shields_destroyed, 0);

    // TimeStep the world for destruction
    world->TimeStep();

    // Check events
    // Should be 10 created + 5 hit + 0 expired + 2 destroyed
    EXPECT_EQ(GetSizeShieldEvents(), 17);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 5);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 2);
    EXPECT_EQ(shields_expired_reason, 0);
    EXPECT_EQ(shields_damaged_reason, 2);

    // Let some time pass
    world->TimeStep();
    world->TimeStep();

    // Check events
    // Make sure shields didn't expire
    EXPECT_EQ(shields_expired, 0);
}

TEST_F(ShieldSystemTest, OnHitSkill)
{
    SpawnCombatUnitsEventSkill(EventType::kShieldWasHit, 200);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    const auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(40_fp));

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    // Blue entity applies effect to red entity
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        effect,
        effect_state);

    // TimeStep the systems
    world->TimeStep();

    // Manual calculation
    // 40 (effect) - 10 (grit) = 30 (10 armour, which makes the attack 90% effective) =
    // 27

    // 2 shields should be destroyed and 1 damaged
    ASSERT_EQ(world->GetShieldTotal(*red_entity), "22.728"_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // 5 shields are delivering 9 PM damage each from OnHit
    // Blue shields should only have 5 left
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), "4.55"_fp);
    // Health is boosted by the instant heal OnHit effect
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 110_fp);

    // Check events
    // Should be 10 created + 30 hit + 0 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 40);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 30);
    EXPECT_EQ(shields_destroyed, 0);

    // TimeStep again for destruction
    world->TimeStep();

    // Check events
    // Should be 10 created + 30 hit + 0 expired + 4 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 44);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 30);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 4);
    EXPECT_EQ(shields_expired_reason, 0);
    EXPECT_EQ(shields_damaged_reason, 4);

    // TimeStep again for shield expiration
    world->TimeStep();

    // Check events
    // 4 shields should have expired and another 2 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 50);
    EXPECT_EQ(shields_expired, 4);
    EXPECT_EQ(shields_destroyed, 6);
    EXPECT_EQ(shields_expired_reason, 0);
    EXPECT_EQ(shields_damaged_reason, 6);
}

TEST_F(ShieldSystemTest, OnExpireSkill)
{
    SpawnCombatUnitsEventSkill(EventType::kShieldExpired, 200, false);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // TimeStep the systems
    world->TimeStep();

    // Shields still intact
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Blue remains unchanged
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    // Check events
    // Should be 10 created + 0 hit + 0 expired + 0 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 10);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 0);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 0);

    // TimeStep the systems again, three times so the expiration can pass and trigger skill
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();

    // Shields expired
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Blue shield expired and health boosted by OnExpire skill
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    // Health boosted by 75 (5 shields * 15 each)
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 125_fp);

    // Check events
    // Should be 10 created + 0 hit + 10 expired + 10 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 30);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 0);
    EXPECT_EQ(shields_expired, 10);
    EXPECT_EQ(shields_destroyed, 10);
    EXPECT_EQ(shields_expired_reason, 10);
    EXPECT_EQ(shields_damaged_reason, 0);
}

TEST_F(ShieldSystemTest, DamageOnExpireSkill)
{
    SpawnCombatUnitsEventSkillWithCrit(EventType::kShieldExpired, 200);
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.SetCritChanceOverflowPercentage(100_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCritAmplificationPercentage, 150_fp);

    // Add empower to confirm shield can_crit
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = false;
    effect_data.lifetime.duration_time_ms = -1;
    effect_data.lifetime.activated_by = AbilityType::kAttack;
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(red_entity->GetID());

    effect_data.attached_effect_package_attributes.can_crit = true;

    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), effect_data, effect_state);

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 10_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 10_fp);

    // TimeStep the systems
    world->TimeStep();

    // Shields still intact
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 10_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    // Blue remains unchanged
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 10_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);

    // Check events
    // Should be 2 created + 0 hit + 0 expired + 0 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 2);
    EXPECT_EQ(shields_created, 2);
    EXPECT_EQ(shields_hit, 0);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 0);

    // TimeStep the systems again, three times so the expiration can pass and trigger skill
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();

    // Shields expired
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    // 150 damage should be done, 100 from shield damage and 50 from crit
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 350_fp);

    // Check events
    // Should be 2 created + 0 hit + 2 expired + 2 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 6);
    EXPECT_EQ(shields_created, 2);
    EXPECT_EQ(shields_hit, 0);
    EXPECT_EQ(shields_expired, 2);
    EXPECT_EQ(shields_destroyed, 2);
    EXPECT_EQ(shields_expired_reason, 2);
    EXPECT_EQ(shields_damaged_reason, 0);
}

TEST_F(ShieldSystemTest, OnDestroySkill)
{
    // Use PendingDestroyed to hook skill otherwise there's no shield
    SpawnCombatUnitsEventSkill(EventType::kShieldPendingDestroyed, 200);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    const auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(40_fp));

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

    // TimeStep the systems
    world->TimeStep();

    // Manual calculation
    // 40 (effect) - 10 (grit) = 30 (10 armour, which makes the attack 90% effective) =
    // 27

    // 2 shields should be destroyed and 1 damaged
    ASSERT_EQ(world->GetShieldTotal(*red_entity), "22.728"_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Blue remains unchanged
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    // Check events
    // Should be 10 created + 5 hit + 0 expired + 0 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 15);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 5);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 0);

    // TimeStep again for OnDestroy and destruction
    world->TimeStep();

    // Red shield value still the same
    ASSERT_EQ(world->GetShieldTotal(*red_entity), "22.728"_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Red shield does OnDestroy damage of 9 to blue
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), "31.820"_fp);
    // No heal because shield is still up
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    // Check events
    // Should be 10 created + 15 hit + 0 expired + 2 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 27);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 15);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 2);
    EXPECT_EQ(shields_expired_reason, 0);
    EXPECT_EQ(shields_damaged_reason, 2);

    // TimeStep again to expire shields
    world->TimeStep();

    // Check events
    // Should be 10 created + 15 hit + 8 expired + 2 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 35);
    EXPECT_EQ(shields_created, 10);
    EXPECT_EQ(shields_hit, 15);
    EXPECT_EQ(shields_expired, 8);
    EXPECT_EQ(shields_destroyed, 2);
    EXPECT_EQ(shields_expired_reason, 0);
    EXPECT_EQ(shields_damaged_reason, 2);
}

TEST_F(ShieldSystemTest, UseShieldStatInExpression)
{
    auto apply_damage = [&](const Entity* sender, const Entity* receiver, const EffectExpression& expression)
    {
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(*sender);
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            sender->GetID(),
            receiver->GetID(),
            EffectData::CreateDamage(EffectDamageType::kPhysical, expression),
            effect_state);
    };

    EventHistory<EventType::kOnHealthGain> health_gains(*world);

    data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);
    data.type_data.stats.Set(StatType::kGrit, 0_fp);
    data.type_data.stats.Set(StatType::kPhysicalResist, 0_fp);
    // Spawn two combat units
    SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
    SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);

    // Add shields
    {
        ShieldData blue_shield_data, red_shield_data;
        blue_shield_data.sender_id = blue_entity->GetID();
        blue_shield_data.receiver_id = blue_entity->GetID();

        red_shield_data.sender_id = red_entity->GetID();
        red_shield_data.receiver_id = red_entity->GetID();

        blue_shield_data.value = 1000_fp;
        blue_shield_data.duration_ms = 200;

        red_shield_data.value = 1000_fp;
        red_shield_data.duration_ms = 200;

        // Add a blue skill
        // Heals self
        auto& skill = blue_shield_data.event_skills[EventType::kShieldWasHit];
        skill = SkillData::Create();
        skill->targeting.type = SkillTargetingType::kSelf;
        skill->deployment.type = SkillDeploymentType::kDirect;

        TestingDataLoader data_loader(world->GetLogger());
        const auto opt_expr = data_loader.ParseAndLoadExpression(
            R"({
            "Operation": "/",
            "Operands": [
                {
                    "Stat": "$ShieldAmount",
                    "StatSource": "Sender"
                },
                4
            ]
        })");

        ASSERT_TRUE(opt_expr.has_value());

        // When shield being hit it heals 25% of remaining shield capacity to owner
        skill->AddEffect(EffectData::CreateHeal(EffectHealType::kNormal, opt_expr.value()));
        EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data);
        EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);
    }

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(1000_fp);
    red_stats_component.SetCurrentHealth(1000_fp);

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 1000_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 1000_fp);

    apply_damage(red_entity, blue_entity, EffectExpression::FromValue(100_fp));

    // TimeStep the systems
    world->TimeStep();

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 900_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 1000_fp);

    // Check events
    // Should be 10 created + 5 hit + 0 expired + 0 destroyed
    EXPECT_EQ(shields_created, 2);
    EXPECT_EQ(shields_hit, 1);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 0);

    world->TimeStep();
}

TEST_F(ShieldSystemTest, UseShieldAmountInExpression)
{
    data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);
    data.type_data.stats.Set(StatType::kGrit, 0_fp);
    data.type_data.stats.Set(StatType::kPhysicalResist, 0_fp);
    data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

    TestingDataLoader data_loader(world->GetLogger());

    {
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = data.type_data.attack_abilities.AddAbility();
        auto& skill = ability.AddSkill();
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.deployment.type = SkillDeploymentType::kDirect;

        const auto expression = data_loader.ParseAndLoadExpression(
            R"({
            "Operation": "/",
            "Operands": [
                {
                    "Stat": "$ShieldAmount",
                    "StatSource": "Receiver"
                },
                2
            ]
        })");
        ASSERT_TRUE(expression.has_value());

        skill.AddDamageEffect(EffectDamageType::kPhysical, *expression);
    }

    // Spawn two combat units
    SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
    SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);

    Stun(*red_entity);

    // Add 3 shields to red entity with amount of 1000 (3000 total) and 200ms duration
    {
        ShieldData shield_data;
        shield_data.value = 1000_fp;
        shield_data.duration_ms = 200;

        for (size_t i = 0; i != 3; ++i)
        {
            shield_data.sender_id = red_entity->GetID();
            shield_data.receiver_id = red_entity->GetID();
            EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, shield_data);
        }
    }

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 3000_fp);

    // TimeStep the systems
    world->TimeStep();

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 1500_fp);

    EXPECT_EQ(shields_created, 3);
    EXPECT_EQ(shields_hit, 3);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 0);
}

TEST_F(ShieldSystemTest, ShieldBuff)
{
    SpawnCombatUnitsWithShields(200);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Create effect
    const auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(35_fp));

    // TimeStep the game systems
    world->TimeStep();

    // Red should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    // Confirm current stats
    // Blue boosted by buff
    EXPECT_EQ(world->GetLiveStats(blue_entity->GetID()).Get(StatType::kOmegaPowerPercentage), 350_fp);
    EXPECT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kOmegaPowerPercentage), 100_fp);

    // Confirm current attack physical damage
    EXPECT_EQ(world->GetLiveStats(blue_entity->GetID()).Get(StatType::kAttackPhysicalDamage), 5_fp);
    // Red boosted by buff
    EXPECT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kAttackPhysicalDamage), 105_fp);

    // TimeStep some more to let shields expire and be destroyed
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();

    // Shields should be gone
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);

    // Confirm current stats
    // Blue no longer boosted
    EXPECT_EQ(world->GetLiveStats(blue_entity->GetID()).Get(StatType::kOmegaPowerPercentage), 100_fp);
    EXPECT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kOmegaPowerPercentage), 100_fp);

    // Confirm current attack physical damage
    EXPECT_EQ(world->GetLiveStats(blue_entity->GetID()).Get(StatType::kAttackPhysicalDamage), 5_fp);
    // Red no longer boosted
    EXPECT_EQ(world->GetLiveStats(red_entity->GetID()).Get(StatType::kAttackPhysicalDamage), 5_fp);
}

TEST_F(ShieldSystemTest, ShieldGainEfficiencyPercentage)
{
    SpawnCombatUnitsWithShields();

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kShieldGainEfficiencyPercentage, 150_fp);
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    red_stats_component.GetMutableTemplateStats().Set(StatType::kShieldGainEfficiencyPercentage, 150_fp);

    const auto add_shield = [this](Entity& entity, const FixedPoint& amount)
    {
        const auto effect_data = EffectData::CreateSpawnShield(EffectExpression::FromValue(amount), kTimeInfinite);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(entity.GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(entity, entity.GetID(), effect_data, effect_state);
    };

    add_shield(*blue_entity, 50_fp);
    add_shield(*red_entity, 60_fp);

    world->TimeStep();

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 125_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 140_fp);
}

TEST_F(ShieldSystemTest, ShieldGainEfficiencyPercentageFromBuff)
{
    SpawnCombatUnitsWithShields();

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    const auto add_shield_gain_efficiency = [this](Entity& entity, const FixedPoint amount)
    {
        const auto effect_data = EffectData::CreateBuff(
            StatType::kShieldGainEfficiencyPercentage,
            EffectExpression::FromValue(amount),
            kTimeInfinite);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(entity.GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(entity, entity.GetID(), effect_data, effect_state);
    };

    const auto add_shield = [this](Entity& entity, const FixedPoint& amount)
    {
        const auto effect_data = EffectData::CreateSpawnShield(EffectExpression::FromValue(amount), kTimeInfinite);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(entity.GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(entity, entity.GetID(), effect_data, effect_state);
    };

    auto get_shield_gain_efficiency = [this](const Entity& entity)
    {
        return world->GetLiveStats(entity).Get(StatType::kShieldGainEfficiencyPercentage);
    };

    ASSERT_EQ(get_shield_gain_efficiency(*blue_entity), 100_fp);
    add_shield_gain_efficiency(*blue_entity, 50_fp);
    ASSERT_EQ(get_shield_gain_efficiency(*blue_entity), 150_fp);

    add_shield(*blue_entity, 50_fp);

    ASSERT_EQ(get_shield_gain_efficiency(*red_entity), 100_fp);
    add_shield_gain_efficiency(*red_entity, 50_fp);
    ASSERT_EQ(get_shield_gain_efficiency(*red_entity), 150_fp);

    add_shield(*red_entity, 60_fp);

    world->TimeStep();

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 125_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 140_fp);
}

TEST_F(ShieldSystemTestWithEmpower, ShieldEmpower)
{
    SpawnCombatUnitsEmpower(200);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Reduce health from starting value of MaxHealth
    blue_stats_component.SetCurrentHealth(50_fp);
    red_stats_component.SetCurrentHealth(50_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // TimeStep the game systems
    world->TimeStep();

    // Red should be down to 25 health with no shields (75 pure damage from 5 shields * 15 each)
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 25_fp);

    // Blue should be unaffected
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);
}

TEST_F(ShieldSystemTest, StartingShield)
{
    SpawnCombatUnit(Team::kBlue, {-17, -35}, data, blue_entity);

    auto& stats_component = blue_entity->Get<StatsComponent>();
    stats_component.GetMutableTemplateStats().Set(StatType::kStartingShield, 10_fp);

    // TimeStep
    world->TimeStep();

    // Starting shield value is 10, expect to create 1 shield
    EXPECT_EQ(shields_created, 1);
    EXPECT_EQ(world->GetShieldTotal(*blue_entity), 10_fp);

    // Test creating an combat unit with Starting Shield 0
    SpawnCombatUnit(Team::kRed, {-22, -45}, data, red_entity);

    // reset shield counter
    shields_created = 0;

    // TimeStep
    world->TimeStep();

    EXPECT_EQ(shields_created, 0);
    EXPECT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Create effect
    const auto effect = EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(20_fp));

    // Apply Effect

    EffectState effect_state;
    effect_state.is_critical = false;
    effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        red_entity->GetID(),
        blue_entity->GetID(),
        effect,
        effect_state);

    // TimeStep
    world->TimeStep();

    // Check starting shield not destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 2);
    EXPECT_EQ(shields_created, 0);
    EXPECT_EQ(shields_hit, 1);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 0);
    EXPECT_EQ(shields_expired_reason, 0);
    EXPECT_EQ(shields_damaged_reason, 0);
}

TEST_F(ShieldSystemTest, StartingShieldDuration)
{
    SpawnCombatUnit(Team::kBlue, {-2, -5}, data, blue_entity);

    auto& stats_component = blue_entity->Get<StatsComponent>();
    stats_component.GetMutableTemplateStats().Set(StatType::kStartingShield, 10_fp);

    // TimeStep
    world->TimeStep();

    // Starting shield value is 10, expect to create 1 shield
    EXPECT_EQ(shields_created, 1);
    EXPECT_EQ(world->GetShieldTotal(*blue_entity), 10_fp);

    // Test Starting Shield duration
    const auto& attached_entity_components = blue_entity->Get<AttachedEntityComponent>();
    const AttachedEntity& attached_entity = attached_entity_components.GetAttachedEntities()[0];
    ASSERT_EQ(attached_entity.type, AttachedEntityType::kShield);
    const auto& shield_entity = world->GetByID(attached_entity.id);
    const auto& duration_component = shield_entity.Get<DurationComponent>();
    EXPECT_EQ(duration_component.GetDurationMs(), kTimeInfinite);
}

TEST_F(ShieldSystemTest, ThornsHitOnShieldExpireDamage)
{
    SpawnCombatUnitsEventSkillWithCrit(EventType::kShieldExpired, 200);
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kThorns, 25_fp);

    // Add empower to confirm shield can_crit
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = false;
    effect_data.lifetime.duration_time_ms = -1;
    effect_data.lifetime.activated_by = AbilityType::kAttack;
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(red_entity->GetID());

    effect_data.attached_effect_package_attributes.can_crit = true;

    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), effect_data, effect_state);

    // Assume health/shield is at maximum
    ASSERT_EQ(blue_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);
    ASSERT_EQ(red_stats_component.GetBaseValueForType(StatType::kMaxHealth), 500_fp);

    // Ensure health and shield values correct
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 10_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 10_fp);

    // TimeStep the systems
    world->TimeStep();

    // Shields still intact
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 10_fp);

    // Health should not be reduced
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    // Blue remains unchanged
    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 10_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 500_fp);

    // Check events
    // Should be 2 created + 0 hit + 0 expired + 0 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 2);
    EXPECT_EQ(shields_created, 2);
    EXPECT_EQ(shields_hit, 0);
    EXPECT_EQ(shields_expired, 0);
    EXPECT_EQ(shields_destroyed, 0);

    // TimeStep the systems again, three times so the expiration can pass and trigger skill
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();

    // Shields expired
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 0_fp);

    // Health reduced by 15, 25 thorn damage - 15 grit
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 485_fp);

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 0_fp);
    // 100 damage should be done from shield damage
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 400_fp);

    // Check events
    // Should be 2 created + 0 hit + 2 expired + 2 destroyed
    ASSERT_EQ(GetSizeShieldEvents(), 7);
    EXPECT_EQ(shields_created, 2);
    EXPECT_EQ(shields_hit, 1);
    EXPECT_EQ(shields_expired, 2);
    EXPECT_EQ(shields_destroyed, 2);
    EXPECT_EQ(shields_expired_reason, 2);
    EXPECT_EQ(shields_damaged_reason, 0);
}

TEST_F(ShieldSystemTest, ShieldBurnEffect)
{
    auto& ability = data.type_data.innate_abilities.AddAbility();
    ability.total_duration_ms = 0;
    ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;
    auto& skill = ability.AddSkill();
    skill.deployment.type = SkillDeploymentType::kDirect;
    skill.targeting.type = SkillTargetingType::kDistanceCheck;
    skill.targeting.num = 1;
    skill.targeting.group = AllegianceType::kEnemy;
    skill.targeting.lowest = false;
    skill.AddEffect(EffectData::CreateShieldBurn(EffectExpression::FromValue(42_fp)));

    SpawnCombatUnitsWithShields(2000);

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 50_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    world->TimeStep();

    ASSERT_EQ(world->GetShieldTotal(*blue_entity), 8_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 8_fp);
}

}  // namespace simulation
