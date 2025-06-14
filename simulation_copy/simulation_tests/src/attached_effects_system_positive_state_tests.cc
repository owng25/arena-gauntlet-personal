#include "attached_effects_system_data_fixtures.h"
#include "systems/ability_system.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
TEST_F(AttachedEffectsSystemTest, ApplyEffectHealOverTimeFullImmune)
{
    // Get the relevant components
    auto& blue_attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set the original health
    constexpr auto original_health = 100_fp;
    blue_stats_component.SetCurrentHealth(original_health);

    // Validate initial state
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), original_health);

    const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 1000);
    EffectState immune_effect_state{};
    immune_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, immune_effect_data, immune_effect_state);

    // Should heal 10 per time step, and only 5 at the end
    constexpr auto expected_health_per_time_step = "10.5"_fp;
    constexpr auto expected_heal_amount = 105_fp;
    auto effect_data = EffectData::CreateHealOverTime(
        EffectHealType::kNormal,
        EffectExpression::FromValue(expected_heal_amount),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect on self
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // NOTE: because we attach the effect directly, there should be no activation effect type
    // Listen to health changed events
    std::vector<event_data::StatChanged> events_health_changed;
    world->SubscribeToEvent(
        EventType::kHealthChanged,
        [&events_health_changed](const Event& event)
        {
            events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    auto blue_stats = world->GetLiveStats(blue_entity_id);

    constexpr int num_time_steps = 10;

    // For loop below should have immune active throughout duration but HOT should apply like normal
    for (int i = 0; i < num_time_steps; i++)
    {
        // Clear before each call
        events_apply_effect.clear();
        events_health_changed.clear();

        // TimeStep the world
        world->TimeStep();

        // Update blue stats
        blue_stats = world->GetLiveStats(blue_entity_id);

        // Not the last step, should equal the expected_health_per_time_step
        if (i == num_time_steps - 1)
        {
            continue;
        }

        // Should have fired health changed event
        ASSERT_EQ(events_health_changed.size(), 2) << " i = " << i;

        // The apply effect over time effect should have fired
        ASSERT_EQ(events_apply_effect.size(), 1) << " i = " << i;

        const auto& sent_effect = events_apply_effect.at(0).data;
        ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantHeal) << " i = " << i;
        ASSERT_TRUE(blue_attached_effects_component.HasPositiveState(EffectPositiveState::kImmune));
    }

    // Update blue stats
    blue_stats = world->GetLiveStats(blue_entity_id);

    // Should have fired a final health effect
    ASSERT_EQ(events_apply_effect.size(), 1);
    ASSERT_EQ(events_health_changed.size(), 2);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantHeal);

    // Last time step should add the precision back
    const ExpressionEvaluationContext expression_context(world.get(), blue_entity_id, blue_entity_id);
    ASSERT_EQ(sent_effect.GetExpression().Evaluate(expression_context), expected_health_per_time_step);

    // Does the total make sense?
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), original_health + expected_heal_amount);

    // TimeStep again
    events_apply_effect.clear();
    events_health_changed.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired a final removal effect
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
    ASSERT_EQ(events_remove_effect.at(1).data.type_id.type, EffectType::kHealOverTime);

    // Check this effect does not reactivate
    events_remove_effect.clear();
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 0);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectPositiveStateUntargetable)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        events_apply_effect.clear();
        world->TimeStep();

        //  Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be targetable
        ASSERT_FALSE(EntityHelper::IsTargetable(*world, blue_entity_id));
    }
    ASSERT_FALSE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);

    // Should be targetable
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));
}

TEST_F(AttachedEffectsSystemTest, FullInvulnerableWithDamages)
{
    // Get the relevant components
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Add shields
    ShieldData blue_shield_data, red_shield_data;
    blue_shield_data.sender_id = blue_entity->GetID();
    blue_shield_data.receiver_id = blue_entity->GetID();

    red_shield_data.sender_id = red_entity->GetID();
    red_shield_data.receiver_id = red_entity->GetID();

    blue_shield_data.value = 50_fp;
    red_shield_data.value = 50_fp;
    EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, blue_shield_data);
    EntityFactory::SpawnShieldAndAttach(*world, Team::kRed, red_shield_data);

    // set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kHealthGainEfficiencyPercentage, 100_fp);
    red_stats_component.SetCurrentHealth(1000_fp);

    // Add an effect and wrap it in attached
    auto energy_effect_data = EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));
    auto physical_effect_data =
        EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    auto pure_effect_data = EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    // Confirm initial stats
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);

    // Add the energy effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), energy_effect_data, effect_state);

    // Add the physical effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), physical_effect_data, effect_state);

    // Add the pure effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), pure_effect_data, effect_state);

    const auto invulnerable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 8000);
    EffectState invulnerable_effect_state{};
    invulnerable_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), invulnerable_effect_data, invulnerable_effect_state);

    for (int i = 0; i < 10; i++)
    {
        // TimeStep the world
        world->TimeStep();

        // Should be invulnerable
        ASSERT_TRUE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kInvulnerable)) << "i = " << i;
    }
    // TimeStep world
    world->TimeStep();

    // Health and shield should not be affected
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 50_fp);
}

TEST_F(AttachedEffectsSystemTest, PartialIndomitable)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kHealthGainEfficiencyPercentage, 100_fp);
    red_stats_component.SetCurrentHealth(200_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateDamageOverTime(
        EffectDamageType::kPhysical,
        EffectExpression::FromValue(500_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    const auto indomitable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kIndomitable, 700);
    EffectState indomitable_effect_state{};
    indomitable_effect_state.sender_stats = world->GetFullStats(red_entity_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity_id, indomitable_effect_data, indomitable_effect_state);

    auto red_live_stats = world->GetLiveStats(red_entity_id);

    for (int i = 0; i < 7; i++)
    {
        // time step the world
        world->TimeStep();
    }

    red_live_stats = world->GetLiveStats(red_entity_id);

    // Current health should be 1. Indomitable should prevent death.
    // DoT would normally do 250 damage so should have killed the red
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1_fp);

    // time step the world so Indomitable expires and DoT kills CombatUnit
    // time step 3 more times, indomitable is expired but Dot remains
    for (int i = 0; i < 3; i++)
    {
        world->TimeStep();
    }
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 0_fp);
    ASSERT_FALSE(red_entity->IsActive());
}

TEST_F(AttachedEffectsSystemTest, ApplyMultiplePositiveEffects)
{
    // Get the relevant components
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.SetCurrentHealth(100_fp);

    const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
    EffectState immune_effect_state{};
    immune_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), immune_effect_data, immune_effect_state);

    const auto immune_effect_data2 = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
    EffectState immune_effect_state2{};
    immune_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), immune_effect_data2, immune_effect_state2);

    const auto invulnerable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 300);
    EffectState invulnerable_effect_state{};
    invulnerable_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), invulnerable_effect_data, invulnerable_effect_state);

    const auto invulnerable_effect_data2 = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 300);
    EffectState invulnerable_effect_state2{};
    invulnerable_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), invulnerable_effect_data2, invulnerable_effect_state2);

    const auto indomitable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kIndomitable, 300);
    EffectState indomitable_effect_state{};
    indomitable_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), indomitable_effect_data, indomitable_effect_state);

    const auto indomitable_effect_data2 = EffectData::CreatePositiveState(EffectPositiveState::kIndomitable, 300);
    EffectState indomitable_effect_state2{};
    indomitable_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), indomitable_effect_data2, indomitable_effect_state2);

    const auto truesight_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kTruesight, 300);
    EffectState truesight_effect_state{};
    truesight_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), truesight_effect_data, truesight_effect_state);

    const auto truesight_effect_data2 = EffectData::CreatePositiveState(EffectPositiveState::kTruesight, 300);
    EffectState truesight_effect_state2{};
    truesight_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), truesight_effect_data2, truesight_effect_state2);

    const auto untargetable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 300);
    EffectState untargetable_effect_state{};
    untargetable_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), untargetable_effect_data, untargetable_effect_state);

    const auto untargetable_effect_data2 = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 300);
    EffectState untargetable_effect_state2{};
    untargetable_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), untargetable_effect_data2, untargetable_effect_state2);

    ASSERT_TRUE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kImmune));
    ASSERT_TRUE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kIndomitable));
    ASSERT_TRUE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kInvulnerable));
    ASSERT_TRUE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kTruesight));
    ASSERT_TRUE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kUntargetable));

    // Removed merged effects
    ASSERT_EQ(events_remove_effect.size(), 10);
    EXPECT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
    EXPECT_EQ(events_remove_effect.at(1).data.type_id.positive_state, EffectPositiveState::kImmune);
    EXPECT_EQ(events_remove_effect.at(2).data.type_id.positive_state, EffectPositiveState::kInvulnerable);
    EXPECT_EQ(events_remove_effect.at(3).data.type_id.positive_state, EffectPositiveState::kInvulnerable);
    EXPECT_EQ(events_remove_effect.at(4).data.type_id.positive_state, EffectPositiveState::kIndomitable);
    EXPECT_EQ(events_remove_effect.at(5).data.type_id.positive_state, EffectPositiveState::kIndomitable);
    EXPECT_EQ(events_remove_effect.at(6).data.type_id.positive_state, EffectPositiveState::kTruesight);
    EXPECT_EQ(events_remove_effect.at(7).data.type_id.positive_state, EffectPositiveState::kTruesight);
    EXPECT_EQ(events_remove_effect.at(8).data.type_id.positive_state, EffectPositiveState::kUntargetable);
    EXPECT_EQ(events_remove_effect.at(9).data.type_id.positive_state, EffectPositiveState::kUntargetable);

    for (int i = 0; i < 3; i++)
    {
        // Timestep the world (3 TimeSteps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Clear before each call
        events_apply_effect.clear();
    }

    // TimeStep again
    events_apply_effect.clear();
    events_remove_effect.clear();

    world->TimeStep();

    // Should have fired end event and removed active effects
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 5);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
    ASSERT_EQ(events_remove_effect.at(1).data.type_id.positive_state, EffectPositiveState::kInvulnerable);
    ASSERT_EQ(events_remove_effect.at(2).data.type_id.positive_state, EffectPositiveState::kIndomitable);
    ASSERT_EQ(events_remove_effect.at(3).data.type_id.positive_state, EffectPositiveState::kTruesight);
    ASSERT_EQ(events_remove_effect.at(4).data.type_id.positive_state, EffectPositiveState::kUntargetable);

    EXPECT_FALSE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kImmune));
    EXPECT_FALSE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kIndomitable));
    EXPECT_FALSE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kInvulnerable));
    EXPECT_FALSE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kTruesight));
    EXPECT_FALSE(red_attached_effects_component.HasPositiveState(EffectPositiveState::kUntargetable));
}

TEST_F(AttachedEffectsSystemTest, PurestDamageIgnoresPositiveState)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Get the Stats Data
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
    red_stats_component.SetCurrentHealth(200_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 100_fp);

    // Update current stats
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);

    // Add some positive states
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(*red_entity);

        const auto immune_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        const auto invulnerable_data = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 300);
        const auto indomitable_data = EffectData::CreatePositiveState(EffectPositiveState::kIndomitable, 300);

        const AttachedEffectsHelper& attached_effects_helper = GetAttachedEffectsHelper();
        attached_effects_helper.AddAttachedEffect(*red_entity, red_entity_id, immune_data, effect_state);
        attached_effects_helper.AddAttachedEffect(*red_entity, red_entity_id, invulnerable_data, effect_state);
        attached_effects_helper.AddAttachedEffect(*red_entity, red_entity_id, indomitable_data, effect_state);
    }
    ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kImmune));
    ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kInvulnerable));
    ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kIndomitable));

    // Send purest damage from blue to red
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 200_fp);
    auto damage_effect_data = EffectData::CreateDamage(EffectDamageType::kPurest, EffectExpression::FromValue(500_fp));

    EffectState damage_effect_state{};
    damage_effect_state.sender_stats = world->GetFullStats(*blue_entity);
    world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        blue_entity->GetID(),
        red_entity->GetID(),
        damage_effect_data,
        damage_effect_state);

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 0_fp);
}

TEST_F(AttachedEffectsSystemTest, FullIndomitable)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Get the Stats Data
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Add shield to red - check that we don't receive energy for the first tick
    // and receive only 1 energy for the second tick
    {
        ShieldData shield_data;
        shield_data.sender_id = red_entity->GetID();
        shield_data.receiver_id = red_entity->GetID();
        shield_data.value = 75_fp;
        EntityFactory::SpawnShieldAndAttach(*world, red_entity->GetTeam(), shield_data);
    }

    // Set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyGainEfficiencyPercentage, 100_fp);
    red_stats_component.SetCurrentHealth(200_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyCost, 100_fp);

    // Check current stats
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);
    ASSERT_EQ(world->GetShieldTotal(*red_entity), 75_fp);

    const AttachedEffectsHelper& attached_effects_helper = GetAttachedEffectsHelper();

    // Add physical damage over time effect effect and wrap it in an attached effect
    {
        auto effect_data = EffectData::CreateDamageOverTime(
            EffectDamageType::kPhysical,
            EffectExpression::FromValue(500_fp),
            1000,
            kDefaultAttachedEffectsFrequencyMs);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(*blue_entity);
        attached_effects_helper.AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
    }

    // Add indomitable effect that covers full duration of DOT effect
    {
        const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kIndomitable, 1000);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(*red_entity);
        attached_effects_helper.AddAttachedEffect(*red_entity, red_entity_id, effect_data, effect_state);
    }

    world->TimeStep();
    // After the first time step health should not change since first damage from dot is consumed by shield
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 200_fp);
    // 1 energy (passive regeneration)
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 1_fp);

    world->TimeStep();
    // After the second time step health should decrease by 25 (another 25 damage from dut consumed by shield)
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 175_fp);
    // 1 energy (passive regeneration)
    // 1.25 energy for 25 damage taken:
    //   (25 (helath change) * 50 (pre-mitigated) / 50 (post-mitigated)) / 20
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), "3.250"_fp);

    // during each of the following three ticks red entity gains
    // one energy due to energy regeneration
    // and 2.5 energy for received damage:
    //   (50 (helath change) * 50 (pre-mitigated) / 50 (post-mitigated)) / 20
    for (int i = 1; i <= (175 / 50); i++)
    {
        constexpr FixedPoint energy_per_step =
            1_fp + Math::CalculateEnergyGainOnTakeDamage(50_fp, 50_fp, 50_fp) / 20_fp;
        world->TimeStep();
        EXPECT_EQ(red_stats_component.GetCurrentEnergy(), "3.250"_fp + energy_per_step * FixedPoint::FromInt(i))
            << "i = " << i;
        EXPECT_EQ(red_stats_component.GetCurrentHealth(), FixedPoint::FromInt(175 - i * 50)) << "i = " << i;
    }

    // Just declare what we should have after the previous loop
    EXPECT_EQ(red_stats_component.GetCurrentEnergy(), "13.75"_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 25_fp);

    // Now red entity has only 25 health and receives 50 damage
    // During the next tick red entity gains
    // 1 energy for regeneration
    // 1.25 energy for damage received:
    //   (25 (helath change) * 50 (pre-mitigated) / 50 (post-mitigated)) / 20
    world->TimeStep();
    EXPECT_EQ(red_stats_component.GetCurrentEnergy(), 16_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 1_fp);

    // during the following 4 ticks red entity does not receive energy from
    // damage taken since health lost is 0 (due to indominate state)
    // but continues receive energy from regeneration
    for (int i = 1; i <= 4; i++)
    {
        // time step the world
        world->TimeStep();
        // Current health should be 1. Indomitable should prevent death.
        // DOT does 500 damage, red has 200 current health and 75 shield so normally would die
        ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1_fp);
        constexpr FixedPoint energy_on_take_damage = Math::CalculateEnergyGainOnTakeDamage(1_fp, 50_fp, 50_fp) / 20_fp;
        const auto expected_energy = 16_fp + FixedPoint::FromInt(i) * (1_fp + energy_on_take_damage);
        ASSERT_EQ(red_stats_component.GetCurrentEnergy(), expected_energy) << "i = " << i;
    }
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectPositiveStateUntargetableTwice)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 200);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Add second to make sure attached effect stacking properly ticks multiple positive effects
    const auto second_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 300);
    EffectState second_effect_state{};
    second_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, second_effect_data, second_effect_state);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        events_apply_effect.clear();
        world->TimeStep();

        //  Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be targetable
        ASSERT_FALSE(EntityHelper::IsTargetable(*world, blue_entity_id)) << "i = " << i;
    }
    ASSERT_FALSE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // TimeStep again
    events_apply_effect.clear();

    // Shorter effect should expire
    ASSERT_EQ(events_remove_effect.size(), 2);
    events_remove_effect.clear();
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);

    // Should be targetable
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));
}

TEST_F(AttachedEffectsSystemTest, ApplyDifferentEffectPositiveState)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Add second to make sure attached effect stacking properly ticks multiple positive effects
    const auto second_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
    EffectState second_effect_state{};
    second_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, second_effect_data, second_effect_state);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        events_apply_effect.clear();
        world->TimeStep();

        //  Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be targetable
        ASSERT_FALSE(EntityHelper::IsTargetable(*world, blue_entity_id));
        ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*blue_entity));
    }
    ASSERT_FALSE(EntityHelper::IsTargetable(*world, blue_entity_id));
    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*blue_entity));

    // TimeStep again
    events_apply_effect.clear();

    // Shorter effect should expire
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);

    // Should be targetable
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));
    ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*blue_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));
    ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*blue_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectPositiveStateEffectPackageBlock)
{
    // Get the relevant components
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Add EffectPackageBlock to red
    auto apply_blind_block = [&](int duration_ms, int blocks_count = kTimeInfinite)
    {
        auto effect_package_block_effect_data =
            EffectData::CreatePositiveState(EffectPositiveState::kEffectPackageBlock, 0);

        // Create effect
        effect_package_block_effect_data.ability_types.emplace_back(AbilityType::kAttack);
        effect_package_block_effect_data.ability_types.emplace_back(AbilityType::kInnate);
        effect_package_block_effect_data.ability_types.emplace_back(AbilityType::kOmega);
        effect_package_block_effect_data.lifetime = {};
        effect_package_block_effect_data.lifetime.activated_by = AbilityType::kAttack;
        effect_package_block_effect_data.lifetime.duration_time_ms = duration_ms;
        effect_package_block_effect_data.lifetime.blocks_until_expiry = blocks_count;

        EffectState effect_package_block_effect_state{};
        effect_package_block_effect_state.sender_stats = world->GetFullStats(red_entity_id);

        // Create pointer to attached effect manually
        const auto effect_package_block_attached_effect = AttachedEffectState::Create(
            red_entity_id,
            effect_package_block_effect_data,
            effect_package_block_effect_state);

        // Add effect_package_block
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, effect_package_block_attached_effect);
    };

    // Applies blind over effect_package_block
    auto apply_blind = [&](int duration_ms)
    {
        EffectPackage effect_package_blind;
        effect_package_blind.AddEffect(EffectData::CreateNegativeState(EffectNegativeState::kBlind, duration_ms));
        auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
        ability_system
            .ApplyEffectPackage(*blue_entity, attack_ability, effect_package_blind, false, red_entity->GetID());
    };

    // Case 1: effect package block has both duration and blocks count
    {
        const int blocks_count = 3;

        // block blinds 3 times during 300 ms
        apply_blind_block(300, blocks_count);

        // Should have the effect type
        ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kEffectPackageBlock));

        // Apply activations_until_expiry times
        for (int i = 0; i < blocks_count; i++)
        {
            EXPECT_EQ(events_effect_package_blocked.size(), i) << "i = " << i;
            apply_blind(400);
        }

        // Should be blocked activations_until_expiry times
        EXPECT_EQ(events_effect_package_blocked.size(), blocks_count);

        // Should not be blinded
        ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

        // EffectPackageBlock should be removed
        events_effect_package_blocked.clear();
        TimeStepForNTimeSteps(4);
        ASSERT_FALSE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kEffectPackageBlock));

        // Apply now should work
        apply_blind(100);

        ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));
        EXPECT_EQ(events_effect_package_blocked.size(), 0);

        // Remove blind before the next test case
        TimeStepForNTimeSteps(2);
        ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
    }

    // Case 2: effect package block has infinite duration and blocks count
    {
        const int blocks_count = 5;

        // block blinds blocks_count times
        apply_blind_block(kTimeInfinite, blocks_count);

        // Should have the effect type
        ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kEffectPackageBlock));

        // Apply activations_until_expiry times
        for (int i = 0; i < blocks_count; i++)
        {
            EXPECT_EQ(events_effect_package_blocked.size(), i) << "i = " << i;
            apply_blind(400);
        }

        // Should be blocked activations_until_expiry times
        EXPECT_EQ(events_effect_package_blocked.size(), blocks_count);

        // Should not be blinded
        ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

        // EffectPackageBlock should be removed
        events_effect_package_blocked.clear();
        TimeStepForNTimeSteps(4);
        ASSERT_FALSE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kEffectPackageBlock));

        // Apply now should work
        apply_blind(100);

        ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));
        EXPECT_EQ(events_effect_package_blocked.size(), 0);

        // Remove blind before the next test case
        TimeStepForNTimeSteps(2);
        ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
    }

    // Case 3: effect expires earlier than max blocks count reached
    {
        // block blinds 5 times
        apply_blind_block(100, 5);

        // Should have the effect type
        ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kEffectPackageBlock));

        // Apply blind 4 times
        for (int i = 0; i < 4; i++)
        {
            apply_blind(400);

            EXPECT_EQ(events_effect_package_blocked.size(), i + 1) << "i = " << i;

            // Should not be blinded
            ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
        }

        // EffectPackageBlock should be removed
        events_effect_package_blocked.clear();
        TimeStepForNTimeSteps(2);
        ASSERT_FALSE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kEffectPackageBlock));

        // Apply now should work
        apply_blind(100);

        ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));
        EXPECT_EQ(events_effect_package_blocked.size(), 0);

        // Remove blind before the next test case
        TimeStepForNTimeSteps(2);
        ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
    }
}

}  // namespace simulation
