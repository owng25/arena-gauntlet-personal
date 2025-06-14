#include "systems/attached_effects_system.h"

#include <vector>

#include "components/attached_effects_component.h"
#include "components/attached_entity_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "data/constants.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "systems/ability_system.h"
#include "systems/focus_system.h"
#include "utility/attached_effects_helper.h"
#include "utility/effect_package_helper.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/grid_helper.h"
#include "utility/math.h"
#include "utility/vector_helper.h"

namespace simulation
{
void AttachedEffectsSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kAbilityDeactivated>(this, &Self::OnAbilityDeactivated);
}

void AttachedEffectsSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Only apply to entities with AttachedEffectsComponent
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return;
    }

    // Get the attached effects component
    TimeStepAttachedEffects(entity);
}

void AttachedEffectsSystem::PostTimeStep(const Entity& entity)
{
    if (!entity.IsActive())
    {
        return;
    }

    if (!entity.Has<AttachedEffectsComponent>())
    {
        return;
    }

    // Don't allow to process effects if the battle finished
    if (world_->IsBattleFinished())
    {
        return;
    }

    auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    attached_effects_component.EraseDestroyedEffects();
}

void AttachedEffectsSystem::PostSystemTimeStep()
{
    Super::PostSystemTimeStep();

    // Don't allow to process effects if the battle finished
    if (world_->IsBattleFinished())
    {
        return;
    }

    UpdateDynamicBuffsDebuffs();
}

void AttachedEffectsSystem::UpdateDynamicBuffsDebuffs()
{
    const AttachedEffectsHelper& attached_effects_helper = world_->GetAttachedEffectsHelper();
    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            if (auto* attached_effects_component = entity.GetPtr<AttachedEffectsComponent>())
            {
                for (const AttachedEffectStatePtr& effect_state : attached_effects_component->GetAttachedEffects())
                {
                    const EffectData& effect_data = effect_state->effect_data;

                    // Only buffs/debuffs currently
                    if (!effect_data.type_id.UsesCapturedValue()) continue;

                    // Update only dynamic effects here
                    if (effect_data.HasStaticFrequency()) continue;

                    if (effect_state->current_time_steps % effect_state->frequency_time_steps == 0)
                    {
                        attached_effects_helper.RefreshDynamicBuffsDebuffs(entity, *effect_state);
                    }
                }
            }
        });
}

void AttachedEffectsSystem::TimeStepAttachedEffects(const Entity& receiver_entity)
{
    // Don't allow to time step the attached effects if the battle finished
    if (world_->IsBattleFinished())
    {
        return;
    }

    const EntityID receiver_id = receiver_entity.GetID();
    const auto& attached_effects_helper = world_->GetAttachedEffectsHelper();
    const auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();
    const auto& attached_effects = attached_effects_component.GetAttachedEffects();

    // Iterate over attached effects
    // Store the size here so that we don't iterate over an attached effect that was added while in
    // the for loop
    const size_t size_before_loop = attached_effects.size();
    for (size_t index = 0; index != size_before_loop; index++)
    {
        // Ensure that array did not shring during the loop
        assert(attached_effects.size() >= size_before_loop);

        // A battle can finish while in this for loop, if this happens just early return here.
        if (world_->IsBattleFinished())
        {
            return;
        }

        // NOTE: Don't make this into return by reference otherwise this might point to an
        // invalid reference inside the vector if it gets resized
        const auto attached_effect = attached_effects[index];
        if (!attached_effect)
        {
            continue;
        }

        // TODO(vampy): Add a test case for this, most likely zones
        if (!world_->HasEntity(attached_effect->combat_unit_sender_id) &&
            !EntityHelper::IsASynergy(*world_, attached_effect->sender_id) &&
            !EntityHelper::IsADroneAugment(*world_, attached_effect->sender_id))
        {
            LogWarn(
                receiver_id,
                "AttachedEffectsSystem::TimeStep - combat_unit_sender_id = {} does not exist "
                "anymore. Removing "
                "attached effect.",
                attached_effect->combat_unit_sender_id);

            // Mark for removal if expired
            attached_effects_helper.RemoveAttachedEffect(receiver_entity, attached_effect);
            continue;
        }

        switch (attached_effect->state)
        {
        case AttachedEffectStateType::kPendingActivation:
        {
            const auto& validations = attached_effect->effect_data.validations;

            // Note: If the validation list is something unreasonable then this effect will never activate
            // Example: CurrentHealth % > 100
            if (!ShouldActivateEffect(receiver_id, attached_effect->combat_unit_sender_id, validations))
            {
                if (attached_effect->effect_data.lifetime.deactivate_if_validation_list_not_valid)
                {
                    attached_effects_helper.RemoveAttachedEffect(receiver_entity, attached_effect);
                }
                break;
            }

            // Set active and apply
            ActivateEffect(receiver_entity, *attached_effect);
            break;
        }
        case AttachedEffectStateType::kActive:
        {
            const auto& validations = attached_effect->effect_data.validations;

            if (!ShouldActivateEffect(receiver_id, attached_effect->combat_unit_sender_id, validations))
            {
                if (attached_effect->effect_data.lifetime.deactivate_if_validation_list_not_valid)
                {
                    attached_effects_helper.RemoveAttachedEffect(receiver_entity, attached_effect);
                    break;
                }
            }

            TimeStepAttachedEffect(receiver_entity, attached_effect);
            break;
        }
        case AttachedEffectStateType::kWaiting:
        {
            // Is data initialized?
            if (!attached_effect->is_data_initialized)
            {
                InitEffectStateData(receiver_entity, *attached_effect);
            }

            TimeStepAttachedEffect(receiver_entity, attached_effect);
            break;
        }
        default:
            break;
        }
    }
}

bool AttachedEffectsSystem::ShouldActivateEffect(
    const EntityID sender_id,
    const EntityID receiver_id,
    const EffectValidations& effect_validations) const
{
    if (effect_validations.IsEmpty())
    {
        return true;
    }

    // Distance check
    for (const auto& distance_check : effect_validations.distance_checks)
    {
        EntityID entity_id = 0;
        if (distance_check.first_entity == ValidationStartEntity::kSelf)
        {
            entity_id = sender_id;
        }
        else if (distance_check.first_entity == ValidationStartEntity::kCurrentFocus)
        {
            entity_id = receiver_id;
        }

        if (!distance_check.Check(*world_, entity_id))
        {
            return false;
        }
    }

    // Expression comparisons check
    const ExpressionEvaluationContext expression_context(world_, sender_id, receiver_id);
    const ExpressionStatsSource stats_source =
        expression_context.MakeStatsSource(effect_validations.GatherRequiredDataSourceTypes());
    for (const auto& effect_validation : effect_validations.expression_comparisons)
    {
        if (!effect_validation.Check(expression_context, stats_source))
        {
            return false;
        }
    }

    return true;
}

bool AttachedEffectsSystem::ShouldBeRemovedInPostTimeStep(const AttachedEffectStatePtr& attached_effect_state)
{
    // Blink effects should be removed in PostTimeStep
    return attached_effect_state->effect_data.type_id.type == EffectType::kBlink;
}

void AttachedEffectsSystem::TimeStepAttachedEffect(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect)
{
    const EntityID receiver_id = receiver_entity.GetID();

    // Advance current effect timer
    attached_effect->current_time_steps++;

    // Did timer expire?
    if (attached_effect->IsExpired())
    {
        // Mark for removal if expired
        LogDebug(
            receiver_id,
            "AttachedEffectsSystem::TimeStepAttachedEffect - attached_effect_state EXPIRED - {}",
            *attached_effect);

        world_->GetAttachedEffectsHelper().RemoveAttachedEffect(receiver_entity, attached_effect);
    }
    else if (attached_effect->apply_over_time)
    {
        // Effect that is applied over time
        TimeStepApplyEffectOverTime(receiver_entity, *attached_effect);
    }
    else if (attached_effect->effect_data.type_id.type == EffectType::kBlink)
    {
        TimeStepBlink(receiver_entity, *attached_effect);
    }
}

void AttachedEffectsSystem::OnAbilityDeactivated(const event_data::AbilityDeactivated& data)
{
    if (data.ability_type == AbilityType::kInnate)
    {
        return;
    }

    if (!world_->HasEntity(data.combat_unit_sender_id))
    {
        return;
    }

    const auto& sender_entity = world_->GetByID(data.combat_unit_sender_id);
    if (!sender_entity.IsActive())
    {
        return;
    }

    // We should not count ability deactivation of children entities
    if (data.combat_unit_sender_id != data.sender_id)
    {
        return;
    }

    // Only interested in omegas or attacks
    if (sender_entity.Has<AttachedEffectsComponent>())
    {
        const auto& attached_effects_component = sender_entity.Get<AttachedEffectsComponent>();

        // Empowers
        AbilityDeactivatedIncreaseConsumableActivations(sender_entity, attached_effects_component.GetEmpowers(), data);

        // Disempowers
        AbilityDeactivatedIncreaseConsumableActivations(
            sender_entity,
            attached_effects_component.GetDisempowers(),
            data);

        // Positive States
        const auto& map_positives_states = attached_effects_component.GetPositiveStates();
        for (const auto& [positive_state, effects_vector] : map_positives_states)
        {
            AbilityDeactivatedIncreaseConsumableActivations(sender_entity, effects_vector, data);
        }

        // Negative States
        const auto& map_negative_states = attached_effects_component.GetNegativeStates();
        for (const auto& [negative_state, effects_vector] : map_negative_states)
        {
            AbilityDeactivatedIncreaseConsumableActivations(sender_entity, effects_vector, data);
        }

        // Plane Changes
        const auto& map_plane_changes = attached_effects_component.GetPlaneChanges();
        for (const auto& [plane_change, effects_vector] : map_plane_changes)
        {
            AbilityDeactivatedIncreaseConsumableActivations(sender_entity, effects_vector, data);
        }

        // Buffs/debuffs
        const AttachedEffectBuffsState& attached_buffs_state = attached_effects_component.GetBuffsState();
        for (const auto& [stat_type, effects_vector] : attached_buffs_state.buffs)
        {
            AbilityDeactivatedIncreaseConsumableActivations(sender_entity, effects_vector, data);
        }
        for (const auto& [stat_type, effects_vector] : attached_buffs_state.debuffs)
        {
            AbilityDeactivatedIncreaseConsumableActivations(sender_entity, effects_vector, data);
        }
    }
}

void AttachedEffectsSystem::AbilityDeactivatedIncreaseConsumableActivations(
    const Entity& sender,
    const std::vector<AttachedEffectStatePtr>& attached_effects,
    const event_data::AbilityDeactivated& event) const
{
    std::shared_ptr<AbilityState> ability_state;
    const auto& abilities_component = sender.Get<AbilitiesComponent>();
    const auto& abilities = abilities_component.GetAbilities(event.ability_type);
    if (event.ability_index < abilities.state.abilities.size())
    {
        ability_state = abilities.state.abilities[event.ability_index];
    }

    const AttachedEffectsHelper& attached_effects_helper = world_->GetAttachedEffectsHelper();
    for (const AttachedEffectStatePtr& attached_effect : attached_effects)
    {
        // This can be consumed by us
        if (!attached_effect->can_be_consumed_by_ability_deactivations)
        {
            continue;
        }

        // In case of consumable empowers we have to check that this empower was
        // actually used by the current activation of an ability to scenarios like this
        //
        // - Attack ability activation
        // - Skill fainst some entity,
        // -     which triggers on Vanquish innate with with consumable attack empower that expires after 1 activation
        // - ability deactivates and increases empower consumable activations
        // - empower expires even though it wasn't used to empower any skill
        if (ability_state && attached_effect->effect_data.type_id.type == EffectType::kEmpower &&
            attached_effect->IsConsumable() &&
            ability_state->consumable_empowers_used.count(attached_effect.get()) == 0)
        {
            continue;
        }

        if (!attached_effects_helper
                 .IncreaseConsumableActivations(sender.GetID(), attached_effect, event.ability_type, event.is_critical))
        {
            continue;
        }

        LogDebug(sender.GetID(), "AttachedEffectsSystem::IncreaseConsumableActivations - {}", *attached_effect);
    }
}

bool AttachedEffectsSystem::InitEffectStateData(
    const Entity& receiver_entity,
    AttachedEffectState& attached_effect_state) const
{
    const EntityID receiver_id = receiver_entity.GetID();

    // Already initialized
    if (attached_effect_state.is_data_initialized)
    {
        return true;
    }

    const EffectData& effect_data = attached_effect_state.effect_data;
    const EntityID combat_unit_sender_id = attached_effect_state.combat_unit_sender_id;
    if (!world_->HasEntity(combat_unit_sender_id) &&
        !EntityHelper::IsASynergy(*world_, attached_effect_state.sender_id) &&
        !EntityHelper::IsADroneAugment(*world_, attached_effect_state.sender_id))
    {
        LogDebug("- combat_unit_sender_id = {} does not exist anymore. ABORTING", combat_unit_sender_id);
        return false;
    }

    // Capture stats of combat unit currently focused by the sender if so required by effect
    if (effect_data.GetRequiredDataSourceTypes().Contains(ExpressionDataSourceType::kSenderFocus))
    {
        const Entity& sender_entity = world_->GetByID(combat_unit_sender_id);
        if (sender_entity.Has<FocusComponent>())
        {
            const auto& focus_component = sender_entity.Get<FocusComponent>();
            const EntityID focused_entity_id = focus_component.GetFocusID();
            attached_effect_state.effect_state.sender_focus_stats = world_->GetFullStats(focused_entity_id);
        }
        else
        {
            LogWarn(
                "InitEffectStateData: Failed to get stats of sender current focus since sender ({}) does not have "
                "focus component.",
                combat_unit_sender_id);
        }
    }

    // Check if we have an apply over time effect
    // OR if the effect is already default applied
    auto& apply_over_time_data = attached_effect_state.apply_over_time_data;
    const EffectTypeID& type_id = effect_data.type_id;
    switch (type_id.type)
    {
    case EffectType::kCondition:
        // Just by existing here, this effect is already applied
        // Otherwise we will be creating the same effect in every time step
        attached_effect_state.is_default_applied = true;
        break;

    case EffectType::kEnergyGainOverTime:
    case EffectType::kHyperGainOverTime:
    case EffectType::kHealOverTime:
    case EffectType::kDamageOverTime:
    case EffectType::kEnergyBurnOverTime:
    case EffectType::kHyperBurnOverTime:
        attached_effect_state.apply_over_time = true;
        apply_over_time_data.total_value =
            world_->EvaluateExpression(effect_data.GetExpression(), combat_unit_sender_id, receiver_id);
        break;
    case EffectType::kBlink:
    case EffectType::kPositiveState:
    case EffectType::kPlaneChange:
        attached_effect_state.is_default_applied = true;
        break;
    case EffectType::kNegativeState:
    {
        // Just by existing here, this effect is already applied
        // Otherwise we will be creating the same effect in every time step
        attached_effect_state.is_default_applied = true;

        // Adjust the duration of the negative state based on tenacity_percentage if duration is not
        // kTimeInfinite
        if (effect_data.lifetime.duration_time_ms != kTimeInfinite)
        {
            const StatsData receiver_live_stats = world_->GetLiveStats(receiver_entity);
            const FixedPoint percentage = kMaxPercentageFP - receiver_live_stats.Get(StatType::kTenacityPercentage);
            const int duration_time_ms = Math::PercentageOf(percentage.AsInt(), effect_data.lifetime.duration_time_ms);
            attached_effect_state.duration_time_steps = Time::MsToTimeSteps(duration_time_ms);
        }
        break;
    }

    case EffectType::kBuff:
    case EffectType::kDebuff:
        // Just by existing here the effect is already applied
        // See StatsComponent::GetLiveStats and AttachedEffectBuffsState
        attached_effect_state.is_default_applied = true;

        // Only static ones get some value on application.
        // Dynamic wait for the end of the frame.
        if (effect_data.HasStaticFrequency() && !attached_effect_state.captured_effect_value.has_value())
        {
            attached_effect_state.CaptureEffectValue(*world_, receiver_id);
        }
        break;

    case EffectType::kEmpower:
    case EffectType::kDisempower:
        // Just by existing here the effect is already applied
        attached_effect_state.is_default_applied = true;
        break;

    case EffectType::kDisplacement:
        // Just by existing here, this effect is already applied
        // Otherwise we will be creating the same effect in every time step
        attached_effect_state.is_default_applied = true;
        break;

    case EffectType::kExecute:
        // Just by existing here the effect is already applied
        attached_effect_state.is_default_applied = true;
        break;

    default:
        break;
    }

    if (attached_effect_state.apply_over_time)
    {
        // Check for invalid duration
        if (attached_effect_state.duration_time_steps == 0)
        {
            LogDebug(
                receiver_id,
                "- INVALID, aborting because duration_time_steps = 0, duration_time_ms = {}",
                effect_data.lifetime.duration_time_ms);
            return false;
        }
        // Example: The damage over time effect poison is attached

        // Calculate the apply effect value per frequency time step
        apply_over_time_data.value_per_frequency_time_step =
            (apply_over_time_data.total_value * FixedPoint::FromInt(attached_effect_state.frequency_time_steps)) /
            FixedPoint::FromInt(attached_effect_state.duration_time_steps);
    }

    attached_effect_state.is_data_initialized = true;
    return true;
}

void AttachedEffectsSystem::ActivateEffect(const Entity& receiver_entity, AttachedEffectState& attached_effect_state)
    const
{
    const EntityID receiver_id = receiver_entity.GetID();

    const EntityID combat_unit_sender_id = attached_effect_state.combat_unit_sender_id;
    const EffectData& effect_to_apply = attached_effect_state.effect_data;
    LogDebug(
        receiver_id,
        "AttachedEffectsSystem::ActivateEffect - {}, duration_time_steps = {}",
        effect_to_apply,
        attached_effect_state.duration_time_steps);

    if (!InitEffectStateData(receiver_entity, attached_effect_state))
    {
        return;
    }

    // Mark it as active
    attached_effect_state.state = AttachedEffectStateType::kActive;

    if (attached_effect_state.apply_over_time)
    {
        // Do apply effect over time
        const auto& apply_over_time_data = attached_effect_state.apply_over_time_data;
        LogDebug(
            receiver_id,
            "- ApplyOverTime: value_per_frequency_time_step = {}, total_value = {}, "
            "duration_time_steps = {}",
            apply_over_time_data.value_per_frequency_time_step,
            apply_over_time_data.total_value,
            attached_effect_state.duration_time_steps);
        TimeStepApplyEffectOverTime(receiver_entity, attached_effect_state);
    }
    else if (attached_effect_state.effect_data.type_id.type == EffectType::kBlink)
    {
        TimeStepBlink(receiver_entity, attached_effect_state);
    }
    else if (!attached_effect_state.is_default_applied)
    {
        ApplyEffect(combat_unit_sender_id, receiver_id, effect_to_apply, attached_effect_state.effect_state);
    }
    else
    {
        LogDebug(receiver_id, "- effect = {} is default applied, nothing else to do", effect_to_apply.type_id.type);
    }
}

void AttachedEffectsSystem::TimeStepApplyEffectOverTime(
    const Entity& receiver_entity,
    AttachedEffectState& attached_effect_state) const
{
    const EntityID receiver_id = receiver_entity.GetID();
    LogDebug(receiver_id, "AttachedEffectsSystem::TimeStepApplyEffectOverTime - {}", attached_effect_state);

    if (!attached_effect_state.apply_over_time)
    {
        return;
    }
    if (attached_effect_state.state == AttachedEffectStateType::kWaiting)
    {
        return;
    }

    auto& apply_over_time = attached_effect_state.apply_over_time_data;
    const EffectData& effect = attached_effect_state.effect_data;
    const bool is_last_time_step =
        attached_effect_state.current_time_steps == attached_effect_state.duration_time_steps - 1;

    // This value will apply on every frequency time
    // Example: DOT, 500 physical damage over 5 seconds
    // DOT attached
    // - 1s passes - Apply 100 damage
    // - 2s passes - Apply 100 damage
    // - 3s passes - Apply 100 damage
    // - 4s passes - Apply 100 damage
    // - 5s passes - Apply 100 damage
    FixedPoint value_to_send = 0_fp;
    if (is_last_time_step)
    {
        // Last time step, add the difference back
        value_to_send = std::max(0_fp, apply_over_time.total_value - apply_over_time.current_value);
    }
    else
    {
        if (attached_effect_state.frequency_time_steps != 0 &&
            (attached_effect_state.current_time_steps + 1) % attached_effect_state.frequency_time_steps == 0)
        {
            // value with frequency steps.
            value_to_send = apply_over_time.value_per_frequency_time_step;
        }
    }

    // If there is no damage, you do not need to continue.
    if (value_to_send == 0_fp)
    {
        return;
    }

    // Keep track of the total applied
    apply_over_time.current_value += value_to_send;

    // Debug time constraint
    if (is_last_time_step)
    {
        if (apply_over_time.current_value != apply_over_time.total_value)
        {
            LogErr(
                receiver_id,
                "- current_value ({}) != total_value ({})",
                apply_over_time.current_value,
                apply_over_time.total_value);
        }
    }
    else
    {
        if (apply_over_time.current_value > apply_over_time.total_value)
        {
            LogErr(
                receiver_id,
                "- current_value ({}) > total_value ({})",
                apply_over_time.current_value,
                apply_over_time.total_value);
        }
    }

    // Figure out the effect package to send
    EffectData effect_to_send{};
    switch (effect.type_id.type)
    {
    case EffectType::kDamageOverTime:
    {
        effect_to_send =
            EffectData::CreateDamage(effect.type_id.damage_type, EffectExpression::FromValue(value_to_send));
        break;
    }
    case EffectType::kHealOverTime:
    {
        effect_to_send = EffectData::CreateHeal(effect.type_id.heal_type, EffectExpression::FromValue(value_to_send));
        break;
    }
    case EffectType::kEnergyGainOverTime:
    {
        effect_to_send = EffectData::Create(EffectType::kInstantEnergyGain, EffectExpression::FromValue(value_to_send));
        break;
    }
    case EffectType::kEnergyBurnOverTime:
    {
        effect_to_send = EffectData::Create(EffectType::kInstantEnergyBurn, EffectExpression::FromValue(value_to_send));
        break;
    }
    case EffectType::kHyperGainOverTime:
    {
        effect_to_send = EffectData::Create(EffectType::kInstantHyperGain, EffectExpression::FromValue(value_to_send));
        break;
    }
    case EffectType::kHyperBurnOverTime:
    {
        effect_to_send = EffectData::Create(EffectType::kInstantHyperBurn, EffectExpression::FromValue(value_to_send));
        break;
    }
    default:
        break;
    }

    effect_to_send.attributes = attached_effect_state.effect_data.attributes;

    // Send the effect
    ApplyEffect(
        attached_effect_state.combat_unit_sender_id,
        receiver_id,
        effect_to_send,
        attached_effect_state.effect_state);
}

void AttachedEffectsSystem::ApplyEffect(
    const EntityID combat_unit_sender_id,
    const EntityID receiver_id,
    const EffectData& effect_data,
    const EffectState& effect_state) const
{
    LogDebug(receiver_id, "AttachedEffectsSystem::ApplyEffect");
    world_->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        combat_unit_sender_id,
        receiver_id,
        effect_data,
        effect_state);
}

void AttachedEffectsSystem::RemoveAllEffects()
{
    auto& attached_effects_helper = world_->GetAttachedEffectsHelper();
    // Iterate all entities
    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            if (!entity.IsActive() || !entity.Has<AttachedEffectsComponent>())
            {
                // Nothing to work on
                return;
            }

            // Get component and effects
            auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

            // Remove all attached effects
            const auto& attached_effects = attached_effects_component.GetAttachedEffects();
            // Walk by indices here because new effects can still appear...
            for (size_t i = 0; i < attached_effects.size(); ++i)
            {
                attached_effects_helper.RemoveAttachedEffect(entity, attached_effects[i]);
            }

            constexpr bool always_destroy_delayed = true;
            attached_effects_component.EraseDestroyedEffects(always_destroy_delayed);
        });
}

void AttachedEffectsSystem::TimeStepBlink(const Entity& receiver_entity, AttachedEffectState& attached_effect_state)
    const
{
    const EntityID receiver_id = receiver_entity.GetID();

    LogDebug(receiver_id, "AttachedEffectsSystem::TimeStepBlink - {}", attached_effect_state);

    if (attached_effect_state.effect_data.type_id.type != EffectType::kBlink)
    {
        return;
    }
    if (attached_effect_state.state == AttachedEffectStateType::kWaiting)
    {
        return;
    }

    const EffectData& effect = attached_effect_state.effect_data;
    const bool is_activation_time_step =
        attached_effect_state.current_time_steps == attached_effect_state.blink_delay_time_steps;

    if (!is_activation_time_step)
    {
        return;
    }

    // Can't apply as the target does not have this component
    if (!receiver_entity.Has<PositionComponent>())
    {
        return;
    }

    // Get position component
    auto& receiver_position_component = receiver_entity.Get<PositionComponent>();

    // Try for a reserved position
    HexGridPosition new_position = receiver_position_component.GetReservedPosition();

    // If we don't have a reserved position, create one
    if (new_position == kInvalidHexHexGridPosition)
    {
        const auto& abilities_component = receiver_entity.Get<AbilitiesComponent>();

        // Get predicted targets, if possible
        std::set<EntityID> predicted_targets;
        auto active_ability = abilities_component.GetActiveAbility();
        if (active_ability != nullptr)
        {
            predicted_targets = active_ability->GetCurrentSkillState().targeting_state.available_targets;
        }
        else if (effect.blink_target != ReservedPositionType::kAcross)
        {
            LogWarn(
                attached_effect_state.combat_unit_sender_id,
                "- receiver = {} no active ability for BLINK targeting",
                receiver_id);
            return;
        }

        const GridHelper& grid_helper = world_->GetGridHelper();
        grid_helper.TryToReservePosition(receiver_entity, predicted_targets, effect.blink_target);
        new_position = receiver_position_component.GetReservedPosition();
    }

    // Validate final blink position
    if (new_position == kInvalidHexHexGridPosition)
    {
        LogWarn(
            attached_effect_state.combat_unit_sender_id,
            "- receiver = {} no valid BLINK target position",
            receiver_id);
        return;
    }

    // Reserve old position before blink if needed
    if (effect.blink_reserve_previous_position)
    {
        receiver_position_component.SetReservedPosition(receiver_position_component.GetPosition());
    }

    // Move entity
    receiver_position_component.SetPosition(new_position);
    LogDebug(
        attached_effect_state.combat_unit_sender_id,
        "- receiver = {} BLINKED to position = {}",
        receiver_id,
        new_position);

    // Predict the new focus
    EntityID predicted_focus_id = kInvalidEntityID;
    const auto predicted_focus =
        world_->GetSystem<FocusSystem>().ChooseFocus(receiver_entity, std::unordered_set<EntityID>{});
    if (predicted_focus)
    {
        predicted_focus_id = predicted_focus->GetID();
    }

    // Send event
    world_->BuildAndEmitEvent<EventType::kBlinked>(receiver_id, new_position, predicted_focus_id);
    world_->BuildAndEmitEvent<EventType::kMoved>(receiver_id, new_position);
}

}  // namespace simulation
