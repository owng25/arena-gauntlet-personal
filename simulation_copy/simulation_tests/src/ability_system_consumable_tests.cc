#include "ability_system_data_fixtures.h"
#include "base_test_fixtures.h"
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
TEST_F(AbilitySystemTest, EmpowerWithDurationAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.SetCurrentEnergy(0_fp);
    red_stats_component.SetCurrentEnergy(0_fp);

    ////////////////////////////////////////////////////////////////////////////////
    // Add empower and effect of pure damage
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = false;
    effect_data.lifetime.duration_time_ms = 2200;
    effect_data.lifetime.activated_by = AbilityType::kAttack;
    effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp))};
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_apply_effect.size(), 3);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPure)
        << "Did not find the pure damage from the empower";
    events_apply_effect.clear();

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Red should be lower because of the pure damage empower from blue
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 900_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 800_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Still reduced by empower attack ability
    ASSERT_EQ(events_apply_effect.size(), 3);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPure)
        << "Did not find the pure damage from the empower";
    events_apply_effect.clear();
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 600_fp);

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Expire empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Should have not found any pure damage from empower
    // for (const auto& event_data : events_apply_effect)
    // {
    //     ASSERT_NE(event_data.data.type_id.damage_type, EffectDamageType::kPure);
    // }

    // Should not exist anymore
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemTest, EmpowerWithConsumableAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_mutable_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_mutable_stats = red_stats_component.GetMutableTemplateStats();

    red_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    blue_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);

    // Add empower and effect of pure damage
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = true;
    effect_data.lifetime.activations_until_expiry = 1;
    effect_data.lifetime.duration_time_ms = 10000;  // Should be ignored
    effect_data.lifetime.activated_by = AbilityType::kAttack;
    effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp))};
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_apply_effect.size(), 3);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPure)
        << "Did not find the pure damage from the empower";
    events_apply_effect.clear();

    // Empowers should be dead after 1 activation
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);

    // Red should be lower because of the pure damage empower from blue
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 900_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 800_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Reduced by normal attack ability
    ASSERT_EQ(events_apply_effect.size(), 2);
    events_apply_effect.clear();
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 700_fp);

    // Empowers should still be dead
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);

    // TimeStep some  more
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Should have not found any pure damage from empower
    for (const auto& event_data : events_apply_effect)
    {
        ASSERT_NE(event_data.data.type_id.damage_type, EffectDamageType::kPure);
    }

    // Still dead
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemTest, EmpowerWithConsumableAttackCritical)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_mutable_stats = red_stats_component.GetMutableTemplateStats();
    red_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    red_mutable_stats.Set(StatType::kCritAmplificationPercentage, 100_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    auto& blue_mutable_stats = blue_stats_component.GetMutableTemplateStats();
    blue_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    blue_mutable_stats.Set(StatType::kCritAmplificationPercentage, 150_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);

    // Add empower and effect of pure damage
    {
        EffectData effect_data;
        effect_data.type_id.type = EffectType::kEmpower;
        effect_data.lifetime.is_consumable = true;
        effect_data.lifetime.activations_until_expiry = 1;
        effect_data.lifetime.duration_time_ms = 10000;  // Should be ignored
        effect_data.lifetime.activated_by = AbilityType::kAttack;
        effect_data.attached_effects = {
            EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(200_fp))};

        effect_data.attached_effect_package_attributes.always_crit = true;
        effect_data.attached_effect_package_attributes.rotate_to_target = true;

        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);
        ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);
    }

    // Listen to the apply effect events
    EventHistory<EventType::kOnDamage> on_damage(*world);

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(on_damage.Size(), 3);
    EXPECT_EQ(on_damage[0].damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(on_damage[0].damage_value, 150_fp);
    EXPECT_EQ(on_damage[0].is_critical, true);
    EXPECT_EQ(on_damage[0].sender_id, blue_entity->GetID());
    EXPECT_EQ(on_damage[0].receiver_id, red_entity->GetID());

    EXPECT_EQ(on_damage[1].damage_type, EffectDamageType::kPure);
    EXPECT_EQ(on_damage[1].damage_value, 300_fp);
    EXPECT_EQ(on_damage[1].is_critical, true);
    EXPECT_EQ(on_damage[1].sender_id, blue_entity->GetID());
    EXPECT_EQ(on_damage[1].receiver_id, red_entity->GetID());

    EXPECT_EQ(on_damage[2].damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(on_damage[2].damage_value, 100_fp);
    EXPECT_EQ(on_damage[2].is_critical, false);
    EXPECT_EQ(on_damage[2].sender_id, red_entity->GetID());
    EXPECT_EQ(on_damage[2].receiver_id, blue_entity->GetID());

    on_damage.Clear();

    // Empowers should be dead after 1 activation
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);

    // Red should be lower because of the pure damage empower from blue
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 900_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 550_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Reduced by normal attack ability
    ASSERT_EQ(on_damage.Size(), 2);
    on_damage.Clear();
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 450_fp);

    // Empowers should still be dead
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);

    // TimeStep some more
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 350_fp);

    // Still dead
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemTestWithStartOmegaOneSkill, EmpowerWithDurationOmega)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Add empower and effect of pure damage
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = false;
    effect_data.lifetime.duration_time_ms = 2200;
    effect_data.lifetime.activated_by = AbilityType::kOmega;
    effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp))};
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_apply_effect.size(), 3);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.type, EffectType::kInstantDamage);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPure)
        << "Did not find the pure damage from the empower";
    events_apply_effect.clear();

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Red should be lower because of the pure damage empower from blue
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 700_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Reduced by normal attack ability
    ASSERT_EQ(events_apply_effect.size(), 2);
    events_apply_effect.clear();
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 700_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 600_fp);

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Should have not found any pure damage from empower
    for (const auto& event_data : events_apply_effect)
    {
        ASSERT_NE(event_data.data.type_id.damage_type, EffectDamageType::kPure);
    }

    // Should not exist anymore
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemTest, TwoEmpowerWithDurationAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.SetCurrentEnergy(0_fp);
    red_stats_component.SetCurrentEnergy(0_fp);

    ////////////////////////////////////////////////////////////////////////////////
    // Add empower and effect of pure damage
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = false;
    effect_data.lifetime.duration_time_ms = 2000;
    effect_data.lifetime.activated_by = AbilityType::kAttack;
    effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp))};
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    // Add stronger empower
    EffectData priority_effect_data;
    priority_effect_data.type_id.type = EffectType::kEmpower;
    priority_effect_data.lifetime.is_consumable = false;
    priority_effect_data.lifetime.duration_time_ms = 2200;
    priority_effect_data.lifetime.activated_by = AbilityType::kAttack;
    priority_effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(200_fp))};
    EffectState priority_effect_state{};
    priority_effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    GetAttachedEffectsHelper()
        .AddAttachedEffect(*blue_entity, blue_entity->GetID(), priority_effect_data, priority_effect_state);

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_apply_effect.size(), 4);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPure)
        << "Did not find the pure damage from the empower";
    events_apply_effect.clear();

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 2);

    // Red should be lower because of the pure damage empower from blue
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 900_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 600_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Still reduced by empower attack ability
    ASSERT_EQ(events_apply_effect.size(), 4);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPure)
        << "Did not find the pure damage from the empower";
    events_apply_effect.clear();
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 200_fp);

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Expire empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Should have not found any pure damage from empower
    // for (const auto& event_data : events_apply_effect)
    // {
    //     ASSERT_NE(event_data.data.type_id.damage_type, EffectDamageType::kPure);
    // }

    // Should not exist anymore
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemTest, TwoEmpowerStrongerExpire)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.SetCurrentEnergy(0_fp);
    red_stats_component.SetCurrentEnergy(0_fp);

    ////////////////////////////////////////////////////////////////////////////////
    // Add empower and effect of pure damage
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = false;
    effect_data.lifetime.duration_time_ms = 2200;
    effect_data.lifetime.activated_by = AbilityType::kAttack;
    effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp))};
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    // Add higher value effect with lower duration, should not fire
    EffectData priority_effect_data;
    priority_effect_data.type_id.type = EffectType::kEmpower;
    priority_effect_data.lifetime.is_consumable = false;
    priority_effect_data.lifetime.duration_time_ms = 2100;
    priority_effect_data.lifetime.activated_by = AbilityType::kAttack;
    priority_effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(200_fp))};
    EffectState priority_effect_state{};
    priority_effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    GetAttachedEffectsHelper()
        .AddAttachedEffect(*blue_entity, blue_entity->GetID(), priority_effect_data, priority_effect_state);

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_apply_effect.size(), 4);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPure)
        << "Did not find the pure damage from the empower";
    events_apply_effect.clear();

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 2);

    // Red should be lower because of the pure damage empower from blue
    // Stronger empower should not take effect since shorter has a longer duration
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 900_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 600_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Still reduced by empower attack ability
    ASSERT_EQ(events_apply_effect.size(), 4);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPure)
        << "Did not find the pure damage from the empower";
    events_apply_effect.clear();
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 200_fp);

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 2);

    // Expire empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Weaker effect should now take effect
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 0_fp);
}

TEST_F(AbilitySystemTest, BuffWithConsumableAttack)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_mutable_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_mutable_stats = red_stats_component.GetMutableTemplateStats();
    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();

    red_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    blue_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    blue_mutable_stats.Set(StatType::kGrit, 500_fp);

    // Before any debuff
    StatsData blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    ASSERT_EQ(blue_live_stats.Get(StatType::kGrit), 500_fp);

    // Add buff
    {
        EffectData effect_data = EffectData::CreateBuff(StatType::kGrit, EffectExpression::FromValue(100_fp), 10000);
        effect_data.lifetime.is_consumable = true;
        effect_data.lifetime.activations_until_expiry = 2;
        effect_data.lifetime.activated_by = AbilityType::kAttack;
        EffectState effect_state{};

        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        ;
        effect_state.source_context.ability_name = "Ability1";

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);
    }

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    // Buff should be active
    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    EXPECT_EQ(blue_live_stats.Get(StatType::kGrit), 600_fp);
    EXPECT_TRUE(blue_attached_efects_component.HasBuffFor(StatType::kGrit));

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Still active
    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    EXPECT_EQ(blue_live_stats.Get(StatType::kGrit), 600_fp);
    EXPECT_TRUE(blue_attached_efects_component.HasBuffFor(StatType::kGrit));

    // Add another
    {
        EffectData effect_data = EffectData::CreateBuff(StatType::kGrit, EffectExpression::FromValue(100_fp), 10000);
        effect_data.lifetime.is_consumable = true;
        effect_data.lifetime.activations_until_expiry = 1;
        effect_data.lifetime.activated_by = AbilityType::kAttack;
        EffectState effect_state{};
        effect_state.source_context.ability_name = "Ability2";

        blue_live_stats = world->GetLiveStats(blue_entity->GetID());
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
        ;

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);
    }

    // Higher
    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    EXPECT_EQ(blue_live_stats.Get(StatType::kGrit), 700_fp);
    EXPECT_TRUE(blue_attached_efects_component.HasBuffFor(StatType::kGrit));

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Should be dead
    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    EXPECT_EQ(blue_live_stats.Get(StatType::kGrit), 500_fp);
    EXPECT_FALSE(blue_attached_efects_component.HasBuffFor(StatType::kGrit));

    // One more
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Still nope
    blue_live_stats = world->GetLiveStats(blue_entity->GetID());
    EXPECT_EQ(blue_live_stats.Get(StatType::kGrit), 500_fp);
    EXPECT_FALSE(blue_attached_efects_component.HasBuffFor(StatType::kGrit));
}

class AbilitySystemTestWithFullEmpower : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
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

            {
                auto vampiric_effect_damage =
                    EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
                vampiric_effect_damage.attributes.missing_health_percentage_to_health = 10_fp;

                skill1.effect_package.AddEffect(vampiric_effect_damage);
            }
            skill1.effect_package.attributes.rotate_to_target = true;

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

TEST_F(AbilitySystemTestWithFullEmpower, EmpowerMergeFull)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.SetCurrentEnergy(0_fp);
    red_stats_component.SetCurrentEnergy(0_fp);

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    ////////////////////////////////////////////////////////////////////////////////
    // Add empowers
    {
        // Add an attached effect with an attached effect package attribute
        EffectData effect_data;
        effect_data.type_id.type = EffectType::kEmpower;
        effect_data.lifetime.is_consumable = true;
        effect_data.lifetime.activations_until_expiry = 1;
        effect_data.lifetime.activated_by = AbilityType::kAttack;
        effect_data.attached_effects = {
            EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(50_fp))};

        effect_data.attached_effects.at(0).attributes.cleanse_bots = true;
        effect_data.attached_effect_package_attributes.can_crit = true;
        effect_data.attached_effect_package_attributes.exploit_weakness = true;
        effect_data.attached_effect_package_attributes.piercing_percentage = EffectExpression::FromValue(5_fp);

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);
    }
    {
        // Add just an attribute
        EffectData effect_data;
        effect_data.type_id.type = EffectType::kEmpower;
        effect_data.lifetime.is_consumable = true;
        effect_data.lifetime.activations_until_expiry = 1;
        effect_data.lifetime.activated_by = AbilityType::kAttack;

        effect_data.attached_effect_package_attributes.rotate_to_target = true;
        effect_data.attached_effect_package_attributes.split_damage = true;

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);
    }
    {
        // Add a global effect attribute
        EffectDataAttributes global_attributes;
        global_attributes.missing_health_percentage_to_health = 3_fp;

        EffectTypeID empower_for_effect_type_id;
        empower_for_effect_type_id.type = EffectType::kInstantDamage;
        empower_for_effect_type_id.damage_type = EffectDamageType::kPhysical;

        const auto global_effect_attribute =
            CreateEmpowerGlobalEffectAttribute(AbilityType::kAttack, 1, empower_for_effect_type_id, global_attributes);

        GetAttachedEffectsHelper()
            .AddAttachedEffect(*blue_entity, blue_entity->GetID(), global_effect_attribute, effect_state);
    }
    {
        // Add a global effect attribute that does not affect anything
        EffectDataAttributes global_attributes;
        global_attributes.max_health_percentage_to_health = 50_fp;

        EffectTypeID empower_for_effect_type_id;
        empower_for_effect_type_id.type = EffectType::kInstantDamage;
        empower_for_effect_type_id.damage_type = EffectDamageType::kEnergy;

        const auto global_effect_attribute =
            CreateEmpowerGlobalEffectAttribute(AbilityType::kAttack, 1, empower_for_effect_type_id, global_attributes);

        GetAttachedEffectsHelper()
            .AddAttachedEffect(*blue_entity, blue_entity->GetID(), global_effect_attribute, effect_state);
    }
    {
        EffectDataAttributes global_attributes;

        EffectTypeID empower_for_effect_type_id;
        empower_for_effect_type_id.type = EffectType::kInstantDamage;
        empower_for_effect_type_id.damage_type = EffectDamageType::kPhysical;

        auto global_effect_attribute =
            CreateEmpowerGlobalEffectAttribute(AbilityType::kAttack, 1, empower_for_effect_type_id, global_attributes);

        GetAttachedEffectsHelper()
            .AddAttachedEffect(*blue_entity, blue_entity->GetID(), global_effect_attribute, effect_state);
    }

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_effect_package_received.size(), 2);

    // Test effect package attributes
    {
        const event_data::EffectPackage& effect_package_event_data = events_effect_package_received.at(0);

        EXPECT_EQ(effect_package_event_data.sender_id, blue_entity->GetID());
        EXPECT_EQ(effect_package_event_data.receiver_id, red_entity->GetID());
        EXPECT_EQ(effect_package_event_data.ability_type, AbilityType::kAttack);
        EXPECT_EQ(effect_package_event_data.attributes.can_crit, true);
        EXPECT_EQ(effect_package_event_data.attributes.exploit_weakness, true);
        EXPECT_EQ(effect_package_event_data.attributes.rotate_to_target, true);
        EXPECT_EQ(effect_package_event_data.attributes.split_damage, true);
        EXPECT_EQ(EvaluateNoStats(effect_package_event_data.attributes.piercing_percentage), 5_fp);
    }

    // Test effect data
    ASSERT_EQ(events_apply_effect.size(), 3);
    {
        // Effect from attack ability
        const event_data::Effect& effect_event_data = events_apply_effect.at(0);
        const EffectData& effect_data = effect_event_data.data;
        const EffectDataAttributes& effect_data_attributes = effect_data.attributes;

        EXPECT_EQ(effect_event_data.sender_id, blue_entity->GetID());
        EXPECT_EQ(effect_event_data.receiver_id, red_entity->GetID());

        EXPECT_EQ(effect_data.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect_data.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect_data.GetExpression().base_value.value, 100_fp);

        // Test attributes
        EXPECT_EQ(effect_data_attributes.cleanse_bots, false);
        EXPECT_EQ(effect_data_attributes.missing_health_percentage_to_health, 13_fp);
        EXPECT_EQ(effect_data_attributes.max_health_percentage_to_health, 0_fp);
    }
    {
        // Effect from empower
        const event_data::Effect& effect_event_data = events_apply_effect.at(1);
        const EffectData& effect_data = effect_event_data.data;
        const EffectDataAttributes& effect_data_attributes = effect_data.attributes;

        EXPECT_EQ(effect_event_data.sender_id, blue_entity->GetID());
        EXPECT_EQ(effect_event_data.receiver_id, red_entity->GetID());

        EXPECT_EQ(effect_data.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect_data.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect_data.GetExpression().base_value.value, 50_fp);

        // Test attributes
        EXPECT_EQ(effect_data_attributes.cleanse_bots, true);
        EXPECT_EQ(effect_data_attributes.missing_health_percentage_to_health, 3_fp);
        EXPECT_EQ(effect_data_attributes.max_health_percentage_to_health, 0_fp);
    }

    events_apply_effect.clear();

    // No empowers
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

class AbilitySystemTestWithFullDisempower : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
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

            {
                auto vampiric_effect_damage =
                    EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
                vampiric_effect_damage.attributes.missing_health_percentage_to_health = 10_fp;

                skill1.effect_package.AddEffect(vampiric_effect_damage);
            }
            skill1.effect_package.attributes.rotate_to_target = true;
            skill1.effect_package.attributes.damage_amplification = EffectExpression::FromValue(50_fp);

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

TEST_F(AbilitySystemTestWithFullDisempower, DisempowerMergeFull)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    blue_stats_component.SetCurrentEnergy(0_fp);
    red_stats_component.SetCurrentEnergy(0_fp);

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    ////////////////////////////////////////////////////////////////////////////////
    // Add empowers
    {
        // Add an attached effect with an attached effect package attribute
        EffectData effect_data;
        effect_data.type_id.type = EffectType::kEmpower;
        effect_data.lifetime.is_consumable = true;
        effect_data.lifetime.activations_until_expiry = 1;
        effect_data.lifetime.activated_by = AbilityType::kAttack;
        effect_data.attached_effects = {
            EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(50_fp))};

        // Add 15
        effect_data.attached_effect_package_attributes.damage_amplification = EffectExpression::FromValue(15_fp);

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);
    }
    // Add disempowers
    {
        // Add an attached effect with an attached effect package attribute
        EffectData effect_data;
        effect_data.type_id.type = EffectType::kDisempower;
        effect_data.lifetime.is_consumable = true;
        effect_data.lifetime.activations_until_expiry = 1;
        effect_data.lifetime.activated_by = AbilityType::kAttack;

        // Subtract 40
        effect_data.attached_effect_package_attributes.damage_amplification = EffectExpression::FromValue(40_fp);

        effect_data.attached_effect_package_attributes.energy_damage_amplification = EffectExpression::FromValue(20_fp);

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity->GetID(), effect_data, effect_state);
    }

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_effect_package_received.size(), 2);

    // Test effect package attributes
    {
        const event_data::EffectPackage& effect_package_event_data = events_effect_package_received.at(0);

        EXPECT_EQ(effect_package_event_data.sender_id, blue_entity->GetID());
        EXPECT_EQ(effect_package_event_data.receiver_id, red_entity->GetID());
        EXPECT_EQ(effect_package_event_data.ability_type, AbilityType::kAttack);
        EXPECT_EQ(effect_package_event_data.attributes.rotate_to_target, true);

        // 50 + 15 - 40
        EXPECT_EQ(EvaluateNoStats(effect_package_event_data.attributes.damage_amplification), 25_fp);

        // Negative damage amplifications
        EXPECT_EQ(EvaluateNoStats(effect_package_event_data.attributes.energy_damage_amplification), -20_fp);
    }

    // Test effect data
    ASSERT_EQ(events_apply_effect.size(), 3);
    {
        // Effect from attack ability
        const event_data::Effect& effect_event_data = events_apply_effect.at(0);
        const EffectData& effect_data = effect_event_data.data;
        const EffectDataAttributes& effect_data_attributes = effect_data.attributes;

        EXPECT_EQ(effect_event_data.sender_id, blue_entity->GetID());
        EXPECT_EQ(effect_event_data.receiver_id, red_entity->GetID());

        EXPECT_EQ(effect_data.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect_data.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect_data.GetExpression().base_value.value, 100_fp);

        // Test attributes
        EXPECT_EQ(effect_data_attributes.cleanse_bots, false);
        EXPECT_EQ(effect_data_attributes.missing_health_percentage_to_health, 10_fp);
    }
    {
        // Effect from empower
        const event_data::Effect& effect_event_data = events_apply_effect.at(1);
        const EffectData& effect_data = effect_event_data.data;
        const EffectDataAttributes& effect_data_attributes = effect_data.attributes;

        EXPECT_EQ(effect_event_data.sender_id, blue_entity->GetID());
        EXPECT_EQ(effect_event_data.receiver_id, red_entity->GetID());

        EXPECT_EQ(effect_data.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(effect_data.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(effect_data.GetExpression().base_value.value, 50_fp);

        // Test attributes
        EXPECT_EQ(effect_data_attributes.missing_health_percentage_to_health, 0_fp);
        EXPECT_EQ(EvaluateNoStats(effect_data.attached_effect_package_attributes.piercing_percentage), 0_fp);
    }

    events_apply_effect.clear();

    // No empowers
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemEmpowerTest, EmpowerWithAttackCritical)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_mutable_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_mutable_stats = red_stats_component.GetMutableTemplateStats();

    red_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    blue_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    blue_mutable_stats.Set(StatType::kCritAmplificationPercentage, 150_fp);
    red_mutable_stats.Set(StatType::kCritAmplificationPercentage, 100_fp);

    // Add empower and effect of pure damage
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = true;
    effect_data.lifetime.activations_until_expiry = 1;
    effect_data.lifetime.duration_time_ms = 10000;  // Should be ignored
    effect_data.lifetime.activated_by = AbilityType::kAttack;
    effect_data.lifetime.activate_on_critical = true;
    effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp))};

    effect_data.attached_effect_package_attributes.always_crit = true;
    effect_data.attached_effect_package_attributes.rotate_to_target = true;

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

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

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_apply_effect.size(), 3);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(events_apply_effect.at(0).state.is_critical, true);
    EXPECT_EQ(events_apply_effect.at(0).sender_id, blue_entity->GetID());
    EXPECT_EQ(events_apply_effect.at(0).receiver_id, red_entity->GetID());

    EXPECT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(events_apply_effect.at(1).state.is_critical, true);
    EXPECT_EQ(events_apply_effect.at(1).sender_id, blue_entity->GetID());
    EXPECT_EQ(events_apply_effect.at(1).receiver_id, red_entity->GetID());

    EXPECT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(events_apply_effect.at(1).state.is_critical, true);
    EXPECT_EQ(events_apply_effect.at(1).sender_id, blue_entity->GetID());
    EXPECT_EQ(events_apply_effect.at(1).receiver_id, red_entity->GetID());

    events_apply_effect.clear();

    // Empowers should be dead after 1 activation
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);

    // Red should be lower because of the pure damage empower from blue
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 900_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 550_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Reduced by normal attack ability
    ASSERT_EQ(events_apply_effect.size(), 2);
    events_apply_effect.clear();
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 400_fp);

    // Empowers should still be dead
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);

    // TimeStep some more
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Still dead
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemTest, EmpowerWithAttackNoCritical)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_mutable_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_mutable_stats = red_stats_component.GetMutableTemplateStats();

    red_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    blue_mutable_stats.Set(StatType::kCurrentEnergy, 0_fp);
    blue_mutable_stats.Set(StatType::kCritAmplificationPercentage, 150_fp);
    red_mutable_stats.Set(StatType::kCritAmplificationPercentage, 100_fp);

    // Add empower and effect of pure damage, should not fire since it only activates on critical
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = true;
    effect_data.lifetime.activations_until_expiry = 1;
    effect_data.lifetime.duration_time_ms = 10000;  // Should be ignored
    effect_data.lifetime.activated_by = AbilityType::kAttack;
    effect_data.lifetime.activate_on_critical = true;
    effect_data.attached_effects = {
        EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp))};

    effect_data.attached_effect_package_attributes.rotate_to_target = true;

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

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

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // No empower ability should fire since there was no critical
    ASSERT_EQ(events_apply_effect.size(), 2);
    EXPECT_EQ(events_apply_effect.at(0).data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(events_apply_effect.at(0).state.is_critical, false);
    EXPECT_EQ(events_apply_effect.at(0).sender_id, blue_entity->GetID());
    EXPECT_EQ(events_apply_effect.at(0).receiver_id, red_entity->GetID());

    EXPECT_EQ(events_apply_effect.at(1).data.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(events_apply_effect.at(1).state.is_critical, false);
    EXPECT_EQ(events_apply_effect.at(1).sender_id, red_entity->GetID());
    EXPECT_EQ(events_apply_effect.at(1).receiver_id, blue_entity->GetID());

    events_apply_effect.clear();

    // Empower should still be active because there was no critical
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Red and blue should be same health
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 900_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 900_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Reduced by normal attack ability
    ASSERT_EQ(events_apply_effect.size(), 2);
    events_apply_effect.clear();
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 800_fp);

    // Empower should still be active because there was no critical
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // TimeStep some more
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Empower should still be active because there was no critical
    EXPECT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);
}

TEST_F(AbilitySystemTestWithStartOmegaZoneSkill, EmpowerWithDurationOmegaZone)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Add empower
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = false;
    effect_data.lifetime.duration_time_ms = 200;
    effect_data.lifetime.activated_by = AbilityType::kOmega;
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    effect_data.attached_effect_package_attributes.can_crit = true;

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    blue_stats_component.SetCritChanceOverflowPercentage(100_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCritChancePercentage, 100_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 0_fp);

    // Listen to the apply effect events
    EventHistory<EventType::kTryToApplyEffect> events_apply_effect(*world);

    // Activate omega to deploy zone
    world->TimeStep();

    // Should be accepted on omega ability
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    ASSERT_EQ(events_apply_effect.Size(), 2);
    events_apply_effect.Clear();

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Red should be lower because of the crit empower from blue
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 750_fp);

    // Do attack abilities that should not have empower
    ASSERT_TRUE(TimeStepUntilEventAbilityDeactivated());

    // Empower should now expire
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemTestWithStartOmegaDirectSkill, EmpowerWithDurationOmegaDirect)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Add empower
    EffectData effect_data;
    effect_data.type_id.type = EffectType::kEmpower;
    effect_data.lifetime.is_consumable = false;
    effect_data.lifetime.duration_time_ms = 200;
    effect_data.lifetime.activated_by = AbilityType::kOmega;
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    effect_data.attached_effect_package_attributes.can_crit = true;

    auto& blue_attached_efects_component = blue_entity->Get<AttachedEffectsComponent>();
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    blue_stats_component.SetCritChanceOverflowPercentage(100_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCritChancePercentage, 100_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 0_fp);

    // Listen to the apply effect events
    std::vector<event_data::Effect> events_apply_effect;
    world->SubscribeToEvent(
        EventType::kTryToApplyEffect,
        [&events_apply_effect](const Event& event)
        {
            events_apply_effect.emplace_back(event.Get<event_data::Effect>());
        });

    world->TimeStep();

    ASSERT_EQ(events_apply_effect.size(), 2);
    events_apply_effect.clear();

    // Empowers should still be there
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 1);

    // Red should be lower because of the crit empower from blue
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 750_fp);

    world->TimeStep();
    world->TimeStep();

    // Empower should now expire
    ASSERT_EQ(blue_attached_efects_component.GetEmpowers().size(), 0);
}

TEST_F(AbilitySystemTest, EmpowerWithExcessVampToShield)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    Stun(*red_entity);

    blue_stats_component.SetCurrentEnergy(0_fp);
    blue_stats_component.SetCurrentHealth(10000_fp);
    red_stats_component.SetCurrentEnergy(0_fp);

    {
        EffectData effect_data;
        effect_data.type_id.type = EffectType::kEmpower;
        effect_data.lifetime.is_consumable = false;
        effect_data.lifetime.duration_time_ms = -1;
        effect_data.lifetime.activated_by = AbilityType::kAttack;
        auto& attached_attributes = effect_data.attached_effect_package_attributes;
        attached_attributes.excess_vamp_to_shield = true;
        attached_attributes.excess_vamp_to_shield_duration_ms = 1000;
        attached_attributes.vampiric_percentage = EffectExpression::FromValue(100_fp);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);
    }

    EventHistory<EventType::kAbilityActivated> ability_activated_events(*world);
    EventHistory<EventType::kOnEffectApplied> effect_applied_events(*world);
    EventHistory<EventType::kApplyHealthGain> health_gain_events(*world);
    EventHistory<EventType::kApplyShieldGain> shield_gain_events(*world);

    ASSERT_TRUE(TimeStepUntilEvent<EventType::kAbilityActivated>());
    ASSERT_EQ(ability_activated_events.Size(), 1) << "One ability activated";
    ASSERT_EQ(ability_activated_events.events.front().sender_id, blue_entity->GetID())
        << "Ability was activated by blue entity (red is stunned)";
    ASSERT_EQ(ability_activated_events.events.front().ability_type, AbilityType::kAttack)
        << "Activated ability is an attack ability";
    ASSERT_TRUE(effect_applied_events.IsEmpty()) << "Effects expected to be applied at the next time step";
    ASSERT_TRUE(shield_gain_events.IsEmpty());
    ASSERT_TRUE(health_gain_events.IsEmpty());
    ability_activated_events.Clear();

    ASSERT_TRUE(TimeStepUntilEvent<EventType::kOnEffectApplied>());
    ASSERT_TRUE(ability_activated_events.IsEmpty()) << "No new abilities expected";
    ASSERT_EQ(effect_applied_events.Size(), 1) << "Expected one (empowered) effect from attack ability";

    {
        ASSERT_EQ(health_gain_events.Size(), 1) << "One health gain event from vampiric";
        const auto& event = health_gain_events.events.front();
        EXPECT_TRUE(event.excess_vamp_to_shield);
        EXPECT_EQ(event.health_gain_type, HealthGainType::kPhysicalVamp);
        EXPECT_EQ(event.vampiric_percentage, 100_fp);
        EXPECT_EQ(event.receiver_id, blue_entity->GetID());
        EXPECT_EQ(event.caused_by_id, blue_entity->GetID());
        EXPECT_EQ(event.value, 100_fp);
        EXPECT_EQ(event.excess_vamp_to_shield_duration_ms, 1000);
    }

    {
        ASSERT_EQ(shield_gain_events.Size(), 1) << "One shield gain event from excess vamp";
        const auto& event = shield_gain_events.events.front();
        EXPECT_EQ(event.receiver_id, blue_entity->GetID());
        EXPECT_EQ(event.duration_time_ms, 1000);
        EXPECT_EQ(event.gain_amount, 100_fp);
    }

    EXPECT_EQ(world->GetShieldTotal(*blue_entity), 100_fp);
}
}  // namespace simulation
