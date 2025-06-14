#include "attached_effects_system_data_fixtures.h"
#include "utility/attached_effects_helper.h"
#include "utility/stats_helper.h"

namespace simulation
{

TEST_F(AttachedEffectsSystemTest, ApplyConditionPoison)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    const auto max_health = 1000000_fp;
    red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, max_health);
    red_stats_component.SetCurrentHealth(max_health);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsPoisoned(*red_entity));

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    auto red_stats = world->GetLiveStats(red_entity_id);
    auto blue_stats = world->GetLiveStats(blue_entity_id);

    // Should expire after 80 time steps
    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kPoison);
    int num_time_steps = Time::MsToTimeSteps(dot_config.duration_ms);
    for (int i = 0; i < num_time_steps - 1; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Update the current stats
        red_stats = world->GetLiveStats(red_entity_id);
        blue_stats = world->GetLiveStats(blue_entity_id);

        // Should be poisoned
        ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity)) << " i = " << i;

        // Blue should not be poisoned
        ASSERT_FALSE(EntityHelper::IsPoisoned(*blue_entity)) << " i = " << i;

        // // No effects should have fired
        // ASSERT_EQ(events_apply_effect.size(), 1) << " i = " << i;
        //
        // const auto& sent_effect = events_apply_effect.at(0).data;
        // ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantDamage);
        // ASSERT_EQ(sent_effect.type_id.damage_type, EffectDamageType::kPure) << "Poison period events is not
        // poison."
        //                                                                     << " i = " << i;
        // ASSERT_EQ(events_apply_effect.at(0).state.sender_live_stats.Get(StatType::kAttackPhysicalDamage), 0) << " i =
        // " << i;
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last time step
    world->TimeStep();

    // Update the current stats
    red_stats = world->GetLiveStats(red_entity_id);
    blue_stats = world->GetLiveStats(blue_entity_id);

    // Should have fired a final pure damage effect
    ASSERT_EQ(events_apply_effect.size(), 1);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantDamage);
    ASSERT_EQ(sent_effect.type_id.damage_type, EffectDamageType::kPure) << "Poison final time step was not pure damage";

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), max_health - 50000_fp)
        << "Poison effect did not have the desired health effect on the entity health";

    // Should be poisoned
    ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should have fired a final removal effect for the attached events and condition
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 2);
    {
        const auto& sent_effect1 = events_remove_effect.at(0).data;
        const auto& sent_effect2 = events_remove_effect.at(1).data;

        // Should fire another 2 attached effects on the first time step
        ASSERT_EQ(sent_effect1.type_id.type, EffectType::kCondition);
        ASSERT_EQ(sent_effect2.type_id.type, EffectType::kDamageOverTime);

        ASSERT_EQ(sent_effect1.type_id.condition_type, EffectConditionType::kPoison);
        ASSERT_EQ(sent_effect2.type_id.damage_type, EffectDamageType::kPure);

        // Check duration
        ASSERT_EQ(sent_effect1.lifetime.duration_time_ms, dot_config.duration_ms);
        ASSERT_EQ(sent_effect2.lifetime.duration_time_ms, dot_config.duration_ms);
    }

    // Should not be poisoned
    ASSERT_FALSE(EntityHelper::IsPoisoned(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsPoisoned(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyConditionWound)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    StatsHelper::SetDefaults(red_stats_component.GetMutableTemplateStats());

    // set sane values
    static constexpr auto scale = 1000_fp;
    const auto max_health = 10000_fp * scale;
    red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, max_health);
    red_stats_component.SetCurrentHealth(max_health);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCritAmplificationPercentage, 100_fp);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsWounded(*red_entity));

    // Add an effect, then Wrap it in an attached effect
    const auto effect_package_value = 5000_fp * scale;
    auto effect_data = EffectData::CreateCondition(EffectConditionType::kWound, kDefaultAttachedEffectsFrequencyMs);
    effect_data.SetExpression(EffectExpression::FromValue(effect_package_value));

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Should expire after 80 time steps
    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kWound);
    const int num_time_steps = Time::MsToTimeSteps(dot_config.duration_ms);
    for (int i = 0; i < num_time_steps - 1; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // time step the world
        world->TimeStep();

        // Should be wounded
        ASSERT_TRUE(EntityHelper::IsWounded(*red_entity)) << " i = " << i;

        // Blue should not be wounded
        ASSERT_FALSE(EntityHelper::IsWounded(*blue_entity)) << " i = " << i;
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last time step
    world->TimeStep();

    // Should have fired a final physical damage effect
    ASSERT_EQ(events_apply_effect.size(), 1);

    const auto& sent_effect = events_apply_effect.at(0).data;
    ASSERT_EQ(sent_effect.type_id.type, EffectType::kInstantDamage);
    ASSERT_EQ(sent_effect.type_id.damage_type, EffectDamageType::kPhysical)
        << "Wound final time step was not physical damage";

    // 1.25% of the Effect Package as Physical Damage per second per stack.
    ASSERT_EQ(
        red_stats_component.GetCurrentHealth(),
        max_health - dot_config.dot_high_precision_percentage.AsHighPrecisionPercentageOf(effect_package_value))
        << "Wound effect did not have the desired health effect on the entity health";
    ASSERT_LT(red_stats_component.GetCurrentHealth(), max_health);

    // Should be wounded
    ASSERT_TRUE(EntityHelper::IsWounded(*red_entity));

    // time step again
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should have fired a final removal effect for the attached events and condition
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 2);
    {
        const auto& sent_effect1 = events_remove_effect.at(0).data;
        const auto& sent_effect2 = events_remove_effect.at(1).data;

        // Should fire another 2 attached effects on the first time step
        ASSERT_EQ(sent_effect1.type_id.type, EffectType::kCondition);
        ASSERT_EQ(sent_effect2.type_id.type, EffectType::kDamageOverTime);

        ASSERT_EQ(sent_effect1.type_id.condition_type, EffectConditionType::kWound);
        ASSERT_EQ(sent_effect2.type_id.damage_type, EffectDamageType::kPhysical);

        // Check duration
        ASSERT_EQ(sent_effect1.lifetime.duration_time_ms, dot_config.duration_ms);
        ASSERT_EQ(sent_effect2.lifetime.duration_time_ms, dot_config.duration_ms);
    }

    // Should not be wounded
    ASSERT_FALSE(EntityHelper::IsWounded(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsWounded(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyConditionBurn)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set sane values
    constexpr auto max_health = 4000000_fp;
    constexpr auto initial_current_health = 2000000_fp;
    constexpr auto missing_health = max_health - initial_current_health;
    red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, max_health);
    red_stats_component.SetCurrentHealth(initial_current_health);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));

    // Add an effect Wrap it in an attached effect
    auto effect_data = EffectData::CreateCondition(EffectConditionType::kBurn, kDefaultAttachedEffectsFrequencyMs);

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    // Add the effect
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Should expire after 40 steps
    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kBurn);
    int num_steps = Time::MsToTimeSteps(dot_config.duration_ms);
    for (int i = 0; i < num_steps - 1; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Should be burned
        ASSERT_TRUE(EntityHelper::IsBurned(*red_entity)) << " i = " << i;

        // Blue should not be burned
        ASSERT_FALSE(EntityHelper::IsBurned(*blue_entity)) << " i = " << i;
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last TimeStep
    world->TimeStep();

    // 2% of missing current health
    ASSERT_EQ(
        red_stats_component.GetCurrentHealth(),
        initial_current_health - dot_config.dot_high_precision_percentage.AsHighPrecisionPercentageOf(missing_health))
        << "Burn effect did not have the desired health effect on the entity health";

    // Should be burned
    ASSERT_TRUE(EntityHelper::IsBurned(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should have fired a final removal effect for the attached events and condition
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 2);
    {
        const auto& sent_effect1 = events_remove_effect.at(0).data;
        const auto& sent_effect2 = events_remove_effect.at(1).data;

        // Should fire another 2 attached effects on the first step
        ASSERT_EQ(sent_effect1.type_id.type, EffectType::kCondition);
        ASSERT_EQ(sent_effect2.type_id.type, EffectType::kDamageOverTime);

        ASSERT_EQ(sent_effect1.type_id.condition_type, EffectConditionType::kBurn);
        ASSERT_EQ(sent_effect2.type_id.damage_type, EffectDamageType::kEnergy);

        // Check duration
        ASSERT_EQ(sent_effect1.lifetime.duration_time_ms, dot_config.duration_ms);
        ASSERT_EQ(sent_effect2.lifetime.duration_time_ms, dot_config.duration_ms);
    }

    // Should not be burned
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyConditionFrost)
{
    // Get the relevant components
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Modify attack sped
    {
        auto red_template_stats = red_stats_component.GetTemplateStats();
        red_template_stats.Set(StatType::kAttackSpeed, 200_fp);
        red_stats_component.SetTemplateStats(red_template_stats);
    }

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsFrosted(*red_entity));

    // Add Frost effect
    auto frost_effect_data =
        EffectData::CreateCondition(EffectConditionType::kFrost, kDefaultAttachedEffectsFrequencyMs);
    EffectState frost_effect_state{};
    frost_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, frost_effect_data, frost_effect_state);

    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kFrost);
    int steps = Time::MsToTimeSteps(dot_config.duration_ms);
    for (int i = 0; i < steps - 1; ++i)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Should be Frosted
        ASSERT_TRUE(EntityHelper::IsFrosted(*red_entity)) << " i = " << i;

        // Blue should not be Frosted
        ASSERT_FALSE(EntityHelper::IsFrosted(*blue_entity)) << " i = " << i;

        // Should have a debuff for Attack Speed
        ASSERT_TRUE(red_attached_effects_component.HasDebuffFor(StatType::kAttackSpeed)) << " i = " << i;

        // No effects should have fired
        ASSERT_EQ(events_apply_effect.size(), 0) << " i = " << i;
    }

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last TimeStep
    world->TimeStep();

    // Should have fired a final effect
    ASSERT_EQ(events_apply_effect.size(), 0);

    // Attack Speed should be reduced by 3%
    const FixedPoint base_attack_speed = red_stats_component.GetBaseValueForType(StatType::kAttackSpeed);
    const FixedPoint reduced_attack_speed =
        base_attack_speed - (base_attack_speed * dot_config.debuff_percentage / 100_fp);
    auto red_live_stats = world->GetLiveStats(red_entity_id);
    ASSERT_EQ(red_live_stats.Get(StatType::kAttackSpeed), reduced_attack_speed);

    // Should be Frosted
    ASSERT_TRUE(EntityHelper::IsFrosted(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should have fired a final removal effect for the attached events and condition
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 2);
    {
        const auto& sent_effect1 = events_remove_effect.at(0).data;
        const auto& sent_effect2 = events_remove_effect.at(1).data;

        // Should fire another 2 attached effects on the first step
        ASSERT_EQ(sent_effect1.type_id.type, EffectType::kCondition);
        ASSERT_EQ(sent_effect2.type_id.type, EffectType::kDebuff);

        ASSERT_EQ(sent_effect1.type_id.condition_type, EffectConditionType::kFrost);
        ASSERT_EQ(sent_effect2.type_id.damage_type, EffectDamageType::kNone);

        // Check duration
        ASSERT_EQ(sent_effect1.lifetime.duration_time_ms, dot_config.duration_ms);
        ASSERT_EQ(sent_effect2.lifetime.duration_time_ms, dot_config.duration_ms);
    }

    // Should not be Frosted
    ASSERT_FALSE(EntityHelper::IsFrosted(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyConditionPoisonReachedMaxStacks)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();

    // Modify health gain efficiency
    constexpr auto max_health = 100000_fp;
    constexpr auto initial_health_gain_efficiency_percentage = 150_fp;
    red_stats_component.GetMutableTemplateStats().Set(
        StatType::kHealthGainEfficiencyPercentage,
        initial_health_gain_efficiency_percentage);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 100000_fp);
    auto red_live_stats = world->GetLiveStats(*red_entity);
    ASSERT_EQ(red_live_stats.Get(StatType::kHealthGainEfficiencyPercentage), initial_health_gain_efficiency_percentage);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsPoisoned(*red_entity));

    // Add Poison effect
    auto poison_effect_data =
        EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kPoison);
    for (int stack = 1; stack <= dot_config.max_stacks - 1; stack++)
    {
        // Attach effect
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, poison_effect_data, effect_state);

        // Clear before each call
        events_apply_effect.clear();
        events_remove_effect.clear();

        // Last TimeStep
        world->TimeStep();

        // Should have fired a final effect
        ASSERT_EQ(events_apply_effect.size(), 0);

        // Should be Poisoned
        ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity));

        // Debuff should not have fired yet
        red_live_stats = world->GetLiveStats(*red_entity);
        ASSERT_EQ(
            red_live_stats.Get(StatType::kHealthGainEfficiencyPercentage),
            initial_health_gain_efficiency_percentage);
    }

    // Should be Poisoned
    ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity));

    // At max stacks activate the max_stacks
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, poison_effect_data, effect_state);

    // Need to be Poisoned
    ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity));

    // Debuff should have fired
    red_live_stats = world->GetLiveStats(*red_entity);
    ASSERT_EQ(
        red_live_stats.Get(StatType::kHealthGainEfficiencyPercentage),
        initial_health_gain_efficiency_percentage - 100_fp);

    // Dots should have max_stacks set
    const auto& active_dots = red_attached_effects_component.GetDots();

    // Check DOT expressions
    {
        ASSERT_EQ(active_dots.size(), 1);
        const auto& dot_attached_effect = active_dots.at(0);
        const FixedPoint dot_expression_value =
            world->EvaluateExpression(dot_attached_effect->effect_data.GetExpression(), blue_entity_id, red_entity_id);
        EXPECT_EQ(
            dot_expression_value,
            FixedPoint::FromInt(dot_config.max_stacks) *
                dot_config.dot_high_precision_percentage.AsHighPrecisionPercentageOf(max_health));
    }

    // Time step
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should still be poisoned
    ASSERT_FALSE(dot_config.cleanse_on_max_stacks);
    ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity));

    // Time step
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should still be able to activate after max stacks
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, poison_effect_data, effect_state);

    // Debuff Should still be the same
    red_live_stats = world->GetLiveStats(*red_entity);
    ASSERT_EQ(
        red_live_stats.Get(StatType::kHealthGainEfficiencyPercentage),
        initial_health_gain_efficiency_percentage - 100_fp);

    // Check DOT expression, should be the same value
    {
        ASSERT_EQ(active_dots.size(), 1);
        const auto& dot_attached_effect = active_dots.at(0);
        const FixedPoint dot_expression_value =
            world->EvaluateExpression(dot_attached_effect->effect_data.GetExpression(), blue_entity_id, red_entity_id);
        EXPECT_EQ(
            dot_expression_value,
            FixedPoint::FromInt(dot_config.max_stacks) *
                dot_config.dot_high_precision_percentage.AsHighPrecisionPercentageOf(max_health));
    }
    ASSERT_TRUE(EntityHelper::IsPoisoned(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyConditionWoundReachedMaxStacks)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();

    // Modify crit amplification
    constexpr auto max_health = 100000_fp;
    constexpr auto initial_crit_amplification_percentage = 150_fp;
    red_stats_component.GetMutableTemplateStats().Set(
        StatType::kCritAmplificationPercentage,
        initial_crit_amplification_percentage);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, max_health);
    auto red_live_stats = world->GetLiveStats(*red_entity);
    ASSERT_EQ(red_live_stats.Get(StatType::kCritAmplificationPercentage), 150_fp);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsWounded(*red_entity));

    // Add Wound effect
    const auto effect_package_value = 200000_fp;
    auto wound_effect_data =
        EffectData::CreateCondition(EffectConditionType::kWound, kDefaultAttachedEffectsFrequencyMs);
    wound_effect_data.SetExpression(EffectExpression::FromValue(effect_package_value));
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kWound);
    for (int stack = 1; stack <= dot_config.max_stacks - 1; stack++)
    {
        // Attach effect
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, wound_effect_data, effect_state);

        // Clear before each call
        events_apply_effect.clear();
        events_remove_effect.clear();

        // Last TimeStep
        world->TimeStep();

        // Should have fired a final effect
        ASSERT_EQ(events_apply_effect.size(), 0);

        // Should be Wounded
        ASSERT_TRUE(EntityHelper::IsWounded(*red_entity));

        // Debuff should not have fired yet
        red_live_stats = world->GetLiveStats(*red_entity);
        EXPECT_EQ(red_live_stats.Get(StatType::kCritAmplificationPercentage), initial_crit_amplification_percentage);
    }

    // Should be Wounded
    ASSERT_TRUE(EntityHelper::IsWounded(*red_entity));

    // At max stacks activate the max_stacks
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, wound_effect_data, effect_state);

    // Need to be Wounded
    ASSERT_TRUE(EntityHelper::IsWounded(*red_entity));

    // Debuff should have fired
    red_live_stats = world->GetLiveStats(*red_entity);
    ASSERT_EQ(
        red_live_stats.Get(StatType::kCritAmplificationPercentage),
        initial_crit_amplification_percentage - 100_fp);

    // Dots should have max_stacks set
    const auto& active_dots = red_attached_effects_component.GetDots();
    ASSERT_EQ(active_dots.size(), 1);

    // 1.25% of the Effect Package as Physical Damage per second per stack.
    const auto& dot_attached_effect = active_dots.at(0);

    // Check DOT expression
    {
        const FixedPoint dot_expression_value =
            world->EvaluateExpression(dot_attached_effect->effect_data.GetExpression(), blue_entity_id, red_entity_id);

        const FixedPoint expected_without_stacks =
            dot_config.dot_high_precision_percentage.AsHighPrecisionPercentageOf(effect_package_value);
        EXPECT_EQ(expected_without_stacks, 20000_fp);
        const FixedPoint expected_dot_expression_value =
            FixedPoint::FromInt(dot_config.max_stacks) * expected_without_stacks;
        EXPECT_EQ(dot_expression_value, expected_dot_expression_value);
    }

    // Time step
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should still be wounded
    ASSERT_FALSE(dot_config.cleanse_on_max_stacks);
    ASSERT_TRUE(EntityHelper::IsWounded(*red_entity));

    // Time step
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should still be able to activate after max stacks
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, wound_effect_data, effect_state);

    // Debuff Should still be the same
    red_live_stats = world->GetLiveStats(*red_entity);
    ASSERT_EQ(
        red_live_stats.Get(StatType::kCritAmplificationPercentage),
        initial_crit_amplification_percentage - 100_fp);

    // Check DOT expression, should be the same value
    {
        ASSERT_EQ(active_dots.size(), 1);
        const auto dot_expression_value =
            world->EvaluateExpression(dot_attached_effect->effect_data.GetExpression(), blue_entity_id, red_entity_id);

        const auto expected_without_stacks =
            dot_config.dot_high_precision_percentage.AsHighPrecisionPercentageOf(effect_package_value);
        EXPECT_EQ(expected_without_stacks, 20000_fp);
        const auto expected_dot_expression_value = FixedPoint::FromInt(dot_config.max_stacks) * expected_without_stacks;
        EXPECT_EQ(dot_expression_value, expected_dot_expression_value);
    }

    ASSERT_TRUE(EntityHelper::IsWounded(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyConditionBurnReachedMaxStacks)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();

    // Set sane values
    constexpr auto max_health = 4000000_fp;
    constexpr auto initial_current_health = 2000000_fp;
    constexpr auto missing_health = max_health - initial_current_health;
    red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, max_health);
    red_stats_component.SetCurrentHealth(initial_current_health);

    // Listen to the zone created events
    std::vector<event_data::ZoneCreated> events_zones_created;
    world->SubscribeToEvent(
        EventType::kZoneCreated,
        [&](const Event& event)
        {
            events_zones_created.emplace_back(event.Get<event_data::ZoneCreated>());
        });

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), initial_current_health);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));

    // Add Wound effect
    const auto burn_effect_data =
        EffectData::CreateCondition(EffectConditionType::kBurn, kDefaultAttachedEffectsFrequencyMs);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kBurn);
    for (int stack = 1; stack <= dot_config.max_stacks - 1; stack++)
    {
        // Attach effect
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, burn_effect_data, effect_state);

        // Clear before each call
        events_apply_effect.clear();
        events_remove_effect.clear();

        // Last TimeStep
        world->TimeStep();

        // Should have fired a final effect
        ASSERT_EQ(events_apply_effect.size(), 0);

        // Should be Burned
        ASSERT_TRUE(EntityHelper::IsBurned(*red_entity));

        // Zone should not have fired yet
        ASSERT_EQ(events_zones_created.size(), 0);
    }

    // Should be Burned
    ASSERT_TRUE(EntityHelper::IsBurned(*red_entity));

    // Check final dot
    const auto& active_dots = red_attached_effects_component.GetDots();

    // Check DOT expression
    // 2% of missing current health
    {
        ASSERT_EQ(active_dots.size(), 1);
        const auto& dot_attached_effect = active_dots.at(0);

        const auto expected_without_stacks =
            dot_config.dot_high_precision_percentage.AsHighPrecisionPercentageOf(missing_health);
        const auto expected_dot_expression_value =
            FixedPoint::FromInt(dot_config.max_stacks - 1) * expected_without_stacks;

        const auto dot_expression_value =
            world->EvaluateExpression(dot_attached_effect->effect_data.GetExpression(), blue_entity_id, red_entity_id);
        EXPECT_EQ(dot_expression_value, expected_dot_expression_value);
    }

    // At max stacks activate the max_stacks zone
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, burn_effect_data, effect_state);

    // Need NOT to be Burned
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));

    // Zone should have fired
    ASSERT_EQ(events_zones_created.size(), 1);
    const auto& zone_event_data = events_zones_created.at(0);
    EXPECT_EQ(zone_event_data.combat_unit_sender_id, blue_entity_id);
    EXPECT_EQ(zone_event_data.duration_ms, 100);

    // No more dots
    EXPECT_EQ(active_dots.size(), 0);

    // Time step
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // Should still be burned
    EXPECT_TRUE(dot_config.cleanse_on_max_stacks);
    EXPECT_FALSE(EntityHelper::IsBurned(*red_entity));
    EXPECT_FALSE(EntityHelper::IsBurned(*blue_entity));

    // Register zone hit
    ASSERT_EQ(events_apply_effect.size(), 1);
    const auto& zone_instant_damage_event = events_apply_effect.at(0);
    EXPECT_EQ(zone_instant_damage_event.data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(zone_instant_damage_event.data.type_id.damage_type, EffectDamageType::kPure);
    EXPECT_EQ(zone_instant_damage_event.receiver_id, red_entity_id);
    EXPECT_EQ(zone_instant_damage_event.sender_id, zone_event_data.entity_id);
}

TEST_F(AttachedEffectsSystemTest, ApplyConditionFrostReachedMaxStacks)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Modify attack sped
    {
        auto red_template_stats = red_stats_component.GetTemplateStats();
        red_template_stats.Set(StatType::kAttackSpeed, 200_fp);
        red_stats_component.SetTemplateStats(red_template_stats);
    }

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsFrosted(*red_entity));
    ASSERT_FALSE(EntityHelper::IsFrozen(*red_entity));

    // Add Frost effect
    auto frost_effect_data =
        EffectData::CreateCondition(EffectConditionType::kFrost, kDefaultAttachedEffectsFrequencyMs);
    EffectState frost_effect_state{};
    frost_effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kFrost);
    for (int stack = 1; stack <= dot_config.max_stacks - 1; stack++)
    {
        // Attach effect
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, blue_entity_id, frost_effect_data, frost_effect_state);

        // Clear before each call
        events_apply_effect.clear();
        events_remove_effect.clear();

        // Last TimeStep
        world->TimeStep();

        // Should have fired a final effect
        ASSERT_EQ(events_apply_effect.size(), 0);

        // Attack Speed should be reduced by 3%
        const auto base_attack_speed = red_stats_component.GetBaseValueForType(StatType::kAttackSpeed);
        const auto reduced_attack_speed =
            base_attack_speed -
            ((base_attack_speed * dot_config.debuff_percentage / 100_fp) * FixedPoint::FromInt(stack));
        auto red_live_stats = world->GetLiveStats(red_entity_id);
        ASSERT_EQ(red_live_stats.Get(StatType::kAttackSpeed), reduced_attack_speed) << "stack = " << stack;

        // Should be Frosted
        ASSERT_TRUE(EntityHelper::IsFrosted(*red_entity));

        // Should not be Frosted
        ASSERT_FALSE(EntityHelper::IsFrosted(*blue_entity));

        // Should able to move, attack, gain energy
        ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
        ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
        ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
        ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
    }

    // Should be Frosted
    ASSERT_TRUE(EntityHelper::IsFrosted(*red_entity));

    // Should not be Frosted
    ASSERT_FALSE(EntityHelper::IsFrosted(*blue_entity));

    // At max stacks activate the max_stacks
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, frost_effect_data, frost_effect_state);

    // Need to be Frozen
    EXPECT_TRUE(EntityHelper::IsFrozen(*red_entity));

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // Last TimeStep
    world->TimeStep();
    // world->TimeStep();

    // Should have fired a final pure damage effect
    // EXPECT_EQ(events_remove_effect.size(), 3);
    ASSERT_EQ(events_apply_effect.size(), 0);

    // Should not be Frosted
    EXPECT_TRUE(dot_config.cleanse_on_max_stacks);
    EXPECT_FALSE(EntityHelper::IsFrosted(*red_entity));
    EXPECT_FALSE(EntityHelper::IsFrosted(*blue_entity));

    // Check we don't have anymore conditions after cleanse
    const auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    {
        EffectTypeID condition_type_id;
        condition_type_id.type = EffectType::kCondition;
        condition_type_id.condition_type = EffectConditionType::kFrost;
        std::map<std::string_view, std::vector<AttachedEffectStatePtr>> same_abilities_map;
        std::vector<AttachedEffectStatePtr> different_abilities;
        const size_t total_count_effects = red_attached_effects_component.GetAllRootEffectsOfTypeIDPerAbilityName(
            condition_type_id,
            &same_abilities_map,
            &different_abilities);
        EXPECT_EQ(total_count_effects, 0) << "Cleanse should have removed all conditions";
    }

    // Need to be Frozen
    ASSERT_TRUE(EntityHelper::IsFrozen(*red_entity));

    // Should not be able to move, attack, gain energy
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyConditionBurnImmuneCancel)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    StatsHelper::SetDefaults(red_stats_component.GetMutableTemplateStats());

    // set sane values
    red_stats_component.SetCurrentHealth(400_fp);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kOmegaPowerPercentage, 100_fp);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));

    // Add an effect, then Wrap it in an attached effect
    auto effect_data = EffectData::CreateCondition(EffectConditionType::kBurn, kDefaultAttachedEffectsFrequencyMs);
    effect_data.SetExpression(EffectExpression::FromValue(1000_fp));

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Should expire after 40 TimeSteps
    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kBurn);
    const int num_steps = Time::MsToTimeSteps(dot_config.duration_ms);
    for (int i = 0; i < num_steps / 2; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Should be burned
        ASSERT_TRUE(EntityHelper::IsBurned(*red_entity)) << " i = " << i;

        // Blue should not be burned
        ASSERT_FALSE(EntityHelper::IsBurned(*blue_entity)) << " i = " << i;
    }

    const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
    EffectState immune_effect_state{};
    immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);

    // Should be immune
    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    // Should still be burned because Immune ignores Metered Combat Stat Change Over Time
    ASSERT_TRUE(EntityHelper::IsBurned(*red_entity));

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    // TimeStep again
    world->TimeStep();

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 352_fp)
        << "Burn effect did not have the desired health effect on the entity health";

    // TimeStep again
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // TimeStep again so immune expires
    world->TimeStep();

    // 17 timesteps since there was 20 in the first loop, 3 steps for immune, and now 17 remain for
    // full burn duration
    for (int i = 0; i < num_steps / 2 - 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Should be burned
        ASSERT_TRUE(EntityHelper::IsBurned(*red_entity)) << " i = " << i;

        // Blue should not be burned
        ASSERT_FALSE(EntityHelper::IsBurned(*blue_entity)) << " i = " << i;
    }

    world->TimeStep();

    // Should have fired a final removal effect for the attached events and condition
    ASSERT_EQ(events_apply_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.size(), 3);
    {
        const auto& sent_effect1 = events_remove_effect.at(1).data;
        const auto& sent_effect2 = events_remove_effect.at(2).data;

        // Should fire another 2 attached effects on the first step
        ASSERT_EQ(sent_effect1.type_id.type, EffectType::kCondition);
        ASSERT_EQ(sent_effect2.type_id.type, EffectType::kDamageOverTime);

        ASSERT_EQ(sent_effect1.type_id.condition_type, EffectConditionType::kBurn);
        ASSERT_EQ(sent_effect2.type_id.damage_type, EffectDamageType::kEnergy);

        // Check duration
        ASSERT_EQ(sent_effect1.lifetime.duration_time_ms, dot_config.duration_ms);
        ASSERT_EQ(sent_effect2.lifetime.duration_time_ms, dot_config.duration_ms);
    }

    // Should not be burned
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsBurned(*red_entity));
}
TEST_F(AttachedEffectsSystemTest, ApplyConditionWoundImmuneCancel)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    StatsHelper::SetDefaults(red_stats_component.GetMutableTemplateStats());

    // set sane values
    static constexpr auto scale = 1000_fp;
    red_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 10000_fp * scale);
    red_stats_component.SetCurrentHealth(10000_fp * scale);
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCritAmplificationPercentage, 100_fp);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsWounded(*red_entity));

    // Add an effect, then Wrap it in an attached effect
    auto effect_data = EffectData::CreateCondition(EffectConditionType::kWound, kDefaultAttachedEffectsFrequencyMs);
    effect_data.SetExpression(EffectExpression::FromValue(1000_fp * scale));

    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);

    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Should expire after 80 TimeSteps
    const auto& dot_config = world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kWound);
    int num_time_steps = Time::MsToTimeSteps(dot_config.duration_ms);
    for (int i = 0; i < num_time_steps / 2; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // time step the world
        world->TimeStep();

        // Should be wounded
        ASSERT_TRUE(EntityHelper::IsWounded(*red_entity)) << " i = " << i;

        // Blue should not be wounded
        ASSERT_FALSE(EntityHelper::IsWounded(*blue_entity)) << " i = " << i;
    }

    const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
    EffectState immune_effect_state{};
    immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);

    // Should be immune
    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    // Should still be wounded because Immune does not ignore Metered Combat Stat Change Over Time
    ASSERT_TRUE(EntityHelper::IsWounded(*red_entity));

    // Clear before each call
    events_apply_effect.clear();
    events_remove_effect.clear();

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 9950000_fp)
        << "Wound effect did not have the desired health effect on the entity health";

    // time step again
    world->TimeStep();

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 9950000_fp)
        << "Wound effect did not affect the entity health during Immune";

    // time step again
    events_apply_effect.clear();
    events_remove_effect.clear();
    world->TimeStep();

    // time step again so immune expires
    world->TimeStep();

    // 17 TimeSteps since there was 20 in the first loop, 3 TimeSteps for immune, and now 17 remain
    // for full wound duration
    for (int i = 0; i < num_time_steps / 2 - 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // time step the world
        world->TimeStep();

        // Should be wounded
        ASSERT_TRUE(EntityHelper::IsWounded(*red_entity)) << " i = " << i;

        // Blue should not be wounded
        ASSERT_FALSE(EntityHelper::IsWounded(*blue_entity)) << " i = " << i;
    }

    world->TimeStep();

    // Should have fired a final removal effect for the attached events and condition
    ASSERT_EQ(events_apply_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.size(), 3);
    {
        const auto& sent_effect1 = events_remove_effect.at(1).data;
        const auto& sent_effect2 = events_remove_effect.at(2).data;

        // Should fire another 2 attached effects on the first time step
        ASSERT_EQ(sent_effect1.type_id.type, EffectType::kCondition);
        ASSERT_EQ(sent_effect2.type_id.type, EffectType::kDamageOverTime);

        ASSERT_EQ(sent_effect1.type_id.condition_type, EffectConditionType::kWound);
        ASSERT_EQ(sent_effect2.type_id.damage_type, EffectDamageType::kPhysical);

        // Check duration
        ASSERT_EQ(sent_effect1.lifetime.duration_time_ms, dot_config.duration_ms);
        ASSERT_EQ(sent_effect2.lifetime.duration_time_ms, dot_config.duration_ms);
    }

    // Should not be wounded
    ASSERT_FALSE(EntityHelper::IsWounded(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsWounded(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyMultipleDOTEffects)
{
    // Get the relevant components
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.SetCurrentHealth(100_fp);

    const auto poison_effect_data =
        EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);
    EffectState poison_effect_state{};
    poison_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), poison_effect_data, poison_effect_state);

    const auto poison_effect_data2 =
        EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);
    EffectState poison_effect_state2{};
    poison_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), poison_effect_data2, poison_effect_state2);

    const auto wound_effect_data =
        EffectData::CreateCondition(EffectConditionType::kWound, kDefaultAttachedEffectsFrequencyMs);
    EffectState wound_effect_state{};
    wound_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), wound_effect_data, wound_effect_state);

    const auto wound_effect_data2 =
        EffectData::CreateCondition(EffectConditionType::kWound, kDefaultAttachedEffectsFrequencyMs);
    EffectState wound_effect_state2{};
    wound_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), wound_effect_data2, wound_effect_state2);

    ASSERT_TRUE(red_attached_effects_component.HasCondition(EffectConditionType::kPoison));
    ASSERT_TRUE(red_attached_effects_component.HasCondition(EffectConditionType::kWound));

    // 40 TimeSteps so wound expires
    const int duration_time_steps =
        Time::MsToTimeSteps(world->GetWorldEffectsConfig().GetConditionType(EffectConditionType::kWound).duration_ms);
    for (int i = 0; i < duration_time_steps; i++)
    {
        //// TimeStep the world (3 TimeSteps)
        world->TimeStep();

        if (!events_apply_effect.empty())
        {
            const auto& poison_sent_effect = events_apply_effect.at(0).data;
            ASSERT_EQ(poison_sent_effect.type_id.type, EffectType::kInstantDamage);
            ASSERT_EQ(poison_sent_effect.type_id.damage_type, EffectDamageType::kPure)
                << "Poison period events is not poison."
                << " i = " << i;
            ASSERT_EQ(events_apply_effect.at(0).state.sender_stats.live.Get(StatType::kAttackPhysicalDamage), 0_fp)
                << " i = " << i;

            // // 2 poison and wound effects sent so each stat reduced by 50% on each application
            // red_live_stats = world->GetLiveStats(red_entity->GetID());
            // EXPECT_EQ(red_live_stats.Get(StatType::kCritAmplificationPercentage), 100);
            // EXPECT_EQ(red_live_stats.Get(StatType::kHealthGainEfficiencyPercentage), 50);

            // Clear before each call
            events_apply_effect.clear();
        }
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 6);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 8);
    ASSERT_FALSE(red_attached_effects_component.HasCondition(EffectConditionType::kWound));
    events_remove_effect.clear();

    // Finish remaining poison TimeSteps
    for (int i = 0; i < 40; i++)
    {
        world->TimeStep();
    }

    ASSERT_EQ(events_remove_effect.size(), 0);
    ASSERT_FALSE(red_attached_effects_component.HasCondition(EffectConditionType::kPoison));
}

}  // namespace simulation
