
#include "attached_effects_system_data_fixtures.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
TEST_F(AttachedEffectsSystemTest, ApplyBuff)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // set sane values
    stats_component.SetCurrentHealth(100_fp);

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 0_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data =
        EffectData::CreateBuff(StatType::kAttackPhysicalDamage, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackPhysicalDamage), 0_fp);

    // Should compute to the current stats even before the TimeStep
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 100_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 100_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 100_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 0_fp);
}

TEST_F(AttachedEffectsSystemTest, DynamicBuffFrequency)
{
    auto& helper = GetAttachedEffectsHelper();
    auto& stats_component = blue_entity->Get<StatsComponent>();
    stats_component.SetCurrentHealth(100_fp);

    // Verfy initial state
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), 0_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), 100_fp);

    // Add static and infinite omega power buff
    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kOmegaPowerPercentage, EffectExpression::FromValue(37_fp), kTimeInfinite);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "Static Omega Power";
        helper.AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Add static and infinite grit buff
    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kGrit, EffectExpression::FromValue(19_fp), kTimeInfinite);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "Static Grit";
        helper.AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Buff are static => stat values change instantly
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), 19_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), 137_fp);

    // Add dynamic grit buff that depends on omega power, expires in 500ms and refreshes every 200ms
    {
        const auto expression = EffectExpression::FromStatPercentage(
            10_fp,
            StatType::kOmegaPowerPercentage,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);
        const auto effect_data = EffectData::CreateDynamicBuff(StatType::kGrit, expression, 500, 200);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "Dynamic grit";
        helper.AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Buff is dynamic, the value should not change before the time step
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), 19_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), 137_fp);

    world->TimeStep();

    // 19 (previous live value) + 10% of 137OP = 19 + 13.7
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "32.7"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), 137_fp);

    // Add dynamic omega power buff that depends on grit, expires in 700ms and refereshes every 300ms
    {
        const auto expression =
            EffectExpression::FromStat(StatType::kGrit, StatEvaluationType::kLive, ExpressionDataSourceType::kReceiver);
        const auto effect_data = EffectData::CreateDynamicBuff(StatType::kOmegaPowerPercentage, expression, 700, 300);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "Dynamic Omega Power";
        helper.AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Buff is dynamic so will be applied the next time step
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "32.7"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), 137_fp);

    // At the end of time step blue has 32.7 grit and 137 OP. These values will be used for new and refereshed dynamic
    // buffs OP bonus = 32.7 (previous live grit)
    // new OP value = 137 (previous live OP) + 32.7 (buff bonus) = 169
    world->TimeStep();
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "32.7"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), "169.7"_fp);

    // The dynamic grit buff refreshes
    // 19 (from static buff) + 169.7 (prev live OP) / 10 = 19 + 16.97 = 35.97
    world->TimeStep();
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "35.97"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), "169.7"_fp);

    // Nothing changes, OP buff is going to refresh the next time step
    world->TimeStep();
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "35.97"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), "169.7"_fp);

    // The dynamic OP buff is going to refresh now
    // OP = 35.97 (previous live grit) + 100 (base) + 37 (static buff) = 137 + 35.97 = 172.97
    world->TimeStep();
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "35.97"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), "172.97"_fp);

    // The dynamic grit buff expires now, leaving only static buff
    // Despite dynamic OP buff depends on grit live value it will not update now
    world->TimeStep();
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "19"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), "172.97"_fp);

    // Nothing changes until the next dynamic OP buff refresh
    world->TimeStep();
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "19"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), "172.97"_fp);

    // The second refresh of dynamic OP buff
    // OP = 19 (prev live grit) + 100  (base) + 37 (static buff) = 156
    world->TimeStep();
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "19"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), "156"_fp);

    // Dynamic OP buff expires
    world->TimeStep();
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kGrit), "19"_fp);
    ASSERT_EQ(world->GetLiveStats(blue_entity_id).Get(StatType::kOmegaPowerPercentage), "137"_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyBuffLiveValue)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // set sane values
    stats_component.SetCurrentHealth(100_fp);

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaPowerPercentage), 100_fp);

    // Add buff to omega power
    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kOmegaPowerPercentage, EffectExpression::FromValue(100_fp), kTimeInfinite);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "1";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Validate omega power buff worked
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaPowerPercentage), 200_fp);
    world->TimeStep();

    // Add base buff for grit
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kGrit), 0_fp);
    {
        auto expression = EffectExpression::FromStatPercentage(
            10_fp,
            StatType::kOmegaPowerPercentage,
            StatEvaluationType::kBase,
            ExpressionDataSourceType::kReceiver);
        auto effect_data = EffectData::CreateBuff(StatType::kGrit, expression, kTimeInfinite);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "1";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Grit should change: 10% of base omega power is 10
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kGrit), 10_fp);
    world->TimeStep();

    // Add live buff for grit
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kGrit), 10_fp);
    {
        auto expression = EffectExpression::FromStatPercentage(
            10_fp,
            StatType::kOmegaPowerPercentage,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);
        auto effect_data = EffectData::CreateDynamicBuff(StatType::kGrit, expression, kTimeInfinite, 100);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "2";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Grit should not have been affected, it happens at the end of time step
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kGrit), 10_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaPowerPercentage), 200_fp);
    world->TimeStep();

    // Grit should have been affected after time step
    // 10 + 20
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kGrit), 30_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaPowerPercentage), 200_fp);

    // Add a buff to omega power that depends on grit
    // NOTE: This creates a circular reference grit <-> omega power
    {
        auto expression = EffectExpression::FromStatPercentage(
            10_fp,
            StatType::kGrit,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);
        auto effect_data =
            EffectData::CreateDynamicBuff(StatType::kOmegaPowerPercentage, expression, kTimeInfinite, 1000);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "2";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Grit remains the same
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kGrit), 30_fp);
    // Before time step grit does not change value
    EXPECT_EQ(live_stats.Get(StatType::kOmegaPowerPercentage), 200_fp);

    world->TimeStep();

    // After time step: 200 + 10% Grit = 200 + 3
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaPowerPercentage), 203_fp);

    // Check that it still works in the next time step
    world->TimeStep();
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kGrit), "30.3"_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaPowerPercentage), 203_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyMaxHealthBuff)
{
    constexpr auto initial_max_health = 1000_fp;
    constexpr auto initial_current_health = 500_fp;

    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Set current health
    stats_component.SetCurrentHealth(initial_current_health);

    // Calculate live stats
    EXPECT_EQ(stats_component.GetMaxHealth(), initial_max_health);
    EXPECT_EQ(stats_component.GetCurrentHealth(), initial_current_health);

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate initial state
    EXPECT_EQ(stats_component.GetMaxHealth(), initial_max_health + 100_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();

        // Should be active now
        EXPECT_EQ(stats_component.GetMaxHealth(), initial_max_health + 100_fp);
        EXPECT_EQ(stats_component.GetCurrentHealth(), initial_current_health + 100_fp) << "i = " << i;
    }

    // Should be active now
    EXPECT_EQ(stats_component.GetMaxHealth(), initial_max_health + 100_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), initial_current_health + 100_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    EXPECT_EQ(stats_component.GetMaxHealth(), initial_max_health);
    EXPECT_EQ(stats_component.GetCurrentHealth(), initial_current_health + 100_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetMaxHealth(), initial_max_health);
    EXPECT_EQ(stats_component.GetCurrentHealth(), initial_current_health + 100_fp);

    // Add an effect Wrap it in an attached effect
    constexpr auto buff_value = 100_fp;
    auto effect_data2 =
        EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(buff_value), kTimeInfinite);
    effect_data2.lifetime.overlap_process_type = EffectOverlapProcessType::kStacking;
    EffectState effect_state2{};
    effect_state2.sender_stats = world->GetFullStats(blue_entity_id);

    for (int i = 1; i < 10; i++)
    {
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data2, effect_state2);

        const auto expected_current_health = initial_current_health + (FixedPoint::FromInt(i + 1) * buff_value);
        const auto expected_max_health = initial_max_health + FixedPoint::FromInt(i) * buff_value;

        ASSERT_EQ(stats_component.GetMaxHealth(), expected_max_health);
        ASSERT_EQ(stats_component.GetCurrentHealth(), expected_current_health) << " i = " << i;

        world->TimeStep();
    }
}

TEST_F(AttachedEffectsSystemTest, ApplyMaxHealthBuffSum)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    effect_data.lifetime.overlap_process_type = EffectOverlapProcessType::kSum;
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);

    for (int i = 1; i < 3; i++)
    {
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
        EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp + FixedPoint::FromInt(i * 100));
        world->TimeStep();
        EXPECT_EQ(stats_component.GetCurrentHealth(), 1000_fp + FixedPoint::FromInt(i * 100)) << i << " ";
    }

    world->TimeStep();

    // 2 Should be active now, Sum overlap will keep their own duration so expire at different times
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1200_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1200_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1000_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyMaxHealthBuffTwice)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    auto second_effect_data = EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    EffectState second_effect_state{};
    second_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, second_effect_data, second_effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1100_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(stats_component.GetMaxHealth(), 1100_fp);
        EXPECT_EQ(stats_component.GetCurrentHealth(), 1100_fp) << "i = " << i;
    }

    ASSERT_EQ(events_remove_effect.size(), 1);
    world->TimeStep();

    world->TimeStep();
    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1000_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyDebuff)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 100_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kWillpowerPercentage, 50_fp);
    red_stats_component.SetCurrentHealth(100_fp);

    // Calculate live stats
    StatsData red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 100_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kWillpowerPercentage), 50_fp);

    // Add debuff
    // Because willpower is 50 this will only have half the effect
    auto effect_data =
        EffectData::CreateDebuff(StatType::kAttackPhysicalDamage, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Should compute to the current stats even before the TimeStep
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 50_fp);

    for (int i = 0; i < 3; i++)
    {
        world->TimeStep();

        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 50_fp);
    }

    // Should be active now
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 50_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);

    // Should no longer be active
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 100_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyMaxHealthDebuff)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate debuff state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 900_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(stats_component.GetMaxHealth(), 900_fp);
        EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 900_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyMaxHealthDebuffTwice)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    auto second_effect_data = EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    EffectState second_effect_state{};
    second_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, second_effect_data, second_effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 900_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(stats_component.GetMaxHealth(), 900_fp);
        EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 900_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);
    ASSERT_EQ(events_remove_effect.size(), 1);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyMaxHealthBuffKeepCurrent)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();
    stats_component.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 500_fp);

    auto& stats_component2 = blue_entity->Get<StatsComponent>();
    stats_component2.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 500_fp);

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 500_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1100_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(stats_component.GetMaxHealth(), 1100_fp);
        EXPECT_EQ(stats_component.GetCurrentHealth(), 600_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1100_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 600_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 600_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyMaxHealthBuffWithDamage)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1100_fp);

    // TimeStep the world (3 steps)
    world->TimeStep();

    // Should be active now
    EXPECT_EQ(stats_component.GetMaxHealth(), 1100_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1100_fp);

    {
        auto damage_effect_data =
            EffectData::CreateDamage(EffectDamageType::kPhysical, EffectExpression::FromValue(200_fp));
        EffectState damage_state{};
        damage_state.sender_stats = world->GetFullStats(blue_entity_id);
        // GetAttachedEffectsHelper().AddAttachedEffect( *blue_entity, red_entity_id, damage_effect_data,
        // damage_state);

        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            red_entity_id,
            blue_entity->GetID(),
            damage_effect_data,
            damage_state);
    }

    world->TimeStep();

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1100_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();
    world->TimeStep();

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetMaxHealth(), 1000_fp);
    // Should have 900 health, 1100 originally from the 100 health buff then 200 damage = 900 health
    EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);

    // Tick again to ensure health does not increase
    world->TimeStep();
    EXPECT_EQ(stats_component.GetCurrentHealth(), 900_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyEnergyCostBuff)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 10000_fp);

    // Add an effect Wrap it in an attached effect
    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kEnergyCost, EffectExpression::FromValue(1000_fp), 300);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName1";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 9000_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(stats_component.GetEnergyCost(), 9000_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 9000_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 10000_fp);

    // Apply percentage buff
    {
        auto expression = EffectExpression::FromValue(50_fp);
        expression.is_used_as_percentage = true;

        auto effect_data = EffectData::CreateBuff(StatType::kEnergyCost, expression, 100);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName2";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 5000_fp);

    // Add regular
    {
        auto expression = EffectExpression::FromValue(1000_fp);
        expression.is_used_as_percentage = false;

        auto effect_data = EffectData::CreateBuff(StatType::kEnergyCost, expression, 100);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName3";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 4500_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyEnergyCostDeBuff)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 10000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateDebuff(StatType::kEnergyCost, EffectExpression::FromValue(1000_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 11000_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(stats_component.GetEnergyCost(), 11000_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 11000_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(stats_component.GetEnergyCost(), 10000_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectHealOverTime)
{
    // Get the relevant components
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set the original health
    constexpr auto original_health = 100_fp;
    blue_stats_component.SetCurrentHealth(original_health);

    // Validate initial state
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), original_health);

    // Should heal 10 per time step, and only 5 at the end
    constexpr auto expected_heal_amount = 105_fp;
    constexpr auto expected_health_per_time_step = expected_heal_amount / 10_fp;
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
    }

    // Update blue stats
    blue_stats = world->GetLiveStats(blue_entity_id);

    // Should have fired a final health effect
    ASSERT_EQ(events_apply_effect.size(), 1);
    ASSERT_EQ(events_health_changed.size(), 2);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantHeal);

    const ExpressionEvaluationContext expression_context(world.get(), blue_entity_id, blue_entity_id);

    // Last time step should add the precision back
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
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kHealOverTime);

    // Check this effect does not reactivate
    events_remove_effect.clear();
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 0);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectPureHealOverTime)
{
    // Get the relevant components
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set the original health
    constexpr auto original_health = 100_fp;
    blue_stats_component.SetCurrentHealth(original_health);

    // Set health gain percentage to 0 since pure heal should heal the full amount
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kHealthGainEfficiencyPercentage, kMinPercentageFP);

    // Validate initial state
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), original_health);

    // Should heal 10 per time step, and only 5 at the end
    constexpr auto expected_heal_amount = 105_fp;
    constexpr auto expected_health_per_time_step = expected_heal_amount / 10_fp;
    auto effect_data = EffectData::CreateHealOverTime(
        EffectHealType::kPure,
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
    }

    // Update blue stats
    blue_stats = world->GetLiveStats(blue_entity_id);

    // Should have fired a final health effect
    ASSERT_EQ(events_apply_effect.size(), 1);
    ASSERT_EQ(events_health_changed.size(), 2);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantHeal);

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
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kHealOverTime);

    // Check this effect does not reactivate
    events_remove_effect.clear();
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 0);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectEnergyGainOverTime)
{
    // Get the relevant components
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set the original health
    constexpr auto original_energy = 100_fp;
    blue_stats_component.SetCurrentEnergy(original_energy);
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kEnergyRegeneration, 0_fp);

    // Validate initial state
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), original_energy);

    // Should heal 10 per time step, and only 5 at the end
    constexpr auto expected_energy_gain_amount = 105_fp;
    constexpr auto expected_energy_per_time_step = expected_energy_gain_amount / 10_fp;
    auto effect_data = EffectData::CreateEnergyGainOverTime(
        EffectExpression::FromValue(expected_energy_gain_amount),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect on self
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // NOTE: because we attach the effect directly, there should be no activation effect type
    // Listen to energy changed events
    std::vector<event_data::StatChanged> events_energy_changed;
    world->SubscribeToEvent(
        EventType::kEnergyChanged,
        [&events_energy_changed](const Event& event)
        {
            events_energy_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    auto blue_stats = world->GetLiveStats(blue_entity_id);

    constexpr int num_time_steps = 10;
    for (int i = 0; i < num_time_steps; i++)
    {
        // Clear before each call
        events_apply_effect.clear();
        events_energy_changed.clear();

        // TimeStep the world
        world->TimeStep();

        // Update blue stats
        blue_stats = world->GetLiveStats(blue_entity_id);

        // Not the last step, should equal the expected_energy_per_time_step
        if (i == num_time_steps - 1)
        {
            continue;
        }

        // Should have fired energy changed event
        ASSERT_EQ(events_energy_changed.size(), 2) << " i = " << i;

        // The apply effect over time effect should have fired
        ASSERT_EQ(events_apply_effect.size(), 1) << " i = " << i;

        const auto& sent_effect = events_apply_effect.at(0).data;
        ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantEnergyGain) << " i = " << i;
    }

    // Update blue stats
    blue_stats = world->GetLiveStats(blue_entity_id);

    // Should have fired a final energy effect
    ASSERT_EQ(events_apply_effect.size(), 1);
    ASSERT_EQ(events_energy_changed.size(), 2);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantEnergyGain);

    const ExpressionEvaluationContext expression_context(world.get(), blue_entity_id, blue_entity_id);
    ASSERT_EQ(sent_effect.GetExpression().Evaluate(expression_context), expected_energy_per_time_step);

    // Does the total make sense?
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), original_energy + expected_energy_gain_amount);

    // TimeStep again
    events_apply_effect.clear();
    events_energy_changed.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired a final removal effect
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kEnergyGainOverTime);

    // Check this effect does not reactivate
    events_remove_effect.clear();
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 0);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectOverTimeNoLostPrecision)
{
    // Get the relevant components
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set the original health
    blue_stats_component.SetCurrentHealth(100_fp);

    auto effect_data = EffectData::CreateHealOverTime(
        EffectHealType::kNormal,
        EffectExpression::FromValue(200_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect on self
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Should not crash
    for (int i = 0; i < 10; i++)
    {
        world->TimeStep();
    }
}

TEST_F(AttachedEffectsSystemTest, ApplyAttachedEffectWithTimeFrequencyTest1)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.SetCurrentHealth(1000_fp);
    auto health = 1000_fp;

    constexpr auto damage = 500_fp;
    constexpr auto duration_time = 1000;
    constexpr auto frequency_time_ms = 300;

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateDamageOverTime(
        EffectDamageType::kPhysical,
        EffectExpression::FromValue(damage),
        duration_time,
        frequency_time_ms);

    // Add effect state
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    auto& attached_effect_state =
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    const auto& apply_over_time_data = attached_effect_state.apply_over_time_data;

    ASSERT_EQ(attached_effect_state.frequency_time_steps, Time::MsToTimeSteps(frequency_time_ms));

    int steps = Time::MsToTimeSteps(duration_time) - 1;
    for (int i = 0; i < steps; ++i)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        ASSERT_EQ(attached_effect_state.apply_over_time, true) << " i = " << i;

        if ((i + 1) % attached_effect_state.frequency_time_steps == 0)
        {
            // effects should have fired
            ASSERT_EQ(events_apply_effect.size(), 1) << " i = " << i;

            // Decreasing health
            health -= apply_over_time_data.value_per_frequency_time_step;
            ASSERT_EQ(red_stats_component.GetCurrentHealth(), health);

            const auto& sent_effect = events_apply_effect.at(0).data;
            ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantDamage);
            ASSERT_EQ(sent_effect.type_id.damage_type, EffectDamageType::kPhysical) << " i = " << i;
        }
        else
        {
            // No effects should have fired
            ASSERT_EQ(events_apply_effect.size(), 0) << " i = " << i;
        }
    }

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 550_fp);

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last TimeStep for final damage
    world->TimeStep();

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    // Should have fired a final pure damage effect
    ASSERT_EQ(events_apply_effect.size(), 1);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantDamage);
    ASSERT_EQ(sent_effect.type_id.damage_type, EffectDamageType::kPhysical);
}

TEST_F(AttachedEffectsSystemTest, ApplyAttachedEffectWithTimeFrequencyTest2)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.SetCurrentHealth(1000_fp);
    auto health = 1000_fp;

    constexpr auto damage = 500_fp;
    constexpr auto duration_time = 1100;
    constexpr auto frequency_time_ms = 300;

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateDamageOverTime(
        EffectDamageType::kPhysical,
        EffectExpression::FromValue(damage),
        duration_time,
        frequency_time_ms);

    // Add effect state
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    auto& attached_effect_state =
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    const auto& apply_over_time_data = attached_effect_state.apply_over_time_data;

    ASSERT_EQ(attached_effect_state.frequency_time_steps, Time::MsToTimeSteps(frequency_time_ms));

    int steps = Time::MsToTimeSteps(duration_time) - 1;
    for (int i = 0; i < steps; ++i)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        ASSERT_EQ(attached_effect_state.apply_over_time, true) << " i = " << i;

        if ((i + 1) % attached_effect_state.frequency_time_steps == 0)
        {
            ASSERT_EQ(events_apply_effect.size(), 1) << " i = " << i;

            // Decreasing health
            health -= apply_over_time_data.value_per_frequency_time_step;
            ASSERT_EQ(red_stats_component.GetCurrentHealth(), health);

            const auto& sent_effect = events_apply_effect.at(0).data;
            ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantDamage);
            ASSERT_EQ(sent_effect.type_id.damage_type, EffectDamageType::kPhysical) << " i = " << i;
        }
        else
        {
            ASSERT_EQ(events_apply_effect.size(), 0) << " i = " << i;
        }
    }

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), "590.911"_fp);

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last TimeStep for final damage
    world->TimeStep();

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);

    // Should have fired a final pure damage effect
    ASSERT_EQ(events_apply_effect.size(), 1);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantDamage);
    ASSERT_EQ(sent_effect.type_id.damage_type, EffectDamageType::kPhysical);

    // TimeStep tree
    world->TimeStep();

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 500_fp);
}

TEST_F(AttachedEffect_SameDebuff_OverlapTest, SameAbilities)
{
    // Should take highest
    RunTest("TestAbilityName", "TestAbilityName", 40_fp);
}

TEST_F(AttachedEffect_SameDebuff_OverlapTest, DifferentAbilities)
{
    // Should sum them up, 100 - 60 - 10 = 30
    RunTest("TestAbilityName1", "TestAbilityName2", 30_fp);
}

TEST_F(AttachedEffect_SameDebuff_DifferentEntities_OverlapTest, DifferentAbilities)
{
    RunTest("TestAbilityName1", "TestAbilityName2", 380_fp);
}

TEST_F(AttachedEffect_SameDebuff_DifferentEntities_OverlapTest, SameAbilities)
{
    RunTest("TestAbilityName", "TestAbilityName", 300_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyDifferentDebuff)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 100_fp);
    red_stats_component.SetCurrentHealth(100_fp);

    // Calculate live stats
    StatsData red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 100_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 100_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kWillpowerPercentage), 0_fp);

    // Add debuff
    auto effect_data = EffectData::CreateDebuff(StatType::kAttackSpeed, EffectExpression::FromValue(50_fp), 400);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Should compute to the current stats even before the TimeStep
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 50_fp);

    world->TimeStep();

    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 50_fp);

    auto second_effect_data =
        EffectData::CreateDebuff(StatType::kAttackPhysicalDamage, EffectExpression::FromValue(70_fp), 300);

    EffectState second_effect_state{};
    second_effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add a second effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, second_effect_data, second_effect_state);

    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 50_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 30_fp);

    for (int i = 0; i < 3; i++)
    {
        world->TimeStep();

        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 50_fp);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 30_fp);
    }

    // Should be active now
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 50_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 30_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);

    // Should no longer be active
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 100_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 100_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplySameBuff)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kVulnerabilityPercentage, 100_fp);
    red_stats_component.SetCurrentHealth(100_fp);

    // Calculate live stats
    StatsData red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 100_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kWillpowerPercentage), 0_fp);

    // Add buff
    auto effect_data =
        EffectData::CreateBuff(StatType::kVulnerabilityPercentage, EffectExpression::FromValue(50_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Should compute to the current stats even before the TimeStep
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 50_fp);

    world->TimeStep();

    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 50_fp);

    auto priority_effect_data =
        EffectData::CreateBuff(StatType::kVulnerabilityPercentage, EffectExpression::FromValue(200_fp), 300);

    EffectState priority_effect_state{};
    priority_effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the priority effect which should take over
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, priority_effect_data, priority_effect_state);

    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 0_fp);

    for (int i = 0; i < 3; i++)
    {
        world->TimeStep();

        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 0_fp);
    }

    // Should be active now
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 0_fp);
    ASSERT_EQ(events_remove_effect.size(), 1);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kVulnerabilityPercentage), 100_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplySameBuffWithStacking)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kResolve, 100_fp);
    red_stats_component.SetCurrentHealth(100_fp);

    // Calculate live stats
    StatsData red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kResolve), 100_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kWillpowerPercentage), 0_fp);

    // Add buff
    const auto effect_data = EffectData::CreateBuff(StatType::kResolve, EffectExpression::FromValue(50_fp), 300);
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName1";

        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
    }

    // Should compute to the current stats even before the TimeStep
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kResolve), 150_fp);

    world->TimeStep();

    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kResolve), 150_fp);

    // Add the effect a second time which should stack
    {
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName2";

        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);
    }

    for (int i = 0; i < 2; i++)
    {
        world->TimeStep();

        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kResolve), 200_fp);
    }

    world->TimeStep();
    // Second effect should expire after this timestep
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kResolve), 150_fp);

    // Should be active now
    world->TimeStep();
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kResolve), 100_fp);
    ASSERT_EQ(events_remove_effect.size(), 2);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kResolve), 100_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyDifferentBuff)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 0_fp);
    red_stats_component.SetCurrentHealth(100_fp);

    // Calculate live stats
    StatsData red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 100_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 0_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kWillpowerPercentage), 0_fp);

    // Add buff
    auto effect_data = EffectData::CreateBuff(StatType::kAttackSpeed, EffectExpression::FromValue(50_fp), 400);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Should compute to the current stats even before the TimeStep
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 150_fp);

    world->TimeStep();

    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 150_fp);

    auto priority_effect_data =
        EffectData::CreateBuff(StatType::kAttackPhysicalDamage, EffectExpression::FromValue(70_fp), 300);

    EffectState priority_effect_state{};
    priority_effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the priority effect which should take over
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, priority_effect_data, priority_effect_state);

    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 150_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 70_fp);

    for (int i = 0; i < 3; i++)
    {
        world->TimeStep();

        red_live_stats = world->GetLiveStats(red_entity_id);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 150_fp);
        EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 70_fp);
    }

    // Should be active now
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 150_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 70_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    red_live_stats = world->GetLiveStats(red_entity_id);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackSpeed), 100_fp);
    EXPECT_EQ(red_live_stats.Get(StatType::kAttackPhysicalDamage), 0_fp);
}

TEST_F(AttachedEffect_MultipleDOTs_OverlapTest, DifferentAbilities)
{
    RunTest("TestAbilityName1", "TestAbilityName2", 900_fp, 65_fp);
}

TEST_F(AttachedEffect_MultipleDOTs_OverlapTest, SameAbilities)
{
    // Less damage because of merge processing
    RunTest("TestAbilityName", "TestAbilityName", 1400_fp, 40_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyDots)
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
    // Do 200 damage over 4 seconds, tick every 1 second
    auto effect_data = EffectData::CreateDamageOverTime(
        EffectDamageType::kEnergy,
        EffectExpression::FromValue(200_fp),  // used actual values to match turtle exactly
        4000,
        1000);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    auto red_live_stats = world->GetLiveStats(red_entity_id);
    auto blue_live_stats = world->GetLiveStats(blue_entity_id);

    for (int i = 0; i < 41; i++)
    {
        // time step the world
        world->TimeStep();

        if (i == 10)
        {
            EXPECT_EQ(red_stats_component.GetCurrentHealth(), 1950_fp);
        }
        else if (i == 20)
        {
            EXPECT_EQ(red_stats_component.GetCurrentHealth(), 1900_fp);
        }
        else if (i == 30)
        {
            EXPECT_EQ(red_stats_component.GetCurrentHealth(), 1850_fp);
        }
        else if (i == 40)
        {
            ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1800_fp);
        }
    }

    world->TimeStep();

    red_live_stats = world->GetLiveStats(red_entity_id);
    blue_live_stats = world->GetLiveStats(blue_entity_id);

    // DOT does 200 damage due to error dues 250
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 1800_fp);
    EXPECT_EQ(red_stats_component.GetCurrentEnergy(), 52_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyAttackDamageBuff)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 1_fp);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackEnergyDamage, 4_fp);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackPureDamage, 7_fp);

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateBuff(StatType::kAttackDamage, EffectExpression::FromValue(10_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 11_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 14_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 17_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 11_fp) << "i = " << i;
        EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 14_fp) << "i = " << i;
        EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 17_fp) << "i = " << i;

        EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDamage), 12_fp) << "i = " << i;
        EXPECT_EQ(stats_component.GetAttackDamage(), 42_fp) << "i = " << i;
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 4);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.stat_type, StatType::kAttackDamage);

    world->TimeStep();

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 1_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 4_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 7_fp);
    EXPECT_EQ(stats_component.GetAttackDamage(), 12_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyExpressionAttackDamageBuff)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 1_fp);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackEnergyDamage, 4_fp);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackPureDamage, 7_fp);

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);

    const auto attack_damage = stats_component.GetAttackDamage();

    // Confirm attack_damage is all three damage types combined
    ASSERT_EQ(stats_component.GetAttackDamage(), 12_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateBuff(StatType::kAttackDamage, EffectExpression::FromValue(attack_damage), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 13_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 16_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 19_fp);

    EXPECT_EQ(stats_component.GetAttackDamage(), 48_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 13_fp);
        EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 16_fp);
        EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 19_fp);

        EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDamage), 12_fp);
        EXPECT_EQ(stats_component.GetAttackDamage(), 48_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 4);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.stat_type, StatType::kAttackDamage);

    world->TimeStep();

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 1_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 4_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 7_fp);
    EXPECT_EQ(stats_component.GetAttackDamage(), 12_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyAttackDamageBuffPercentage)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 10_fp);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackEnergyDamage, 40_fp);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackPureDamage, 70_fp);

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);

    // Confirm attack_damage is all three damage types combined
    ASSERT_EQ(stats_component.GetAttackDamage(), 120_fp);

    EffectLifetime effect_lifetime{};
    effect_lifetime.duration_time_ms = 300;
    effect_lifetime.frequency_time_ms = 0;
    effect_lifetime.max_stacks = 5;

    // Add an effect Wrap it in an attached effect
    {
        const auto expression = EffectExpression::FromStatPercentage(
            40_fp,
            StatType::kAttackDamage,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);

        EffectData effect_data;
        effect_data.type_id.type = EffectType::kBuff;
        effect_data.type_id.stat_type = StatType::kAttackDamage;
        effect_data.SetExpression(expression);
        effect_data.lifetime = effect_lifetime;

        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);

        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 14_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 56_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 98_fp);
    EXPECT_EQ(stats_component.GetAttackDamage(), 168_fp);

    const auto& attached_effects = blue_entity->Get<AttachedEffectsComponent>();

    // Number of attached effects should equal 1 (original effect) + the number of damage types
    EXPECT_EQ(attached_effects.attached_effects_.size(), 1 + kAttackDamageStatTypes.size());

    // Make sure all the effects have correct duplicated lifetime
    for (const auto& effect : attached_effects.attached_effects_)
    {
        EXPECT_EQ(effect->effect_data.lifetime.duration_time_ms, effect_lifetime.duration_time_ms);
        EXPECT_EQ(effect->effect_data.lifetime.frequency_time_ms, effect_lifetime.frequency_time_ms);
        EXPECT_EQ(effect->effect_data.lifetime.max_stacks, effect_lifetime.max_stacks);
    }

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();

        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 14_fp);
        EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 56_fp);
        EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 98_fp);

        EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDamage), 120_fp);
        EXPECT_EQ(stats_component.GetAttackDamage(), 168_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 4);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.stat_type, StatType::kAttackDamage);

    world->TimeStep();

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 10_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 40_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 70_fp);
    EXPECT_EQ(stats_component.GetAttackDamage(), 120_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyAttackDamageDebuff)
{
    // Get the relevant components
    auto& stats_component = blue_entity->Get<StatsComponent>();

    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackPhysicalDamage, 11_fp);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackEnergyDamage, 14_fp);
    stats_component.GetMutableTemplateStats().Set(StatType::kAttackPureDamage, 17_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateDebuff(StatType::kAttackDamage, EffectExpression::FromValue(10_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 1_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 4_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 7_fp);

    EXPECT_EQ(stats_component.GetAttackDamage(), 12_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 1_fp);
        EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 4_fp);
        EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 7_fp);

        EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDamage), 42_fp);
        EXPECT_EQ(stats_component.GetAttackDamage(), 12_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 4);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.stat_type, StatType::kAttackDamage);

    world->TimeStep();

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 11_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackEnergyDamage), 14_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackPureDamage), 17_fp);

    EXPECT_EQ(stats_component.GetAttackDamage(), 42_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyRangeBuff)
{
    // Calculate current range
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 10_fp);

    // Add an effect Wrap it in an attached effect
    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kAttackRangeUnits, EffectExpression::FromValue(10_fp), 300);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 20_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 20_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 20_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 10_fp);

    // Add Percentage
    {
        auto expression = EffectExpression::FromValue(60_fp);
        expression.is_used_as_percentage = true;

        const auto effect_data = EffectData::CreateBuff(StatType::kAttackRangeUnits, expression, 300);

        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName1";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 16_fp);

    // Add Value
    {
        auto expression = EffectExpression::FromValue(4_fp);
        expression.is_used_as_percentage = false;

        const auto effect_data = EffectData::CreateBuff(StatType::kAttackRangeUnits, expression, 300);

        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName2";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), "22.4"_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectEnergyBurnOverTime)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.SetCurrentEnergy(10000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateEnergyBurnOverTime(
        EffectExpression::FromValue(10000_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    auto red_stats = world->GetLiveStats(red_entity_id);
    int precision = 0;

    // Should expire after 10 steps
    for (int i = 1; i <= 11; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Update the current stats
        red_stats = world->GetLiveStats(red_entity_id);

        if (i != 1)
        {
            precision += 1;
        }
        if (i != 11)
        {
            ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 10000_fp - FixedPoint::FromInt(1000 * i - precision))
                << " i = " << i;
        }
        else
        {
            // Because of precision loss and energy regen we end with 10 energy
            ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 10_fp);
        }
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last timestep
    world->TimeStep();

    // Update the current stats
    red_stats = world->GetLiveStats(red_entity_id);

    // Energy burn should be over, we regen 1 energy from energy regen
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 11_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectExecuteNoDamage)
{
    // Listen for events
    std::vector<event_data::CombatUnitStateChanged> events_state_changed;
    std::vector<event_data::Fainted> events_fainted;

    // Listen to the effect specific events
    world->SubscribeToEvent(
        EventType::kCombatUnitStateChanged,
        [&events_state_changed](const Event& event)
        {
            events_state_changed.emplace_back(event.Get<event_data::CombatUnitStateChanged>());
        });
    world->SubscribeToEvent(
        EventType::kFainted,
        [&events_fainted](const Event& event)
        {
            events_fainted.emplace_back(event.Get<event_data::Fainted>());
        });

    // Add an effect
    auto effect_data = EffectData::CreateExecute(EffectExpression::FromValue(50_fp), 1000, {AbilityType::kAttack});

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    auto red_stats = world->GetLiveStats(red_entity_id);

    // Should expire after 10 steps
    for (int i = 1; i <= 11; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Update the current stats
        red_stats = world->GetLiveStats(red_entity_id);

        // Check health state
        EXPECT_EQ(red_stats.Get(StatType::kCurrentHealth), 1000_fp);
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last timestep
    world->TimeStep();

    // Update the current stats
    red_stats = world->GetLiveStats(red_entity_id);

    // Verify events
    EXPECT_EQ(events_state_changed.size(), 0);
    EXPECT_EQ(events_fainted.size(), 0);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectExecuteWithDamage)
{
    // Listen for events
    std::vector<event_data::CombatUnitStateChanged> events_state_changed;
    std::vector<event_data::Fainted> events_fainted;

    // Listen to the effect specific events
    world->SubscribeToEvent(
        EventType::kCombatUnitStateChanged,
        [&events_state_changed](const Event& event)
        {
            events_state_changed.emplace_back(event.Get<event_data::CombatUnitStateChanged>());
        });
    world->SubscribeToEvent(
        EventType::kFainted,
        [&events_fainted](const Event& event)
        {
            events_fainted.emplace_back(event.Get<event_data::Fainted>());
        });

    // Add effects
    const auto execute_effect_data =
        EffectData::CreateExecute(EffectExpression::FromValue(50_fp), 1000, {AbilityType::kAttack});
    const auto dot_effect_data = EffectData::CreateDamageOverTime(
        EffectDamageType::kPure,
        EffectExpression::FromValue(750_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState execute_effect_state{};
    execute_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    execute_effect_state.source_context.combat_unit_ability_type = AbilityType::kInnate;

    EffectState dot_effect_state{};
    dot_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    dot_effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

    // Add the effects
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*blue_entity, blue_entity_id, execute_effect_data, execute_effect_state);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, dot_effect_data, dot_effect_state);

    auto red_stats = world->GetLiveStats(red_entity_id);

    // Should expire after 10 steps
    for (int i = 1; i <= 11; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Update the current stats
        red_stats = world->GetLiveStats(red_entity_id);

        // Check health state
        if (i > 6)
        {
            // Execute already applied
            EXPECT_EQ(red_stats.Get(StatType::kCurrentHealth), 0_fp);
        }
        else
        {
            // Normal damage over time
            EXPECT_EQ(red_stats.Get(StatType::kCurrentHealth), 1000_fp - FixedPoint::FromInt(750 * i / 10));
        }
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Update the current stats
    red_stats = world->GetLiveStats(red_entity_id);

    // Verify events
    EXPECT_EQ(events_state_changed.size(), 1);
    EXPECT_EQ(events_fainted.size(), 1);
    EXPECT_EQ(events_fainted[0].death_reason, DeathReasonType::kExecution);
    EXPECT_EQ(events_fainted[0].source_context.combat_unit_ability_type, AbilityType::kInnate);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectExecuteWithWrongDamage)
{
    // Listen for events
    std::vector<event_data::CombatUnitStateChanged> events_state_changed;
    std::vector<event_data::Fainted> events_fainted;

    // Listen to the effect specific events
    world->SubscribeToEvent(
        EventType::kCombatUnitStateChanged,
        [&events_state_changed](const Event& event)
        {
            events_state_changed.emplace_back(event.Get<event_data::CombatUnitStateChanged>());
        });
    world->SubscribeToEvent(
        EventType::kFainted,
        [&events_fainted](const Event& event)
        {
            events_fainted.emplace_back(event.Get<event_data::Fainted>());
        });

    // Add effects
    auto execute_effect_data =
        EffectData::CreateExecute(EffectExpression::FromValue(50_fp), 1000, {AbilityType::kAttack});
    auto dot_effect_data = EffectData::CreateDamageOverTime(
        EffectDamageType::kPure,
        EffectExpression::FromValue(750_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    effect_state.source_context.combat_unit_ability_type = AbilityType::kOmega;

    // Add the effects
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, execute_effect_data, effect_state);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, dot_effect_data, effect_state);

    auto red_stats = world->GetLiveStats(red_entity_id);

    // Should expire after 10 steps
    for (int i = 1; i <= 10; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Update the current stats
        red_stats = world->GetLiveStats(red_entity_id);

        // Check health state
        // Normal damage over time
        EXPECT_EQ(red_stats.Get(StatType::kCurrentHealth), 1000_fp - FixedPoint::FromInt(750 * i / 10));
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last timestep
    world->TimeStep();

    // Update the current stats
    red_stats = world->GetLiveStats(red_entity_id);

    // Verify events
    EXPECT_EQ(events_state_changed.size(), 0);
    EXPECT_EQ(events_fainted.size(), 0);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectBlink)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // Set up a reservation
    auto& blue_position_component = blue_entity->Get<PositionComponent>();
    blue_position_component.SetReservedPosition(HexGridPosition(25, -30));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateBlink(ReservedPositionType::kBehindReceiver, 150, 300);
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

        // Check position
        if (i < 1)
        {
            EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(-20, -40));
        }
        else
        {
            EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(25, -30));
        }
    }
    ASSERT_FALSE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBlink);

    // Should be targetable
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsTargetable(*world, blue_entity_id));
}

TEST_F(AttachedEffectsSystemTest, DeactivateOnBattleFinished)
{
    // Get the relevant components
    auto& attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();

    // Add an untargetable state
    auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // TimeStep the world
    world->TimeStep();

    // Should be active now
    ASSERT_TRUE(attached_effects_component.HasPositiveState(EffectPositiveState::kUntargetable));
    ASSERT_FALSE(EntityHelper::IsTargetable(*world, blue_entity_id));
    ASSERT_EQ(events_remove_effect.size(), 0);

    // Abruptly end the battle
    world->BuildAndEmitEvent<EventType::kBattleFinished>(Team::kRed);

    // TimeStep the world
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kUntargetable);

    // Should no longer be active
    ASSERT_FALSE(attached_effects_component.HasPositiveState(EffectPositiveState::kUntargetable));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectHyperGainOverTime)
{
    // Get the relevant components
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Set the original health
    constexpr auto original_hyper = 100_fp;
    blue_stats_component.SetCurrentSubHyper(original_hyper);

    // Validate initial state
    ASSERT_EQ(blue_stats_component.GetCurrentSubHyper(), original_hyper);

    // Should heal 10 per time step, and only 5 at the end
    constexpr auto expected_hyper_gain_amount = 105_fp;
    constexpr auto expected_hyper_per_time_step = expected_hyper_gain_amount / 10_fp;
    auto effect_data = EffectData::CreateHyperGainOverTime(
        EffectExpression::FromValue(expected_hyper_gain_amount),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect on self
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    // NOTE: because we attach the effect directly, there should be no activation effect type
    // Listen to hyper changed events
    std::vector<event_data::StatChanged> events_hyper_changed;
    world->SubscribeToEvent(
        EventType::kHyperChanged,
        [&events_hyper_changed](const Event& event)
        {
            events_hyper_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    auto blue_stats = world->GetLiveStats(blue_entity_id);

    constexpr int num_time_steps = 10;
    for (int i = 0; i < num_time_steps; i++)
    {
        // Clear before each call
        events_apply_effect.clear();
        events_hyper_changed.clear();

        // TimeStep the world
        world->TimeStep();

        // Update blue stats
        blue_stats = world->GetLiveStats(blue_entity_id);

        // Not the last step, should equal the expected_hyper_per_time_step
        if (i == num_time_steps - 1)
        {
            continue;
        }

        // Should have fired hyper changed event
        ASSERT_EQ(events_hyper_changed.size(), 1) << " i = " << i;

        // The apply effect over time effect should have fired
        ASSERT_EQ(events_apply_effect.size(), 1) << " i = " << i;

        const auto& sent_effect = events_apply_effect.at(0).data;
        ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantHyperGain) << " i = " << i;
    }

    // Update blue stats
    blue_stats = world->GetLiveStats(blue_entity_id);

    // Should have fired a final hyper effect
    ASSERT_EQ(events_apply_effect.size(), 1);
    ASSERT_EQ(events_hyper_changed.size(), 1);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantHyperGain);

    const ExpressionEvaluationContext expression_context(world.get(), blue_entity_id, blue_entity_id);

    // Last time step should add the precision back
    ASSERT_EQ(sent_effect.GetExpression().Evaluate(expression_context), expected_hyper_per_time_step);

    // Does the total make sense?
    ASSERT_EQ(blue_stats_component.GetCurrentSubHyper(), original_hyper + expected_hyper_gain_amount);

    // TimeStep again
    events_apply_effect.clear();
    events_hyper_changed.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired a final removal effect
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kHyperGainOverTime);

    // Check this effect does not reactivate
    events_remove_effect.clear();
    world->TimeStep();
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 0);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectHyperBurnOverTime)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.SetCurrentSubHyper(10000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateHyperBurnOverTime(
        EffectExpression::FromValue(10000_fp),
        1000,
        kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    auto red_stats = world->GetLiveStats(red_entity_id);

    // Should expire after 10 steps
    for (int i = 1; i < 11; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Update the current stats
        red_stats = world->GetLiveStats(red_entity_id);

        ASSERT_EQ(red_stats_component.GetCurrentSubHyper(), FixedPoint::FromInt(10000 - (1000 * i))) << " i = " << i;
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last timestep
    world->TimeStep();

    // Update the current stats
    red_stats = world->GetLiveStats(red_entity_id);

    // Energy burn should be over, we regen 1 energy from energy regen
    ASSERT_EQ(red_stats_component.GetCurrentSubHyper(), 0_fp);
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyMoveSpeedBuff)
{
    // Get the relevant components
    const auto& movement_component = blue_entity->Get<MovementComponent>();

    // Current stats
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 5000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateBuff(StatType::kMoveSpeedSubUnits, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();

        // Should be active now
        EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 5100_fp);
    }

    // Should be active now
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 5100_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 5000_fp);
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyMoveSpeedDebuff)
{
    // Get the relevant components
    const auto& movement_component = blue_entity->Get<MovementComponent>();

    // Current stats
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 5000_fp);

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateDebuff(StatType::kMoveSpeedSubUnits, EffectExpression::FromValue(100_fp), 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();

        // Should be active now
        EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 4900_fp);
    }

    // Should be active now
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 4900_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kDebuff);

    // Should no longer be active
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 5000_fp);
}

TEST_F(AttachedEffectsSystemTest, ApplyCritAmplificationBuff)
{
    // Calculate live stats
    StatsData live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kCritAmplificationPercentage), 150_fp);

    // Add an effect Wrap it in an attached effect
    {
        const auto effect_data =
            EffectData::CreateBuff(StatType::kCritAmplificationPercentage, EffectExpression::FromValue(10_fp), 300);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName1";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Validate initial state
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kCritAmplificationPercentage), 160_fp);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        world->TimeStep();
        live_stats = world->GetLiveStats(blue_entity_id);

        // Should be active now
        EXPECT_EQ(live_stats.Get(StatType::kCritAmplificationPercentage), 160_fp);
    }

    // Should be active now
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kCritAmplificationPercentage), 160_fp);
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kBuff);

    // Should no longer be active
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kCritAmplificationPercentage), 150_fp);

    // Apply percentage buff
    {
        auto expression = EffectExpression::FromValue(50_fp);
        expression.is_used_as_percentage = true;

        auto effect_data = EffectData::CreateBuff(StatType::kCritAmplificationPercentage, expression, 100);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName2";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    // Expression is used as a percentage but since the stat is also a
    // Percentage the buff should be calculated as a standard buff
    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kCritAmplificationPercentage), 200_fp);

    // Add regular
    {
        auto expression = EffectExpression::FromValue(25_fp);
        expression.is_used_as_percentage = false;

        auto effect_data = EffectData::CreateBuff(StatType::kCritAmplificationPercentage, expression, 100);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue_entity_id);
        effect_state.source_context.ability_name = "AbilityName3";
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
    }

    live_stats = world->GetLiveStats(blue_entity_id);
    EXPECT_EQ(live_stats.Get(StatType::kCritAmplificationPercentage), 225_fp);
}

}  // namespace simulation
