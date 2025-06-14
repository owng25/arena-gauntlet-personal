#include "systems/effect_system.h"

#include "components/abilities_component.h"
#include "components/attached_effects_component.h"
#include "components/attached_entity_component.h"
#include "components/displacement_component.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "components/shield_component.h"
#include "components/stats_component.h"
#include "data/aura_data.hpp"
#include "data/constants.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"
#include "utility/attached_effects_helper.h"
#include "utility/effect_package_helper.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/grid_helper.h"
#include "utility/hex_grid_position.h"
#include "utility/math.h"
#include "utility/stats_helper.h"

namespace simulation
{
void EffectSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kTryToApplyEffect>(this, &Self::OnTryToApplyEffect);
    world_->SubscribeMethodToEvent<EventType::kOnAttachedEffectRemoved>(this, &Self::OnAttachedEffectRemoved);
}

void EffectSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }
}

void EffectSystem::OnTryToApplyEffect(const event_data::Effect& event_data)
{
    // If receiver id is ivalid it means we are sending effect to the ground
    // when direct skill has lost it's target and cancel/retarget skill is too late.
    // So we just sending these events but don't have to do anythhing in simulation to
    // handle them.
    if (event_data.receiver_id == kInvalidEntityID && event_data.receiver_position != kInvalidHexHexGridPosition)
    {
        return;
    }

    ApplyEffect(event_data.sender_id, event_data.receiver_id, event_data.data, event_data.state);
}

void EffectSystem::OnAttachedEffectRemoved(const event_data::Effect& event_data)
{
    const EffectData& data = event_data.data;

    // This effect is going to be removed
    switch (data.type_id.type)
    {
    case EffectType::kDisplacement:
    {
        event_data::Displacement emit_event;
        emit_event.sender_id = event_data.sender_id;
        emit_event.combat_unit_sender_id = world_->GetCombatUnitParentID(emit_event.sender_id);
        emit_event.receiver_id = event_data.receiver_id;
        emit_event.displacement_type = data.type_id.displacement_type;
        emit_event.duration_time_ms = data.lifetime.duration_time_ms;
        emit_event.distance_sub_units = 0;

        world_->EmitEvent<EventType::kDisplacementStopped>(emit_event);

        if (world_->HasEntity(event_data.receiver_id))
        {
            auto& receiver = world_->GetByID(event_data.receiver_id);
            receiver.Get<DisplacementComponent>().ClearAttachedEffectState();
        }

        break;
    }
    case EffectType::kBlink:
    {
        // Clear reserved position when blink finished
        if (world_->HasEntity(event_data.sender_id))
        {
            const Entity& sender_entity = world_->GetByID(event_data.sender_id);
            if (sender_entity.Has<PositionComponent>())
            {
                auto& position_component = sender_entity.Get<PositionComponent>();
                if (position_component.HasReservedPosition())
                {
                    position_component.ClearReservedPosition();
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

void EffectSystem::ApplyEffect(
    const EntityID sender_id,
    const EntityID receiver_id,
    const EffectData& data,
    const EffectState& state)
{
    if (!world_->HasEntity(sender_id))
    {
        LogDebug("| ApplyEffect sender_id = {} does not exist anymore. ABORTING", sender_id);
        return;
    }
    if (!world_->HasEntity(receiver_id))
    {
        LogDebug("| ApplyEffect receiver_id = {} does not exist anymore. ABORTING", receiver_id);
        return;
    }

    auto& sender_entity = world_->GetByID(sender_id);
    auto& receiver_entity = world_->GetByID(receiver_id);

    // Can't apply effect on a target without stats
    if (!receiver_entity.Has<StatsComponent>())
    {
        return;
    }

    // Sender
    const bool is_sender_a_combat_unit = EntityHelper::IsACombatUnit(sender_entity);

    // Receiver
    const EffectPackageAttributes& effect_package_attributes = state.effect_package_attributes;
    auto& receiver_stats_component = receiver_entity.Get<StatsComponent>();
    const FixedPoint receiver_health = receiver_stats_component.GetCurrentHealth();

    // Current stats
    FullStatsData sender_stats = state.sender_stats;

    // By default death reason is damage
    DeathReasonType death_reason = DeathReasonType::kDamage;

    // Copy context form effect state, can be modified later
    SourceContextData source_context = state.source_context;

    // Find out the true sender combat stats.
    // - sender is a combat unit - just the sender_live_stats
    // - sender is not a combat unit - we get the last known combat unit parent current stats before spawning the entity
    if (!is_sender_a_combat_unit && !EntityHelper::IsASynergy(sender_entity) &&
        !EntityHelper::IsADroneAugment(sender_entity))
    {
        const auto& sender_stats_component = sender_entity.Get<StatsComponent>();
        sender_stats.live = sender_stats_component.GetCombatUnitParentLiveStats();
    }

    // Check if bad effect or good effect
    // If it received this effect from an Ally it will ignore any of the 'bad' effects.
    // If it received it from an Enemy it will ignore all the 'good' effects.

    // Evaluate the effect expression to get the final value
    ExpressionStatsSource effect_stats_source;
    effect_stats_source.Set(
        ExpressionDataSourceType::kSender,
        sender_stats,
        &world_->GetAllSynergiesOfEntityID(sender_id));
    effect_stats_source.Set(ExpressionDataSourceType::kReceiver, world_->GetEntityDataForExpression(receiver_id));
    if (data.GetRequiredDataSourceTypes().Contains(ExpressionDataSourceType::kSenderFocus))
    {
        effect_stats_source.Set(ExpressionDataSourceType::kSenderFocus, state.sender_focus_stats, nullptr);
    }
    const ExpressionEvaluationContext expression_context(world_, sender_id, receiver_id);
    const FixedPoint effect_value = data.GetExpression().Evaluate(expression_context, effect_stats_source);

    // General debug log
    LogDebug(sender_id, "| ApplyEffect - receiver = {}", receiver_id);
    LogDebug(sender_id, "- data: {}", data);
    LogDebug(sender_id, "- expression = {} | value = {}", data.GetExpression(), effect_value);
    LogDebug(sender_id, "- state: {}", state);
    LogDebug(
        sender_id,
        "- sender_stats: crit_amplification = {}, attack_physical_damage = {}, "
        "attack_energy_damage = {}, attack_pure_damage = {}, physical_piercing = {}, "
        "energy_piercing = {}",
        sender_stats.live.Get(StatType::kCritAmplificationPercentage),
        sender_stats.live.Get(StatType::kAttackPhysicalDamage),
        sender_stats.live.Get(StatType::kAttackEnergyDamage),
        sender_stats.live.Get(StatType::kAttackPureDamage),
        sender_stats.live.Get(StatType::kPhysicalPiercingPercentage),
        sender_stats.live.Get(StatType::kEnergyPiercingPercentage));

    if (!receiver_entity.IsActive())
    {
        LogDebug(sender_id, "- IGNORED receiver = {} is not active anymore", receiver_id);
        return;
    }

    // Execute condition check if a condition should be considered
    if (!world_->DoesPassConditions(receiver_entity, data.required_conditions))
    {
        LogDebug(sender_id, "- IGNORED as did not pass conditions check for receiver = {}", receiver_id);
        return;
    }

    // Ignore bad effects from allies and good effects from enemies
    if (IsDeniedByAgency(sender_entity, receiver_entity, data, state))
    {
        return;
    }

    // Send Correct events
    world_->BuildAndEmitEvent<EventType::kOnEffectApplied>(sender_id, receiver_id, data, state);

    // Clear receivers that did purest damage on the next time step
    if (world_->GetTimeStepCount() != time_step_receivers_with_purest_damage_)
    {
        time_step_receivers_with_purest_damage_ = world_->GetTimeStepCount();
        receivers_with_purest_damage_.clear();
    }

    // Figure out the effect type
    const EffectDataAttributes& attributes = data.attributes;
    const AttachedEffectsHelper& attached_effects_helper = world_->GetAttachedEffectsHelper();

    ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
    switch (data.type_id.type)
    {
    case EffectType::kStatOverride:
        ApplyStatOverrideEffect(receiver_entity, data.type_id.stat_type, data.GetExpression().base_value.value);
        break;

    case EffectType::kInstantDamage:
    {
        ApplyDamageEffect(
            sender_entity,
            receiver_entity,
            data,
            state,
            effect_value,
            effect_stats_source.Get(ExpressionDataSourceType::kReceiver).stats,
            death_reason,
            source_context);
        break;
    }
    case EffectType::kInstantShieldBurn:
    {
        ApplyShieldBurnEffect(sender_entity, receiver_entity, effect_value);
        break;
    }
    case EffectType::kDamageOverTime:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // Due to Damage Bonus and Amplification we need to adjsut the package before it is attached
        // Damage Bonus https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791738/EffectPackage.DamageBonus
        // Damage Amplification
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247530406/EffectPackage.DamageAmplification

        // Replace the expression with our new value
        EffectData effect_data = data;
        effect_data.SetExpression(EffectExpression::FromValue(ApplyBonusDamageAndAmplify(
            sender_id,
            receiver_id,
            effect_data,
            state,
            effect_package_attributes,
            effect_value)));

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, effect_data, state);
        break;
    }
    case EffectType::kCondition:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        break;
    }
    case EffectType::kInstantHeal:
    {
        // If the receiver has the purest damage effect.
        if (receivers_with_purest_damage_.count(receiver_id) == 1)
        {
            break;
        }

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/248021110/EffectPackage.HealBonus
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/248020993/EffectPackage.HealAmplification
        const FixedPoint total_value =
            effect_value + effect_package_attributes.heal_bonus.Evaluate(expression_context, effect_stats_source);
        const FixedPoint total_percentage =
            kMaxPercentageFP +
            effect_package_attributes.heal_amplification.Evaluate(expression_context, effect_stats_source);
        const FixedPoint final_effect_value = total_percentage.AsPercentageOf(total_value);
        LogDebug(sender_id, "- kInstantHeal - final_effect_value = {}", final_effect_value);

        HealthGainType heal_gain_type = HealthGainType::kNone;

        switch (data.type_id.heal_type)
        {
        case EffectHealType::kNormal:
        {
            heal_gain_type = HealthGainType::kInstantHeal;
            break;
        }
        case EffectHealType::kPure:
        {
            heal_gain_type = HealthGainType::kInstantPureHeal;
            break;
        }
        default:
            break;
        }

        event_data::ApplyHealthGain event_data;
        event_data.caused_by_id = sender_id;
        event_data.receiver_id = receiver_id;
        event_data.health_gain_type = heal_gain_type;
        event_data.value = final_effect_value;
        event_data.excess_heal_to_shield = attributes.excess_heal_to_shield;
        event_data.missing_health_percentage_to_health = attributes.missing_health_percentage_to_health;
        event_data.max_health_percentage_to_health = attributes.max_health_percentage_to_health;
        event_data.source_context = state.source_context;

        world_->EmitEvent<EventType::kApplyHealthGain>(event_data);
        break;
    }
    case EffectType::kHealOverTime:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/248021110/EffectPackage.HealBonus
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/248020993/EffectPackage.HealAmplification
        const FixedPoint total_value =
            effect_value + effect_package_attributes.heal_bonus.Evaluate(expression_context, effect_stats_source);
        const FixedPoint total_percentage =
            kMaxPercentageFP +
            effect_package_attributes.heal_amplification.Evaluate(expression_context, effect_stats_source);
        const FixedPoint final_effect_value = total_percentage.AsPercentageOf(total_value);
        LogDebug(sender_id, "- kHealOverTime - final_effect_value = {}", final_effect_value);

        // Replace the expression with our new value
        EffectData effect_data = data;
        effect_data.SetExpression(EffectExpression::FromValue(final_effect_value));

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, effect_data, state);
        break;
    }
    case EffectType::kInstantEnergyGain:
    {
        const FixedPoint current_energy = receiver_stats_component.GetCurrentEnergy();

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791857/EffectPackage.EnergyGainBonus
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791714/EffectPackage.EnergyGainAmplification
        const FixedPoint total_value =
            effect_value +
            effect_package_attributes.energy_gain_bonus.Evaluate(expression_context, effect_stats_source);
        const FixedPoint total_percentage =
            kMaxPercentageFP +
            effect_package_attributes.energy_gain_amplification.Evaluate(expression_context, effect_stats_source);
        const FixedPoint final_effect_value = total_percentage.AsPercentageOf(total_value);
        LogDebug(sender_id, "- kInstantEnergyGain - final_effect_value = {}", final_effect_value);

        receiver_stats_component.SetCurrentEnergy(current_energy + final_effect_value);

        // Send event
        const FixedPoint delta = final_effect_value - effect_value;
        world_->BuildAndEmitEvent<EventType::kEnergyChanged>(sender_id, receiver_id, delta);
        break;
    }
    case EffectType::kEnergyGainOverTime:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791857/EffectPackage.EnergyGainBonus
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791714/EffectPackage.EnergyGainAmplification
        const FixedPoint total_value =
            effect_value +
            effect_package_attributes.energy_gain_bonus.Evaluate(expression_context, effect_stats_source);
        const FixedPoint total_percentage =
            kMaxPercentageFP +
            effect_package_attributes.energy_gain_amplification.Evaluate(expression_context, effect_stats_source);
        const FixedPoint final_effect_value = total_percentage.AsPercentageOf(total_value);
        LogDebug(sender_id, "- kEnergyGainOverTime - final_effect_value = {}", final_effect_value);

        // Replace the expression with our new value
        EffectData effect_data = data;
        effect_data.SetExpression(EffectExpression::FromValue(final_effect_value));

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, effect_data, state);
        break;
    }
    case EffectType::kInstantEnergyBurn:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/248021158/EffectPackage.EnergyBurnBonus
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791691/EffectPackage.EnergyBurnAmplification
        const FixedPoint total_value =
            effect_value +
            effect_package_attributes.energy_burn_bonus.Evaluate(expression_context, effect_stats_source);
        const FixedPoint total_percentage =
            kMaxPercentageFP +
            effect_package_attributes.energy_burn_amplification.Evaluate(expression_context, effect_stats_source);
        const FixedPoint final_effect_value = total_percentage.AsPercentageOf(total_value);
        LogDebug(sender_id, "- kInstantEnergyBurn - final_effect_value = {}", final_effect_value);

        const FixedPoint current_energy = receiver_stats_component.GetCurrentEnergy();
        receiver_stats_component.SetCurrentEnergy(current_energy - final_effect_value);

        // Send event
        const FixedPoint delta = -final_effect_value;
        world_->BuildAndEmitEvent<EventType::kEnergyChanged>(sender_id, receiver_id, delta);
        break;
    }
    case EffectType::kEnergyBurnOverTime:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791691/EffectPackage.EnergyBurnAmplification
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/248021158/EffectPackage.EnergyBurnBonus
        const FixedPoint total_value =
            effect_value +
            effect_package_attributes.energy_burn_bonus.Evaluate(expression_context, effect_stats_source);
        const FixedPoint total_percentage =
            kMaxPercentageFP +
            effect_package_attributes.energy_burn_amplification.Evaluate(expression_context, effect_stats_source);
        const FixedPoint final_effect_value = total_percentage.AsPercentageOf(total_value);
        LogDebug(sender_id, "- kEnergyBurnOverTime - final_effect_value = {}", final_effect_value);

        // Replace the expression with our new value
        EffectData effect_data = data;
        effect_data.SetExpression(EffectExpression::FromValue(final_effect_value));

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, effect_data, state);
        break;
    }
    case EffectType::kHyperGainOverTime:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        break;
    }
    case EffectType::kInstantHyperGain:
    {
        const FixedPoint previous_hyper = receiver_stats_component.GetCurrentHyper();
        receiver_stats_component.SetCurrentSubHyper(receiver_stats_component.GetCurrentSubHyper() + effect_value);

        // Send event
        const FixedPoint delta = receiver_stats_component.GetCurrentHyper() - previous_hyper;
        if (delta != 0_fp)
        {
            world_->BuildAndEmitEvent<EventType::kHyperChanged>(sender_id, receiver_id, delta);
        }
        break;
    }
    case EffectType::kInstantHyperBurn:
    {
        const FixedPoint previous_hyper = receiver_stats_component.GetCurrentHyper();
        receiver_stats_component.SetCurrentSubHyper(receiver_stats_component.GetCurrentSubHyper() - effect_value);

        // Send event
        const FixedPoint delta = receiver_stats_component.GetCurrentHyper() - previous_hyper;
        if (delta != 0_fp)
        {
            world_->BuildAndEmitEvent<EventType::kHyperChanged>(sender_id, receiver_id, delta);
        }
        break;
    }
    case EffectType::kHyperBurnOverTime:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        break;
    }
    case EffectType::kDebuff:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        LogDebug(sender_id, "- Added Debuff attached effect to receiver = {}", receiver_id);
        break;
    }
    case EffectType::kBuff:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        LogDebug(sender_id, "- Added Buff attached effect to receiver = {}", receiver_id);
        break;
    }
    case EffectType::kSpawnShield:
    {
        // NOTE: Agency is not checked for this type

        // Check for shield component
        if (!receiver_entity.Has<AttachedEntityComponent>())
        {
            LogErr(
                sender_id,
                "- DENIED because receiver_entity = {} does not have AttachedEntityComponent",
                receiver_entity.GetID());
            break;
        }

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791666/EffectPackage.ShieldAmplification
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/248021135/EffectPackage.ShieldBonus
        const FixedPoint total_value =
            effect_value + effect_package_attributes.shield_bonus.Evaluate(expression_context, effect_stats_source);
        const FixedPoint total_percentage =
            world_->GetLiveStats(receiver_entity).Get(StatType::kShieldGainEfficiencyPercentage) +
            effect_package_attributes.shield_amplification.Evaluate(expression_context, effect_stats_source);
        const FixedPoint final_effect_value = total_percentage.AsPercentageOf(total_value);
        LogDebug(sender_id, "- kSpawnShield - final_effect_value = {}", final_effect_value);

        // Create new shield
        ShieldData new_shield_data;
        new_shield_data.sender_id = sender_id;
        new_shield_data.receiver_id = receiver_id;
        new_shield_data.value = final_effect_value;
        new_shield_data.duration_ms = data.lifetime.duration_time_ms;
        new_shield_data.event_skills = data.event_skills;
        new_shield_data.effects = data.attached_effects;
        new_shield_data.source_context = state.source_context;
        EntityFactory::SpawnShieldAndAttach(*world_, sender_entity.GetTeam(), new_shield_data);

        LogDebug(
            sender_id,
            "- target [health = {}, shield = {}, energy = {}]",
            receiver_stats_component.GetCurrentHealth(),
            world_->GetShieldTotal(receiver_entity),
            receiver_stats_component.GetCurrentEnergy());
        break;
    }
    case EffectType::kAura:
    {
        for (const EffectData& effect : data.attached_effects)
        {
            // Spawn an aura for each attached effect as they may have different lifetimes
            EntityFactory::SpawnAuraAndAttach(
                *world_,
                AuraData{
                    .effect = effect,
                    .aura_sender_id = sender_id,
                    // effects will be applied on behalf of the entity that applied the aura
                    .effect_sender_id = sender_id,
                    .receiver_id = receiver_id,
                    .source_context = state.source_context,
                });
        }
        break;
    }
    case EffectType::kSpawnMark:
    {
        // Check for mark component
        if (!receiver_entity.Has<AttachedEntityComponent>())
        {
            LogDebug(receiver_id, "- DENIED because entity does not have AttachedEntityComponent");
            break;
        }

        // Create new mark
        MarkData new_mark_data;
        new_mark_data.sender_id = sender_id;
        new_mark_data.receiver_id = receiver_id;
        new_mark_data.duration_ms = data.lifetime.duration_time_ms;
        new_mark_data.should_destroy_on_sender_death = data.should_destroy_attached_entity_on_sender_death;
        new_mark_data.abilities_data = data.attached_abilities;
        new_mark_data.source_context = state.source_context;

        // Spawn and attach mark to sender entity
        EntityFactory::SpawnMarkAndAttach(*world_, sender_entity.GetTeam(), new_mark_data);

        LogDebug(sender_id, "- Mark spawned and attached");
        break;
    }
    case EffectType::kDisplacement:
    {
        // Immune, don't apply effect
        if (EntityHelper::IsImmuneToDetrimentalEffect(receiver_entity, data.type_id))
        {
            break;
        }

        // Can't attach as the target does not have these components
        if (!(receiver_entity.Has<DisplacementComponent>() && receiver_entity.Has<PositionComponent>()))
        {
            LogErr(receiver_id, "- Does not have DisplacementComponent and PositionComponent");
            assert(false);
            break;
        }

        // Sanity check for duration
        if (data.lifetime.duration_time_ms < 0)
        {
            LogErr(receiver_id, "- Displacement cannot have negative duration");
            assert(false);
            break;
        }
        ApplyDisplacementEffect(sender_entity, receiver_entity, data, state);
        break;
    }
    case EffectType::kEmpower:
    case EffectType::kDisempower:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        break;
    }

    case EffectType::kPositiveState:
    case EffectType::kNegativeState:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        break;
    }
    case EffectType::kPlaneChange:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // Add effect
        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        LogDebug(sender_id, "- Added Plane Change attached effect to receiver = {}", receiver_id);
        break;
    }
    case EffectType::kExecute:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, data, state);
        LogDebug(sender_id, "- Added Execute attached effect to receiver = {}", receiver_id);
        break;
    }
    case EffectType::kBlink:
    {
        if (!receiver_entity.Has<AttachedEffectsComponent>())
        {
            break;
        }

        // Add attached effect to sender
        attached_effects_helper.AddAttachedEffect(sender_entity, sender_id, data, state);
        break;
    }
    case EffectType::kCleanse:
    {
        auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();

        std::vector<AttachedEffectStatePtr> effects_to_remove;
        if (attributes.cleanse_debuffs)
        {
            VectorHelper::AppendVector(effects_to_remove, attached_effects_component.GetAllCleansableDebuffs());
        }
        if (attributes.cleanse_conditions)
        {
            VectorHelper::AppendVector(effects_to_remove, attached_effects_component.GetAllCleansableConditions());
        }
        if (attributes.cleanse_negative_states)
        {
            VectorHelper::AppendVector(effects_to_remove, attached_effects_component.GetAllCleansableNegativeStates());
        }
        if (attributes.cleanse_dots)
        {
            VectorHelper::AppendVector(effects_to_remove, attached_effects_component.GetAllCleansableDOTs());
        }
        if (attributes.cleanse_bots)
        {
            VectorHelper::AppendVector(
                effects_to_remove,
                attached_effects_component.GetAllCleansableEnergyBurnOverTimes());
        }

        // Remove
        for (const auto& attached_effect : effects_to_remove)
        {
            attached_effects_helper.RemoveAttachedEffect(receiver_entity, attached_effect);
        }
        break;
    }
    default:
        assert(false);
        break;
    }

    // Stats modified events from the apply effects above
    const FixedPoint receiver_new_health = receiver_stats_component.GetCurrentHealth();
    if (receiver_new_health != receiver_health)
    {
        LogDebug(
            sender_id,
            "- kHealthChanged receiver_new_health ({}) != receiver_health ({})",
            receiver_new_health,
            receiver_health);
        const FixedPoint delta = receiver_new_health - receiver_health;
        world_->BuildAndEmitEvent<EventType::kHealthChanged>(sender_id, receiver_id, delta);
    }

    // Check indomitable for everything except purest
    if (data.type_id.damage_type != EffectDamageType::kPurest && receiver_new_health <= 0_fp &&
        EntityHelper::IsIndomitable(receiver_entity))
    {
        receiver_stats_component.SetCurrentHealth(1_fp);
        world_->BuildAndEmitEvent<EventType::kHealthChanged>(sender_id, receiver_id, 1_fp);
        LogDebug("| Target Health <= 0 but the target is indomitable, setting the health to 1", receiver_id);
        return;
    }

    // Died? aka fainted
    if (receiver_new_health <= 0_fp)
    {
        LogDebug(sender_id, " - VANQUISHED receiver = {}, death_reason = {}", receiver_id, death_reason);

        // Deactivate the target
        if (receiver_entity.IsActive())
        {
            EntityHelper::Kill(receiver_entity);

            event_data::Fainted event_data;
            event_data.entity_id = receiver_id;
            event_data.vanquisher_id = sender_id;
            event_data.source_context = source_context;
            event_data.death_reason = death_reason;
            world_->EmitEvent<EventType::kFainted>(event_data);
        }
    }
}

FixedPoint EffectSystem::ApplyShieldDamage(
    const Entity& sender_entity,
    const std::vector<AttachedEntity>& attached_shields,
    const FixedPoint pm_damage)
{
    FixedPoint remaining_damage = pm_damage;

    for (auto shield_iterator = attached_shields.rbegin(); shield_iterator != attached_shields.rend();
         ++shield_iterator)
    {
        const AttachedEntity& attached_shield = *shield_iterator;

        // Only shields
        if (attached_shield.type != AttachedEntityType::kShield)
        {
            continue;
        }

        if (!world_->HasEntity(attached_shield.id))
        {
            LogWarn(
                sender_entity.GetID(),
                "| ApplyShieldDamage - attached entity id = {} does not exist",
                attached_shield.id);
            continue;
        }

        const auto& shield = world_->GetByID(attached_shield.id);
        auto& shield_state_component = shield.Get<ShieldComponent>();

        const FixedPoint receiver_old_shield = shield_state_component.GetValue();
        FixedPoint receiver_new_shield = receiver_old_shield;

        // Once remaining_damage reaches 0, stop applying damage
        if (remaining_damage > 0_fp)
        {
            receiver_new_shield -= remaining_damage;
            if (receiver_new_shield < 0_fp)
            {
                // Shield destroyed
                remaining_damage = -receiver_new_shield;
                receiver_new_shield = 0_fp;
            }
            else
            {
                // Damage exhausted
                remaining_damage = 0_fp;
            }
            shield_state_component.SetValue(receiver_new_shield);
        }

        // Shield hit event
        world_->BuildAndEmitEvent<EventType::kShieldWasHit>(
            attached_shield.id,
            shield_state_component.GetSenderID(),
            shield_state_component.GetReceiverID(),
            sender_entity.GetID());
    }

    return remaining_damage;
}

void EffectSystem::ApplyStatOverrideEffect(const Entity& entity, const StatType stat_type, const FixedPoint value)
{
    auto& stats_component = entity.Get<StatsComponent>();
    stats_component.OverrideBaseStat(stat_type, value);
    world_->ClearEntityBaseStatCache(entity.GetID());
}

void EffectSystem::ApplyShieldBurnEffect(
    const Entity& sender_entity,
    const Entity& receiver_entity,
    const FixedPoint effect_value)
{
    if (receiver_entity.Has<AttachedEntityComponent>())
    {
        auto& receiver_shield_component = receiver_entity.Get<AttachedEntityComponent>();
        const auto receiver_shields = receiver_shield_component.GetAttachedEntities();  // explicit copy
        ApplyShieldDamage(sender_entity, receiver_shields, effect_value);
    }
}

void EffectSystem::ApplyDamageEffect(
    const Entity& sender_entity,
    const Entity& receiver_entity,
    const EffectData& data,
    const EffectState& state,
    const FixedPoint effect_value,
    const FullStatsData& receiver_stats,
    DeathReasonType& out_death_reason,
    SourceContextData& out_source_context)
{
    const EntityID sender_id = sender_entity.GetID();
    const FullStatsData& sender_stats = state.sender_stats;
    const bool is_synergy = EntityHelper::IsASynergy(sender_entity);
    const EntityID combat_unit_sender_id =
        !is_synergy ? world_->GetCombatUnitParentID(sender_entity) : kInvalidEntityID;

    const EntityID receiver_id = receiver_entity.GetID();
    auto& receiver_stats_component = receiver_entity.Get<StatsComponent>();
    const EffectPackageAttributes& effect_package_attributes = state.effect_package_attributes;

    const FixedPoint receiver_health = receiver_stats_component.GetCurrentHealth();
    const FixedPoint receiver_energy = receiver_stats_component.GetCurrentEnergy();

    // By default return source context form effect state, can be modified later
    out_source_context = state.source_context;

    // Shields
    std::vector<AttachedEntity> receiver_shields;
    if (receiver_entity.Has<AttachedEntityComponent>())
    {
        auto& receiver_shield_component = receiver_entity.Get<AttachedEntityComponent>();
        receiver_shields = receiver_shield_component.GetAttachedEntities();
    }

    // Effect value + bonus
    FixedPoint pre_mitigated_damage = effect_value;
    FixedPoint post_mitigated_damage = 0_fp;

    const ExpressionEvaluationContext expression_context(world_, sender_id, receiver_id);
    const ExpressionStatsSource effect_stats_source =
        expression_context.MakeStatsSource(data.GetRequiredDataSourceTypes());

    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/256770049/EffectPackage.Piercing
    // Set default piercing_percentage if the attributes contains any, pure damage types won't be affected.

    FixedPoint sender_piercing_percentage =
        effect_package_attributes.piercing_percentage.Evaluate(expression_context, effect_stats_source);
    FixedPoint sender_vampiric_percentage = 0_fp;
    FixedPoint receiver_resist = 0_fp;
    FixedPoint flat_damage_reduction = 0_fp;
    HealthGainType vampiric_health_type = HealthGainType::kNone;

    std::string log_flat_damage_reduction;

    // Apply bonus and amplify
    pre_mitigated_damage = ApplyBonusDamageAndAmplify(
        sender_id,
        receiver_id,
        data,
        state,
        effect_package_attributes,
        pre_mitigated_damage);

    switch (data.type_id.damage_type)
    {
    case EffectDamageType::kPhysical:
    {
        flat_damage_reduction = receiver_stats.live.Get(StatType::kGrit);

        if (AreDebugLogsEnabled())
        {
            log_flat_damage_reduction = fmt::format("grit = {}", flat_damage_reduction);
        }

        sender_piercing_percentage +=
            sender_stats.live.Get(StatType::kPhysicalPiercingPercentage) +
            effect_package_attributes.physical_piercing_percentage.Evaluate(expression_context, effect_stats_source);
        receiver_resist = receiver_stats.live.Get(StatType::kPhysicalResist);
        sender_vampiric_percentage = sender_stats.live.Get(StatType::kPhysicalVampPercentage);
        vampiric_health_type = HealthGainType::kPhysicalVamp;
        break;
    }
    case EffectDamageType::kEnergy:
    {
        flat_damage_reduction = receiver_stats.live.Get(StatType::kResolve);

        if (AreDebugLogsEnabled())
        {
            log_flat_damage_reduction = fmt::format("resolve = {}", flat_damage_reduction);
        }

        sender_piercing_percentage +=
            sender_stats.live.Get(StatType::kEnergyPiercingPercentage) +
            effect_package_attributes.energy_piercing_percentage.Evaluate(expression_context, effect_stats_source);
        receiver_resist = receiver_stats.live.Get(StatType::kEnergyResist);
        sender_vampiric_percentage = sender_stats.live.Get(StatType::kEnergyVampPercentage);
        vampiric_health_type = HealthGainType::kEnergyVamp;
        break;
    }
    case EffectDamageType::kPure:
    {
        sender_vampiric_percentage = sender_stats.live.Get(StatType::kPureVampPercentage);
        vampiric_health_type = HealthGainType::kPureVamp;
        if (AreDebugLogsEnabled())
        {
            log_flat_damage_reduction = "no flat damage reduction";
        }

        break;
    }
    case EffectDamageType::kPurest:
    {
        receivers_with_purest_damage_.insert(receiver_id);
        break;
    }
    default:
        assert(false);
        break;
    }
    const EffectDataAttributes& attributes = data.attributes;

    // Apply crit
    std::string log_crit;
    if (state.is_critical && sender_stats.live.Get(StatType::kCritAmplificationPercentage) != 0_fp)
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/263226178/Crit+Reduction
        // Crit Reduction decreases the amount of extra damage a critical will do based on a percentage
        const FixedPoint crit_amplification_bonus_damage =
            sender_stats.live.Get(StatType::kCritAmplificationPercentage).AsPercentageOf(pre_mitigated_damage) -
            pre_mitigated_damage;

        const FixedPoint crit_reduction_piercing_percentage =
            effect_package_attributes.crit_reduction_piercing_percentage.Evaluate(
                expression_context,
                effect_stats_source);
        const FixedPoint piercing_percentage =
            std::clamp(crit_reduction_piercing_percentage, kMinPercentageFP, kMaxPercentageFP);
        const FixedPoint resist_percentage =
            std::clamp(receiver_stats.live.Get(StatType::kCritReductionPercentage), kMinPercentageFP, kMaxPercentageFP);

        const FixedPoint mitigation_percentage = kMaxPercentageFP - (resist_percentage - piercing_percentage);
        const FixedPoint crit_amplification_bonus_damage_after_mitigation =
            mitigation_percentage.AsPercentageOf(crit_amplification_bonus_damage);

        if (AreDebugLogsEnabled())
        {
            log_crit = fmt::format(
                "crit_amplification_bonus_damage = {}, crit_reduction_piercing_percentage = {}, "
                "crit_reduction_piercing_percentage = {}, "
                "mitigation_percentage = {}, crit_amplification_bonus_damage_after_mitigation = {}",
                crit_amplification_bonus_damage,
                crit_reduction_piercing_percentage,
                receiver_stats.live.Get(StatType::kCritReductionPercentage),
                mitigation_percentage,
                crit_amplification_bonus_damage_after_mitigation);
        }

        pre_mitigated_damage += crit_amplification_bonus_damage_after_mitigation;
    }

    // Apply vulnerability
    pre_mitigated_damage = Math::VulnerabilityMitigation(
        pre_mitigated_damage,
        receiver_stats.live.Get(StatType::kVulnerabilityPercentage));

    auto log_string_ptr = [&](std::string& string)
    {
        return AreDebugLogsEnabled() ? &string : nullptr;
    };

    // Optional exploit weakness
    std::string log_exploit_weakness;
    if (data.attached_effect_package_attributes.exploit_weakness)
    {
        CalculateExploitWeakness(receiver_stats.live, &pre_mitigated_damage, log_string_ptr(log_exploit_weakness));
    }

    // Calculate post mitigated damage
    std::string log_mitigation;
    if (data.type_id.damage_type == EffectDamageType::kPhysical ||
        data.type_id.damage_type == EffectDamageType::kEnergy)
    {
        const FixedPoint effect_package_piercing_percentage =
            data.attached_effect_package_attributes.piercing_percentage.Evaluate(
                expression_context,
                effect_stats_source);
        CalculatePostMitigation(
            effect_package_piercing_percentage,
            sender_piercing_percentage,
            flat_damage_reduction,
            pre_mitigated_damage,
            receiver_resist,
            &post_mitigated_damage,
            log_string_ptr(log_mitigation));
    }
    else
    {
        // Pure damage does not have mitigations
        post_mitigated_damage = pre_mitigated_damage;
    }

    std::string log_vampiric;
    const FixedPoint attached_effect_package_vampiric_percentage =
        data.attached_effect_package_attributes.vampiric_percentage.Evaluate(expression_context, effect_stats_source);
    const FixedPoint effect_package_vampiric_percentage =
        effect_package_attributes.vampiric_percentage.Evaluate(expression_context, effect_stats_source);
    const int effect_package_excess_vamp_to_shield_duration_ms =
        data.attached_effect_package_attributes.excess_vamp_to_shield_duration_ms;
    const int attached_effect_package_excess_vamp_to_shield_duration_ms =
        effect_package_attributes.excess_vamp_to_shield_duration_ms;

    if ((sender_vampiric_percentage + attached_effect_package_vampiric_percentage +
         effect_package_vampiric_percentage) > 0_fp)
    {
        if (AreDebugLogsEnabled())
        {
            log_vampiric = fmt::format(
                "vampiric_health_type = {}, sender_vampiric_percentage = {}, "
                "data.attached_effect_package_attributes.vampiric_percentage = {} "
                "effect_package_attributes.vampiric_percentage = {}",
                vampiric_health_type,
                sender_vampiric_percentage,
                attached_effect_package_vampiric_percentage,
                effect_package_vampiric_percentage);
        }

        event_data::ApplyHealthGain event_data;
        event_data.caused_by_id = sender_id;
        event_data.receiver_id = combat_unit_sender_id;
        event_data.health_gain_type = vampiric_health_type;
        event_data.value = post_mitigated_damage;
        event_data.vampiric_percentage =
            attached_effect_package_vampiric_percentage + effect_package_vampiric_percentage;
        event_data.excess_vamp_to_shield = effect_package_attributes.excess_vamp_to_shield ||
                                           data.attached_effect_package_attributes.excess_vamp_to_shield;

        if (attached_effect_package_excess_vamp_to_shield_duration_ms == kTimeInfinite ||
            effect_package_excess_vamp_to_shield_duration_ms == kTimeInfinite)
        {
            event_data.excess_vamp_to_shield_duration_ms = kTimeInfinite;
        }
        else
        {
            event_data.excess_vamp_to_shield_duration_ms = attached_effect_package_excess_vamp_to_shield_duration_ms +
                                                           effect_package_excess_vamp_to_shield_duration_ms;
        }
        event_data.source_context = state.source_context;

        world_->EmitEvent<EventType::kApplyHealthGain>(event_data);
    }

    std::string log_to_shield;
    if (data.type_id.damage_type != EffectDamageType::kPurest && attributes.to_shield_percentage > 0_fp)
    {
        // Adjust amount to send based on to_shield_percentage
        const FixedPoint to_shield_amount = attributes.to_shield_percentage.AsPercentageOf(post_mitigated_damage);

        if (AreDebugLogsEnabled())
        {
            log_to_shield = fmt::format(
                "data.to_shield_percentage = {}, to_shield_amount = {}",
                attributes.to_shield_percentage,
                to_shield_amount);
        }

        // Send event
        world_->BuildAndEmitEvent<EventType::kApplyShieldGain>(
            sender_id,
            to_shield_amount,
            effect_package_excess_vamp_to_shield_duration_ms);
    }

    const FixedPoint receiver_new_energy = receiver_stats_component.GetCurrentEnergy();
    if (receiver_new_energy != receiver_energy)
    {
        LogDebug(
            sender_id,
            "- kEnergyChanged receiver_new_energy ({}) != receiver_energy ({})",
            receiver_new_energy,
            receiver_energy);
        const FixedPoint delta = receiver_new_energy - receiver_energy;
        world_->BuildAndEmitEvent<EventType::kEnergyChanged>(sender_id, receiver_id, delta);
    }

    // Target is invulnerable if damage type is not Purest
    if (data.type_id.damage_type != EffectDamageType::kPurest && EntityHelper::IsInvulnerable(receiver_entity))
    {
        LogDebug(sender_id, "- Can't apply damage because the target is invulnerable");
        return;
    }

    // If target is not invulnerable apply damage to shield/health, will be clamped to 0

    // Apply damage to shield
    FixedPoint receiver_new_health = receiver_health;

    // Apply damage
    FixedPoint remaining_damage = post_mitigated_damage;

    // Apply shield damage if it should not be bypassed
    if (!attributes.shield_bypass && data.type_id.damage_type != EffectDamageType::kPurest &&
        post_mitigated_damage > 0_fp)
    {
        remaining_damage = ApplyShieldDamage(sender_entity, receiver_shields, post_mitigated_damage);
    }
    if (remaining_damage > 0_fp)
    {
        // Apply damage to health
        receiver_new_health -= remaining_damage;
    }

    if (!is_synergy)
    {
        const Entity& combat_unit_sender_entity = world_->GetByID(combat_unit_sender_id);

        // Faint unit if execute effect applied and within threshold
        const auto& combat_unit_sender_attached_effects_component =
            combat_unit_sender_entity.Get<AttachedEffectsComponent>();

        for (const auto& execute_effect : combat_unit_sender_attached_effects_component.GetExecutes())
        {
            if (ApplyExecuteEffect(
                    receiver_new_health,
                    execute_effect,
                    state,
                    sender_id,
                    receiver_id,
                    receiver_stats.live,
                    &receiver_new_health,
                    &post_mitigated_damage))
            {
                out_death_reason = DeathReasonType::kExecution;

                // Take execution effect for damage context
                out_source_context = execute_effect->effect_state.source_context;
            }
        }
    }

    // Set new health
    // NOTE: Notification is sent at the end of ApplyEffect
    receiver_stats_component.SetCurrentHealth(receiver_new_health);

    // Energy gain event to calculate how much energy receiver gains for taking damage
    {
        event_data::ApplyEnergyGain event_data;
        event_data.caused_by_id = sender_id;
        event_data.receiver_id = receiver_id;
        event_data.energy_gain_type = EnergyGainType::kOnTakeDamage;
        const FixedPoint health_lost = receiver_health - receiver_stats_component.GetCurrentHealth();
        event_data.value =
            Math::CalculateEnergyGainOnTakeDamage(health_lost, pre_mitigated_damage, post_mitigated_damage);

        world_->EmitEvent<EventType::kApplyEnergyGain>(event_data);
    }

    // Send on take damage event only if we applied something with a value
    if (post_mitigated_damage != 0_fp)
    {
        event_data::AppliedDamage data_damage;
        data_damage.source_context = out_source_context;
        data_damage.receiver_id = receiver_id;
        data_damage.sender_id = sender_id;
        data_damage.combat_unit_sender_id = combat_unit_sender_id;
        data_damage.damage_type = data.type_id.damage_type;
        data_damage.damage_value = post_mitigated_damage;
        data_damage.is_critical = state.is_critical;

        world_->EmitEvent<EventType::kOnDamage>(data_damage);
    }

    if (AreDebugLogsEnabled())
    {
        if (!log_to_shield.empty())
        {
            LogDebug(sender_id, "- to_shield: {}", log_to_shield);
        }
        if (!log_exploit_weakness.empty())
        {
            LogDebug(sender_id, "- exploit_weakness_percentage: {}", log_exploit_weakness);
        }
        if (!log_crit.empty())
        {
            LogDebug(sender_id, "- crit: {}", log_crit);
        }
        if (!log_vampiric.empty())
        {
            LogDebug(sender_id, "- vampiric: {}", log_vampiric);
        }
        if (!log_mitigation.empty())
        {
            LogDebug(sender_id, "- mitigation: {}", log_mitigation);
        }
        LogDebug(
            sender_id,
            "- pre_mitigated_damage = {}, post_mitigated_damage = {}",
            pre_mitigated_damage,
            post_mitigated_damage);
        LogDebug(
            sender_id,
            "- target: health = {}, shield = {}, vulnerability_percentage = {}, {}",
            receiver_stats_component.GetCurrentHealth(),
            world_->GetShieldTotal(receiver_entity),
            receiver_stats.live.Get(StatType::kVulnerabilityPercentage),
            log_flat_damage_reduction);
    }
}

void EffectSystem::CalculateExploitWeakness(
    const StatsData& receiver_live_stats,
    FixedPoint* out_pre_mitigated_damage,
    std::string* out_log_exploit_weakness) const
{
    const FixedPoint missing_health_percentage =
        kMaxPercentageFP - receiver_live_stats.Get(StatType::kCurrentHealth)
                               .AsProportionalPercentageOf(receiver_live_stats.Get(StatType::kMaxHealth));

    if (out_log_exploit_weakness)
    {
        *out_log_exploit_weakness =
            fmt::format("data.exploit_weakness = true, missing_health_percentage = {}", missing_health_percentage);
    }

    // multiply by 100% plus the percentage of missing health of the recipient.
    const FixedPoint adjusted_damage =
        (kMaxPercentageFP + missing_health_percentage).AsPercentageOf(*out_pre_mitigated_damage);

    *out_pre_mitigated_damage = adjusted_damage;
}

void EffectSystem::ApplyDisplacementEffect(
    const Entity& sender_entity,
    const Entity& receiver_entity,
    const EffectData& data,
    const EffectState& state)
{
    const EntityID sender_id = sender_entity.GetID();
    const EntityID receiver_id = receiver_entity.GetID();

    const auto& sender_position_component = sender_entity.Get<PositionComponent>();

    auto& displacement_component = receiver_entity.Get<DisplacementComponent>();
    auto& receiver_position_component = receiver_entity.Get<PositionComponent>();

    // Set up the target according to type of displacement
    HexGridPosition target_position_sub_units = kInvalidHexHexGridPosition;

    ILLUVIUM_ENSURE_ENUM_SIZE(EffectDisplacementType, 6);
    switch (data.type_id.displacement_type)
    {
    case EffectDisplacementType::kKnockUp:
    {
        // Target is right where it is
        target_position_sub_units = receiver_position_component.ToSubUnits();
        break;
    }
    case EffectDisplacementType::kKnockBack:
    {
        const int sub_units_to_move = data.displacement_distance_sub_units;

        // Calculate vectors for desired position
        const HexGridPosition start = sender_position_component.ToSubUnits();
        const HexGridPosition end = receiver_position_component.ToSubUnits();
        const HexGridPosition destination_vector = end - start;

        // Calculate direction
        const HexGridPosition destination_vector_normal = destination_vector.ToNormalizedAndScaled();

        // Calculate destination past the end (receiver) position
        const HexGridPosition movement_destination =
            (destination_vector_normal * sub_units_to_move) / kPrecisionFactor + end;

        target_position_sub_units = movement_destination;
        break;
    }
    case EffectDisplacementType::kPull:
    {
        // Target is near sender
        target_position_sub_units = sender_position_component.ToSubUnits();
        break;
    }
    case EffectDisplacementType::kThrowToHighestEnemyDensity:
    {
        const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
        const int receiver_radius = receiver_position_component.GetRadius();
        const GridHelper& grid_helper = world_->GetGridHelper();
        GridHelper::BuildObstaclesParameters build_parameters;
        build_parameters.mark_borders_as_obstacles = true;
        build_parameters.source = &receiver_entity;
        grid_helper.BuildObstacles(build_parameters);
        HexGridPosition throw_to_postion = targeting_helper.FindMaxEnemyOverlapPosition(
            sender_id,
            receiver_radius,
            receiver_radius + 1,
            {receiver_id});
        if (throw_to_postion == kInvalidHexHexGridPosition)
        {
            throw_to_postion = receiver_position_component.GetPosition();
        }

        // Reserve destination and set overlapable, so other units won't be avoiding it
        receiver_position_component.SetReservedPosition(throw_to_postion);
        receiver_position_component.SetOverlapable(true);

        target_position_sub_units = throw_to_postion.ToSubUnits();

        LogDebug(sender_id, "- Start throw to location = {}", throw_to_postion);

        break;
    }
    case EffectDisplacementType::kThrowToFurthestEnemy:
    {
        const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
        // TODO(shchavinskyi): We may need to add targeting params later
        EntityID secondary_target_entity_id = targeting_helper.GetFurthestEnemyEntity(sender_id, {receiver_id});
        if (secondary_target_entity_id == kInvalidEntityID)
        {
            secondary_target_entity_id = receiver_id;
        }
        const auto& secondary_target_entity = world_->GetByID(secondary_target_entity_id);
        const auto& secondary_target_position_component = secondary_target_entity.Get<PositionComponent>();

        const GridHelper& grid_helper = world_->GetGridHelper();

        // Build obstacle map to find open spots
        grid_helper.BuildObstacles(&receiver_entity);
        const auto open_position_near_secondary_target = grid_helper.GetOpenPositionInFront(
            sender_position_component.GetPosition(),
            secondary_target_position_component.GetPosition(),
            secondary_target_position_component.GetRadius(),
            receiver_position_component.GetRadius());

        // Reserve destination and set overlapable, so other units won't be avoiding it
        receiver_position_component.SetReservedPosition(open_position_near_secondary_target);
        receiver_position_component.SetOverlapable(true);

        target_position_sub_units = open_position_near_secondary_target.ToSubUnits();

        LogDebug(sender_id, "- Start throw to location = {}", open_position_near_secondary_target);

        break;
    }
    default:
    {
        assert(false);
        break;
    }
    }

    // Update the component
    displacement_component.SetSenderID(sender_id);
    displacement_component.SetDisplacementType(data.type_id.displacement_type);
    displacement_component.SetDisplacementTargetPosition(target_position_sub_units.ToUnits());

    if (data.event_skills.count(EventType::kDisplacementStopped) > 0)
    {
        displacement_component.SetDisplacementEndSkill(data.event_skills.at(EventType::kDisplacementStopped));
    }
    else
    {
        displacement_component.ClearDisplacementEndSkill();
    }

    // Add the attached effect
    const auto& attached_effect_state =
        world_->GetAttachedEffectsHelper().AddAttachedEffect(receiver_entity, sender_id, data, state);
    displacement_component.SetAttachedEffectState(attached_effect_state);

    LogDebug(sender_id, "- Added Displacement attached effect to receiver = {}", receiver_id);

    // Emit event
    event_data::Displacement event_data;
    event_data.sender_id = sender_id;
    event_data.combat_unit_sender_id = world_->GetCombatUnitParentID(sender_id);
    event_data.receiver_id = receiver_id;
    event_data.displacement_type = data.type_id.displacement_type;
    event_data.duration_time_ms = data.lifetime.duration_time_ms;

    world_->EmitEvent<EventType::kDisplacementStarted>(event_data);
}

void EffectSystem::CalculatePostMitigation(
    const FixedPoint effect_package_piercing_percentage,
    const FixedPoint sender_piercing_percentage,
    const FixedPoint flat_damage_reduction,
    const FixedPoint pre_mitigated_damage,
    const FixedPoint receiver_resist,
    FixedPoint* out_post_mitigated_damage,
    std::string* out_log_mitigation) const
{
    // Reduce by flat value
    *out_post_mitigated_damage = pre_mitigated_damage - flat_damage_reduction;

    const FixedPoint piercing_percentage =
        std::clamp(sender_piercing_percentage + effect_package_piercing_percentage, kMinPercentageFP, kMaxPercentageFP);

    const FixedPoint final_piercing_percentage = kMaxPercentageFP - piercing_percentage;
    const FixedPoint mitigation = final_piercing_percentage.AsPercentageOf(receiver_resist);

    // Don't allow negative post mitigation damage
    *out_post_mitigated_damage = std::max(0_fp, Math::PostMitigationDamage(*out_post_mitigated_damage, mitigation));

    if (out_log_mitigation)
    {
        *out_log_mitigation = fmt::format(
            "sender_piercing_percentage = {}, data.attached_effect_package_attributes.piercing_percentage = {}, "
            "final_piercing_percentage = {}, receiver_resist = {}, mitigation = {}",
            sender_piercing_percentage,
            effect_package_piercing_percentage,
            final_piercing_percentage,
            receiver_resist,
            mitigation);
    }
}

bool EffectSystem::ApplyExecuteEffect(
    FixedPoint receiver_health,
    const AttachedEffectStatePtr& execute_effect,
    const EffectState& state,
    const EntityID sender_id,
    const EntityID receiver_id,
    const StatsData& receiver_live_stats,
    FixedPoint* out_receiver_new_health,
    FixedPoint* out_post_mitigated_damage) const
{
    // Ignore if linked to wrong type of ability
    const EffectPackageHelper& effect_package_helper = world_->GetEffectPackageHelper();
    if (!effect_package_helper.DoesAbilityTypesMatchActivatedAbility(
            execute_effect->effect_data.ability_types,
            state.source_context.combat_unit_ability_type))
    {
        return false;
    }

    // Calculate health values
    const FixedPoint current_health_percentage =
        receiver_health.AsProportionalPercentageOf(receiver_live_stats.Get(StatType::kMaxHealth));
    const FixedPoint target_health_percentage =
        world_->EvaluateExpression(execute_effect->effect_data.GetExpression(), execute_effect->sender_id, receiver_id);

    // Did not hit threshold
    if (current_health_percentage > target_health_percentage)
    {
        return false;
    }

    // Damage until no health remaining
    *out_post_mitigated_damage += *out_receiver_new_health;
    *out_receiver_new_health = 0_fp;

    // Log and emit event
    LogDebug(
        sender_id,
        "- EXECUTING receiver_id = {}, current_health_percentage = {}, target_health_percentage = {}",
        receiver_id,
        current_health_percentage,
        target_health_percentage);

    return true;
}

FixedPoint EffectSystem::ApplyBonusDamageAndAmplify(
    const EntityID sender_id,
    const EntityID receiver_id,
    const EffectData& data,
    const EffectState& state,
    const EffectPackageAttributes& effect_package_attributes,
    const FixedPoint pre_mitigated_damage) const
{
    if (data.type_id.type != EffectType::kInstantDamage && data.type_id.type != EffectType::kDamageOverTime)
    {
        return 0_fp;
    }

    const ExpressionEvaluationContext expression_context(world_, sender_id, receiver_id);
    // Damage Bonus https://illuvium.atlassian.net/wiki/spaces/AB/pages/247791738/EffectPackage.DamageBonus
    const FixedPoint bonus_value = effect_package_attributes.GetDamageBonusForDamageType(
        expression_context,
        data.type_id.damage_type,
        state.sender_stats);

    // Damage Amplification
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/247530406/EffectPackage.DamageAmplification
    const FixedPoint damage_type_amp = effect_package_attributes.GetDamageAmplificationForDamageType(
        expression_context,
        data.type_id.damage_type,
        state.sender_stats);
    const FixedPoint ability_type_amp = StatsHelper::GetDamageAmplificationForAbilityType(
        state.source_context.combat_unit_ability_type,
        state.sender_stats,
        world_->GetMaxAttackSpeed());
    const FixedPoint bonus_amplification = damage_type_amp + ability_type_amp;

    // Nothing to do
    if (bonus_value == 0_fp && bonus_amplification == 0_fp)
    {
        return pre_mitigated_damage;
    }

    const FixedPoint total_value = pre_mitigated_damage + bonus_value;
    const FixedPoint total_percentage = kMaxPercentageFP + bonus_amplification;

    // Don't allow negative effect value
    const FixedPoint final_effect_value = std::max(0_fp, total_percentage.AsPercentageOf(total_value));

    LogDebug(
        sender_id,
        "| ApplyBonusDamageAndAmplify - pre_mitigated_value = {}, bonus_value = {}, bonus_amplification = {}, "
        "final_effect_value = {}",
        pre_mitigated_damage,
        bonus_value,
        bonus_amplification,
        final_effect_value);

    return final_effect_value;
}

bool EffectSystem::IsDeniedByAgency(
    const Entity& sender_entity,
    const Entity& receiver_entity,
    const EffectData& data,
    const EffectState& state) const
{
    // Special case for overload system
    if (state.source_context.Has(SourceContextType::kOverload))
    {
        return false;
    }

    // Is this effect coming from an ally
    bool is_from_ally = sender_entity.IsAlliedWith(receiver_entity);

    // Special case for marks where we flip the sign
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/243367937/Detrimental+Mark+Effect
    if (world_->HasEntity(state.attached_from_entity_id))
    {
        const Team attached_from_team = world_->GetByID(state.attached_from_entity_id).GetTeam();

        const bool previous_is_from_ally = is_from_ally;
        is_from_ally = receiver_entity.IsAlliedWith(attached_from_team);
        LogDebug(
            sender_entity.GetID(),
            "- FLIPPING friendly fire - attached_from_entity_id = {}, attached_from_team = {}, "
            "previous_is_from_ally = {}, new_is_from_ally = {}",
            state.attached_from_entity_id,
            attached_from_team,
            previous_is_from_ally,
            is_from_ally);
    }

    if (data.IsTypeBeneficial() && !is_from_ally)
    {
        LogDebug(
            sender_entity.GetID(),
            "- DENIED because of agency (effect type is beneficial but the receiver_id = {} is an enemy)",
            receiver_entity.GetID());
        return true;
    }
    if (data.IsTypeDetrimental() && is_from_ally)
    {
        LogDebug(
            sender_entity.GetID(),
            "- DENIED because of agency (effect type is detrimental but the receiver_id = {} is an ally)",
            receiver_entity.GetID());
        return true;
    }

    return false;
}
}  // namespace simulation