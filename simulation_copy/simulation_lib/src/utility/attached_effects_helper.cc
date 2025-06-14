#include "attached_effects_helper.h"

#include "components/attached_effects_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "data/zone_data.h"
#include "ecs/event_types_data.h"
#include "factories/entity_factory.h"
#include "utility/effect_package_helper.h"
#include "utility/enum.h"
#include "utility/vector_helper.h"

namespace simulation
{
AttachedEffectsHelper::AttachedEffectsHelper(World* world) : world_(world) {}

std::shared_ptr<Logger> AttachedEffectsHelper::GetLogger() const
{
    return world_->GetLogger();
}

void AttachedEffectsHelper::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int AttachedEffectsHelper::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

void AttachedEffectsHelper::AddRootAttachedEffect(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect) const
{
    // Check for misuse
    const EntityID receiver_id = receiver_entity.GetID();
    if (!attached_effect->IsRootType())
    {
        assert(false);
        LogErr(receiver_id, "AttachedEffectsHelper::AddRootAttachedEffect - NOT A ROOT TYPE: {}", *attached_effect);
        return;
    }

    LogDebug(receiver_id, "AttachedEffectsHelper::AddRootAttachedEffect - {}", *attached_effect);

    // Keep track of this
    KeepTrackOfAttachedEffect(receiver_entity, attached_effect);

    if (attached_effect->effect_data.HasStaticFrequency() && !attached_effect->captured_effect_value.has_value())
    {
        attached_effect->CaptureEffectValue(*world_, receiver_id);
    }

    // Do a process overlap pass
    const EffectTypeID& effect_type_id = attached_effect->effect_data.type_id;
    ProcessAllAttachedEffectsOfTypeID(receiver_entity, effect_type_id, false);
}

void AttachedEffectsHelper::AddChildAttachedEffect(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& parent,
    const AttachedEffectStatePtr& child) const
{
    // Check for misuse
    const EntityID receiver_id = receiver_entity.GetID();
    if (!parent->IsRootType() || !child->IsRootType() || !child->children.empty() || child == parent)
    {
        assert(false);
        LogErr(
            receiver_id,
            "AttachedEffectsHelper::AddChildAttachedEffect - MISUSED, parent & child must be a root type (and "
            "different), and child must not have any children of its own. parent: {}, child: {}",
            *parent,
            *child);
        return;
    }

    LogDebug(receiver_id, "AttachedEffectsHelper::AddChildAttachedEffect - parent: {}, child: {}", *parent, *child);

    // Keep track of child <-> parent connection
    parent->AddChild(child);

    // Keep track of child lifetime
    KeepTrackOfAttachedEffect(receiver_entity, child);
}

void AttachedEffectsHelper::KeepTrackOfAttachedEffect(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect) const
{
    assert(attached_effect);
    auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();

    // Set the true combat unit parent of sender_id
    attached_effect->combat_unit_sender_id = world_->GetCombatUnitParentID(attached_effect->sender_id);

    // attached_effects_ tracks all effects and instead of state the state of the highest effect is
    // Sent Add to tracked effects
    attached_effects_component.attached_effects_.emplace_back(attached_effect);
}

void AttachedEffectsHelper::RemoveAttachedEffect(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect,
    const int delay_time_steps) const
{
    // Effect is already destroyed, don't do anything
    if (attached_effect->state == AttachedEffectStateType::kDestroyed)
    {
        return;
    }

    const EffectTypeID& effect_type_id = attached_effect->effect_data.type_id;
    const EntityID receiver_id = receiver_entity.GetID();

    LogDebug(receiver_id, "AttachedEffectsHelper::RemoveAttachedEffect - {}", *attached_effect);

    // Mark destroyed. AttachedEffectsSystem will remove it from an array on PostTimeStep
    attached_effect->state = AttachedEffectStateType::kDestroyed;
    if (delay_time_steps > 0)
    {
        attached_effect->time_step_when_destroy = world_->GetTimeStepCount() + delay_time_steps;
    }

    // Remove from active
    RemoveAttachedEffectFromActive(receiver_entity, attached_effect, false);

    // Send event
    world_->BuildAndEmitEvent<EventType::kOnAttachedEffectRemoved>(
        attached_effect->combat_unit_sender_id == kInvalidEntityID ? attached_effect->sender_id
                                                                   : attached_effect->combat_unit_sender_id,
        receiver_id,
        attached_effect->effect_data,
        attached_effect->effect_state);

    // Remove children if it has any
    RemoveAllChildrenFromAttachedEffect(receiver_entity, attached_effect, delay_time_steps);

    if (EnableExtraLogs())
    {
        LogDebug(receiver_id, "AttachedEffectsHelper::RemoveAttachedEffect - END");
    }

    // Do a process pass over the existing effects
    ProcessAllAttachedEffectsOfTypeID(receiver_entity, effect_type_id, true);
}

void AttachedEffectsHelper::RemoveAllChildrenFromAttachedEffect(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect,
    const int delay_time_steps) const
{
    // Remove children if it has any
    if (attached_effect->children.empty())
    {
        return;
    }

    LogDebug(
        receiver_entity.GetID(),
        "AttachedEffectsHelper::RemoveAllChildrenFromAttachedEffect - Removing {} children from effect: {}",
        attached_effect->children.size(),
        *attached_effect);

    for (const auto& child : attached_effect->children)
    {
        RemoveAttachedEffect(receiver_entity, child, delay_time_steps);
    }
    attached_effect->children.clear();
}

void AttachedEffectsHelper::AddAttachedEffectToActive(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect,
    const bool do_buffs_adjustments) const
{
    auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();
    const EntityID receiver_id = receiver_entity.GetID();
    const EffectData& effect_data = attached_effect->effect_data;
    const EffectTypeID& effect_type_id = effect_data.type_id;

    // TODO(vampy): Do we need this special case for hyper?
    // If the effect is able to be stacked then return early
    // if (effect_data.is_always_stackable || effect_state.source_context.Has(SourceContextType::kHyper) ||
    //     effect_data.lifetime.is_consumable)
    // {
    //     if (EnableExtraLogs())
    //     {
    //         LogDebug(
    //             receiver_id,
    //             "AttachedEffectsHelper::OnAddedStackingCheck - ADDED, skipped stacking check - {}",
    //             *attached_effect);
    //     }

    //     return;
    // }

    // Already exists, should this be happening?
    if (attached_effects_component.IsAttachedEffectInActive(attached_effect))
    {
        // NOTE: This should be normal, as the ProcessAttachedEffects iterates
        if (EnableExtraLogs())
        {
            LogDebug(
                receiver_id,
                "AttachedEffectsHelper::AddAttachedEffectToActive - ALREADY EXISTS IN ACTIVE: {}",
                *attached_effect);
        }

        // Do special adjustment for buffs/debuffs, even if marked already in active
        if (do_buffs_adjustments)
        {
            AdjustFromBuffsDebuffs(receiver_entity, *attached_effect);
        }
        return;
    }
    if (EnableExtraLogs())
    {
        LogDebug(receiver_id, "AttachedEffectsHelper::AddAttachedEffectToActive - {}", *attached_effect);
    }

    // Mark state
    attached_effect->state = AttachedEffectStateType::kPendingActivation;

    // PRE ADD - Handle some special types
    switch (effect_type_id.type)
    {
    case EffectType::kPlaneChange:
    {
        // Remove all previous plane changes
        std::vector<AttachedEffectStatePtr> effects_to_remove;
        for (const auto& [plane_change_type, effects] : attached_effects_component.active_plane_changes_)
        {
            VectorHelper::AppendVector(effects_to_remove, effects);
        }

        if (EnableExtraLogs() && !effects_to_remove.empty())
        {
            LogDebug(
                receiver_id,
                "| Remove existing Plane changes, effects_to_remove.size() = {}",
                effects_to_remove.size());
        }

        // Remove these attached effects
        for (const auto& old_effect : effects_to_remove)
        {
            RemoveAttachedEffect(receiver_entity, old_effect);
        }
        break;
    }
    default:
        break;
    };

    // Add to active
    attached_effects_component.AddAttachedEffectToActive(attached_effect);

    // Do special adjustment for buffs/debuffs
    if (do_buffs_adjustments)
    {
        AdjustFromBuffsDebuffs(receiver_entity, *attached_effect);
    }

    // Send event to UI
    world_->BuildAndEmitEvent<EventType::kOnAttachedEffectApplied>(
        attached_effect->combat_unit_sender_id == kInvalidEntityID ? attached_effect->sender_id
                                                                   : attached_effect->combat_unit_sender_id,
        receiver_id,
        attached_effect->effect_data,
        attached_effect->effect_state);
}

void AttachedEffectsHelper::RemoveAttachedEffectFromActive(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect,
    const bool remove_children) const
{
    auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();
    const EntityID receiver_id = receiver_entity.GetID();

    if (EnableExtraLogs())
    {
        LogDebug(receiver_id, "AttachedEffectsHelper::RemoveAttachedEffectFromActive - {}", *attached_effect);
    }

    // Remove from component
    attached_effects_component.RemoveAttachedEffectFromActive(attached_effect);

    // Remove children
    if (remove_children)
    {
        for (const auto& child : attached_effect->children)
        {
            RemoveAttachedEffectFromActive(receiver_entity, child, remove_children);
        }
    }

    // Undo adjustment for buff/debuffs
    UndoAdjustFromBuffsDebuffs(receiver_entity, *attached_effect);
}

void AttachedEffectsHelper::ProcessAllAttachedEffectsOfTypeID(
    const Entity& receiver_entity,
    const EffectTypeID& effect_type_id,
    const bool from_remove) const
{
    auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();
    const EntityID receiver_id = receiver_entity.GetID();

    // Nothing to do just return
    if (attached_effects_component.attached_effects_.empty())
    {
        LogDebug(
            receiver_id,
            "AttachedEffectsHelper::ProcessAllAttachedEffectsOfTypeID: no attached effects in component. returning");
        return;
    }

    // Group in effects with the same ability and effects
    std::map<std::string_view, std::vector<AttachedEffectStatePtr>> same_abilities_map;
    std::vector<AttachedEffectStatePtr> different_abilities;
    const size_t total_count_effects = attached_effects_component.GetAllRootEffectsOfTypeIDPerAbilityName(
        effect_type_id,
        &same_abilities_map,
        &different_abilities);

    if (EnableExtraLogs())
    {
        LogDebug(
            receiver_id,
            "AttachedEffectsHelper::ProcessAllAttachedEffectsOfTypeID - effect_type_id: {}, "
            "same_abilities_map.size() = {}, different_abilities.size() = {}, total_count_effects = {}",
            effect_type_id,
            same_abilities_map.size(),
            different_abilities.size(),
            total_count_effects);
    }

    // Find out the overlap type for these
    const EffectOverlapProcessType default_same_ability_overlap_type = GetDefaultOverlapType(effect_type_id.type, true);
    const EffectOverlapProcessType default_different_ability_overlap_type =
        GetDefaultOverlapType(effect_type_id.type, false);

    // Check if the processing for the same ability type was overriden for each ability name
    // See https://illuvium.atlassian.net/wiki/spaces/AB/pages/72417779/Effect+Overlap+Processing#Default-Processing
    std::unordered_map<std::string_view, EffectOverlapProcessType> same_abilities_overlap_type_map;
    for (const auto& [ability_name, attached_effects] : same_abilities_map)
    {
        // Initialize with the default value
        same_abilities_overlap_type_map[ability_name] = default_same_ability_overlap_type;

        // Check if all have any different
        std::unordered_set<EffectOverlapProcessType> seen_overriden_overlap_types;
        for (const auto& attached_effect : attached_effects)
        {
            seen_overriden_overlap_types.insert(attached_effect->effect_data.lifetime.overlap_process_type);
        }

        // If only one value is seen (and set) in all the effect types (for this ability name) then we use that for the
        // override
        if (seen_overriden_overlap_types.size() == 1)
        {
            // Get the only element in the set
            const EffectOverlapProcessType overriden_overlap_type = *seen_overriden_overlap_types.begin();
            if (overriden_overlap_type != EffectOverlapProcessType::kNone)
            {
                same_abilities_overlap_type_map[ability_name] = overriden_overlap_type;
            }
        }
    }

    // Process Same abilities
    for (const auto& [ability_name, attached_effects] : same_abilities_map)
    {
        // TODO(vampy): This should never happen (it should be impossible), but for some reason it happens (here be
        // dragons). See Issue https://illuvium.atlassian.net/browse/AB-4821.
        if (same_abilities_overlap_type_map.count(ability_name) == 0)
        {
            LogErr(
                receiver_id,
                "AttachedEffectsHelper::ProcessAllAttachedEffectsOfTypeID - effect_type_id: {}, "
                "ability_name = {} DOES NOT EXIST (IMPOSSIBLE)",
                effect_type_id,
                ability_name);
            continue;
        }

        const EffectOverlapProcessType process_type = same_abilities_overlap_type_map.at(ability_name);
        ProcessAttachedEffects(receiver_entity, process_type, attached_effects, from_remove);
    }

    // Process different abilities, always use the default
    ProcessAttachedEffects(receiver_entity, default_different_ability_overlap_type, different_abilities, from_remove);
}

void AttachedEffectsHelper::KeepAttachedEffectInWaitingAndRemoveFromActive(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect) const
{
    // Already in waiting
    if (attached_effect->state == AttachedEffectStateType::kWaiting)
    {
        return;
    }

    // Keep in waiting
    attached_effect->state = AttachedEffectStateType::kWaiting;

    // Remove from active
    RemoveAttachedEffectFromActive(receiver_entity, attached_effect, true);
}

void AttachedEffectsHelper::ProcessAttachedEffects(
    const Entity& receiver_entity,
    const EffectOverlapProcessType process_type,
    const std::vector<AttachedEffectStatePtr>& attached_effects,
    const bool from_remove) const
{
    // Nothing to process
    if (attached_effects.empty())
    {
        return;
    }

    const auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();
    const EntityID receiver_id = receiver_entity.GetID();

    if (EnableExtraLogs())
    {
        LogDebug(
            receiver_id,
            "AttachedEffectsHelper::ProcessAttachedEffects - process_type = `{}`, attached_effects.size() = {}",
            process_type,
            attached_effects.size());
    }

    switch (process_type)
    {
    case EffectOverlapProcessType::kHighest:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/280297473/Highest+Processing
        // Figure out the highest value one and only activate that, the rest remove them from active
        const auto& highest_attached_effect = GetHighestValueAttachedEffect(receiver_entity, attached_effects);
        assert(highest_attached_effect);

        if (EnableExtraLogs())
        {
            LogDebug(receiver_id, "| FOUND highest_attached_effect: {}", *highest_attached_effect);
        }

        // Mark highest as active
        if (!attached_effects_component.IsAttachedEffectInActive(highest_attached_effect))
        {
            // If the highest is not yet active, activate it and do any buff adjustments
            static constexpr bool do_buffs_adjustments = true;
            AddAttachedEffectToActive(receiver_entity, highest_attached_effect, do_buffs_adjustments);
        }

        // Mark the rest as in waiting and remove from active
        for (const auto& attached_effect : attached_effects)
        {
            // Everything except highest
            if (attached_effect == highest_attached_effect)
            {
                continue;
            }

            KeepAttachedEffectInWaitingAndRemoveFromActive(receiver_entity, attached_effect);
        }
        break;
    }
    case EffectOverlapProcessType::kMerge:
    {
        // We don't need to process anything on removal, as there exists only one
        if (from_remove)
        {
            break;
        }

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/280330315/Merge+Processing
        // Just activate current as there is nothing to merge
        if (attached_effects.size() == 1)
        {
            // NOTE: This is safe because merged_attached_effect will always be a new effect
            const auto& merged_attached_effect = attached_effects[0];
            if (!attached_effects_component.IsAttachedEffectInActive(merged_attached_effect))
            {
                static constexpr bool do_buffs_adjustments = true;
                AddAttachedEffectToActive(receiver_entity, merged_attached_effect, do_buffs_adjustments);
            }
            break;
        }

        // Create new merged effect
        const auto merged_attached_effect = MergeAllAttachedEffects(receiver_entity, attached_effects);
        assert(merged_attached_effect);

        if (EnableExtraLogs())
        {
            LogDebug(receiver_id, "| Created merged_attached_effect: {}", *merged_attached_effect);
        }

        // Remove all the existing ones as they were merged inside the  merged_attached_effect
        // NOTE: This needs to be done before adding the new merged effect
        for (const auto& attached_effect : attached_effects)
        {
            RemoveAttachedEffect(receiver_entity, attached_effect);
        }

        // Add this merged effect
        AddAttachedEffect(receiver_entity, merged_attached_effect);
        break;
    }
    case EffectOverlapProcessType::kStacking:
    {
        // We don't need to process anything on removal, as there exists only one stack value per group
        if (from_remove)
        {
            break;
        }

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/280330241/Stacking+Processing
        // Stack all the values
        bool did_value_change = false;
        const auto stack_attached_effect =
            StackAllAttachedEffects(receiver_entity, attached_effects, &did_value_change);
        assert(stack_attached_effect);
        if (EnableExtraLogs())
        {
            LogDebug(receiver_id, "| stack_attached_effect: {}", *stack_attached_effect);
        }

        // Mark stack as active
        const bool do_buffs_adjustments =
            !attached_effects_component.IsAttachedEffectInActive(stack_attached_effect) || did_value_change;
        AddAttachedEffectToActive(receiver_entity, stack_attached_effect, do_buffs_adjustments);

        // Remove the rest of the attached effects coming in
        if (EnableExtraLogs())
        {
            LogDebug(
                receiver_id,
                "| Removing the rest (attached_effects.size() = {}) that are NOT stack_attached_effect",
                attached_effects.size());
        }
        for (const auto& attached_effect : attached_effects)
        {
            // Everything except our stacked one
            if (attached_effect == stack_attached_effect)
            {
                continue;
            }

            // Remove from active
            RemoveAttachedEffect(receiver_entity, attached_effect);
        }
        if (EnableExtraLogs())
        {
            LogDebug(
                receiver_id,
                "| END  Removing the rest (attached_effects.size() = {}) that are NOT stack_attached_effect",
                attached_effects.size());
        }

        break;
    }
    case EffectOverlapProcessType::kCondition:
    {
        // We don't need to process anything on removal, as there exists only one stack value per group
        if (from_remove)
        {
            break;
        }

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/280330257/Condition+Processing
        const auto stack_attached_effect = StackAllConditionAttachedEffects(receiver_entity, attached_effects);
        assert(stack_attached_effect);

        // Remove the rest of the conditions coming in
        if (EnableExtraLogs())
        {
            LogDebug(receiver_id, "stack_attached_effect: {}", *stack_attached_effect);
            LogDebug(
                receiver_id,
                "| Removing the rest (attached_effects.size() = {}) that are NOT stack_attached_effect",
                attached_effects.size());
        }
        for (const auto& attached_effect : attached_effects)
        {
            // Everything except our stacked one
            if (attached_effect == stack_attached_effect)
            {
                continue;
            }

            // Remove from active
            RemoveAttachedEffect(receiver_entity, attached_effect);
        }
        if (EnableExtraLogs())
        {
            LogDebug(
                receiver_id,
                "| END  Removing the rest (attached_effects.size() = {}) that are NOT stack_attached_effect",
                attached_effects.size());
        }

        // Sync the condition with the current stacks
        SyncConditionWithCurrentCurrentStacks(receiver_entity, stack_attached_effect);
        break;
    }
    case EffectOverlapProcessType::kSum:
    default:
    {
        // Default and sum are similar
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/280166641/Sum+Processing
        // Add all of them as separate, with separate duration
        for (const auto& attached_effect : attached_effects)
        {
            if (attached_effects_component.IsAttachedEffectInActive(attached_effect))
            {
                continue;
            }

            // For sum it is ok to always do them if the effect is not already activated yet
            static constexpr bool do_buffs_adjustments = true;
            AddAttachedEffectToActive(receiver_entity, attached_effect, do_buffs_adjustments);
        }
        break;
    }
    }
}

bool AttachedEffectsHelper::EnableExtraLogs() const
{
    return world_->GetLogsConfig().enable_attached_effects;
}

void AttachedEffectsHelper::SyncConditionWithCurrentCurrentStacks(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& condition_root_effect) const
{
    // Condition already marked as destroyed, nothing to do
    if (!condition_root_effect->IsValid())
    {
        return;
    }

    if (EnableExtraLogs())
    {
        LogDebug(
            receiver_entity.GetID(),
            "AttachedEffectsHelper::SyncConditionWithCurrentCurrentStacks - condition_root_effect: {}",
            *condition_root_effect);
    }

    auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();
    EffectData& condition_root_effect_data = condition_root_effect->effect_data;
    const EffectTypeID& condition_effect_type_id = condition_root_effect_data.type_id;
    const WorldEffectsConfig& effects_config = world_->GetWorldEffectsConfig();

    // Remove exising children?
    RemoveAllChildrenFromAttachedEffect(receiver_entity, condition_root_effect, 0);

    // Keep track of stack values
    const int stacks_count = condition_root_effect->current_stacks_count;

    // Add new children with updated current stacks
    const WorldEffectConditionConfig& dot_config =
        effects_config.GetConditionType(condition_effect_type_id.condition_type);

    // Set lifetime of root from config
    condition_root_effect_data.lifetime.duration_time_ms = dot_config.duration_ms;
    condition_root_effect_data.lifetime.max_stacks = dot_config.max_stacks;
    condition_root_effect_data.lifetime.cleanse_at_max_stacks = dot_config.cleanse_on_max_stacks;

    // Set the correct predefined time
    EffectExpression dot_expression_without_stacks;
    switch (condition_effect_type_id.condition_type)
    {
    case EffectConditionType::kPoison:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44368654/Poison
        // Create DOT expression
        dot_expression_without_stacks = EffectExpression::FromStatHighPrecisionPercentage(
            dot_config.dot_high_precision_percentage,
            StatType::kMaxHealth,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);
        break;
    }
    case EffectConditionType::kWound:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44336079/Wound
        // Create OOT Expressions
        // Set dot_amount of the effect package expression (the root condition effect package expression)
        dot_expression_without_stacks.operation_type = EffectOperationType::kHighPrecisionPercentageOf;
        dot_expression_without_stacks.operands = {
            EffectExpression::FromValue(dot_config.dot_high_precision_percentage),
            condition_root_effect_data.GetExpression()};
        break;
    }
    case EffectConditionType::kBurn:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44368692/Burn
        // Create DOT expression
        // Calculate missing health = MaxHealth - CurrentHealth
        EffectExpression expression_missing_health;
        expression_missing_health.operation_type = EffectOperationType::kSubtract;
        expression_missing_health.operands = {
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromStat(
                StatType::kCurrentHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver)};

        // 2% of missing Health
        dot_expression_without_stacks.operation_type = EffectOperationType::kHighPrecisionPercentageOf;
        dot_expression_without_stacks.operands = {
            EffectExpression::FromValue(dot_config.dot_high_precision_percentage),
            expression_missing_health};
        break;
    }
    case EffectConditionType::kFrost:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/204243633/Frost
        // It's just debuff.
        break;
    }
    default:
        break;
    }

    // Create the dot attached effects
    std::vector<AttachedEffectStatePtr> dot_children;
    if (dot_config.dot_high_precision_percentage > 0_fp)
    {
        assert(!dot_expression_without_stacks.IsEmpty());

        // Add in stacks
        const EffectExpression dot_expression =
            CreateStacksMultiplierExpression(dot_expression_without_stacks, stacks_count);

        // Create DOT effect
        const auto dot_effect = EffectData::CreateDamageOverTime(
            dot_config.dot_damage_type,
            dot_expression,
            dot_config.duration_ms,
            dot_config.frequency_time_ms);
        const auto dot_attached_effect = AttachedEffectState::Create(
            condition_root_effect->combat_unit_sender_id,
            dot_effect,
            condition_root_effect->effect_state);

        dot_children.push_back(dot_attached_effect);
    }
    if (dot_config.debuff_percentage > 0_fp)
    {
        assert(dot_config.debuff_stat_type != StatType::kNone);
        EffectExpression debuff_expression_without_stacks = EffectExpression::FromStatPercentage(
            dot_config.debuff_percentage,
            dot_config.debuff_stat_type,
            StatEvaluationType::kBase,
            ExpressionDataSourceType::kReceiver);

        // Add in stacks
        const EffectExpression debuff_expression =
            CreateStacksMultiplierExpression(debuff_expression_without_stacks, stacks_count);

        // Create Debuff effect
        const auto debuff_effect =
            EffectData::CreateDebuff(dot_config.debuff_stat_type, debuff_expression, dot_config.duration_ms);
        const auto debuff_attached_effect = AttachedEffectState::Create(
            condition_root_effect->combat_unit_sender_id,
            debuff_effect,
            condition_root_effect->effect_state);

        dot_children.push_back(debuff_attached_effect);
    }

    // Add root to active
    static constexpr bool do_buffs_adjustments = true;
    if (attached_effects_component.IsAttachedEffectInActive(condition_root_effect))
    {
        // Reset current duration
        condition_root_effect->ResetCurrentLifetime();
    }
    else
    {
        // First time adding root
        condition_root_effect->UpdateCachedValues();
        condition_root_effect->ResetCurrentLifetime();
        AddAttachedEffectToActive(receiver_entity, condition_root_effect, do_buffs_adjustments);
    }

    // Add Children to root
    for (const auto& child : dot_children)
    {
        AddChildAttachedEffect(receiver_entity, condition_root_effect, child);
    }

    // Activate the children
    for (const auto& child : dot_children)
    {
        AddAttachedEffectToActive(receiver_entity, child, do_buffs_adjustments);
    }
}

EffectOverlapProcessType AttachedEffectsHelper::GetDefaultOverlapType(
    const EffectType effect_type,
    const bool same_ability) const
{
    // Defaults taken from
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/72417779/Effect+Overlap+Processing
    switch (effect_type)
    {
    // Combat Stat Modifier
    case EffectType::kBuff:
    case EffectType::kDebuff:
        return same_ability ? EffectOverlapProcessType::kHighest : EffectOverlapProcessType::kSum;

    // State Change/Forced Action and Displacement
    // TODO(vampy): Separate Forced Action from the Negative state
    case EffectType::kNegativeState:
    case EffectType::kPositiveState:
    case EffectType::kDisplacement:
        return EffectOverlapProcessType::kMerge;

    // Condition
    case EffectType::kCondition:
        return EffectOverlapProcessType::kCondition;

    // Effect Package Modifier
    case EffectType::kEmpower:
    case EffectType::kDisempower:
        // TODO(vampy): Either make Merge work properly with Empower/Disempower or just don't merge here
        // We area already doing a merger at the ability level, see
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower
        return EffectOverlapProcessType::kNone;
        // return same_ability ? EffectOverlapProcessType::kMerge : EffectOverlapProcessType::kNone;

    // Metered Combat Stat Change Over Time
    case EffectType::kHealOverTime:
    case EffectType::kDamageOverTime:
    case EffectType::kEnergyGainOverTime:
    case EffectType::kEnergyBurnOverTime:
    case EffectType::kHyperBurnOverTime:
    case EffectType::kHyperGainOverTime:
        return same_ability ? EffectOverlapProcessType::kMerge : EffectOverlapProcessType::kNone;

    // TODO(vampy): Use these
    case EffectType::kSpawnShield:
        return EffectOverlapProcessType::kShield;
    case EffectType::kSpawnMark:
        return EffectOverlapProcessType::kMerge;

    default:
        return EffectOverlapProcessType::kNone;
    }
}

bool AttachedEffectsHelper::GetEffectComparisonValue(
    const EffectData& effect_data,
    const ExpressionEvaluationContext& context,
    const ExpressionStatsSource& stats_source,
    FixedPoint* out_comparison_value) const
{
    assert(out_comparison_value != nullptr);

    if (effect_data.UsesExpression())
    {
        *out_comparison_value = effect_data.GetExpression().Evaluate(context, stats_source);
        return true;
    }
    else if (effect_data.UsesDurationTime())
    {
        *out_comparison_value = FixedPoint::FromInt(effect_data.lifetime.duration_time_ms);
        return true;
    }

    return false;
}

AttachedEffectStatePtr AttachedEffectsHelper::GetHighestValueAttachedEffect(
    const Entity& receiver_entity,
    const std::vector<AttachedEffectStatePtr>& attached_effects) const
{
    // There exists no max
    if (attached_effects.empty())
    {
        return nullptr;
    }

    const EntityID receiver_id = receiver_entity.GetID();
    if (EnableExtraLogs())
    {
        LogDebug(receiver_id, "AttachedEffectsHelper::GetHighestValueAttachedEffect");
    }

    ExpressionEvaluationContext expression_context(world_, kInvalidEntityID, receiver_id);
    ExpressionStatsSource stats_source =
        expression_context.MakeStatsSource(MakeEnumSet(ExpressionDataSourceType::kReceiver));

    // Used for sanity checking
    EffectTypeID previous_type_id = attached_effects[0]->effect_data.type_id;

    // Finds the maximum attached effect (in waiting) of the same type as the removed effect
    size_t max_index = kInvalidIndex;
    FixedPoint max_value = FixedPoint::FromInt(std::numeric_limits<int>::min());
    for (size_t index = 0; index < attached_effects.size(); index++)
    {
        const AttachedEffectStatePtr& attached_effect = attached_effects[index];
        const EffectData& effect_data = attached_effect->effect_data;

        const EnumSet<ExpressionDataSourceType>& required_sources = effect_data.GetRequiredDataSourceTypes();
        if (required_sources.Contains(ExpressionDataSourceType::kSender))
        {
            expression_context.UpdateSenderAndStatsSource(attached_effect->combat_unit_sender_id, stats_source);
        }

        if (required_sources.Contains(ExpressionDataSourceType::kSenderFocus))
        {
            stats_source.Set(
                ExpressionDataSourceType::kSenderFocus,
                attached_effect->effect_state.sender_focus_stats,
                nullptr);
        }

        // Sanity check all effects are the same type id
        if (previous_type_id != effect_data.type_id)
        {
            LogErr(
                receiver_id,
                "| FOUND TYPE ID different:{} for "
                "attached_effect: {}",
                previous_type_id,
                *attached_effect);
            continue;
        }

        // Get the comparison value
        FixedPoint comparison_value = 0_fp;
        if (!GetEffectComparisonValue(effect_data, expression_context, stats_source, &comparison_value))
        {
            LogErr(
                receiver_id,
                "| HAS NOT ACTIVE STACKING COMPARISON VALUE: "
                "{}",
                *attached_effect);
            continue;
        }

        // Found a better value
        if (comparison_value > max_value)
        {
            max_index = index;
            max_value = comparison_value;
        }

        // Store the previous
        previous_type_id = effect_data.type_id;
    }

    // Did not find anything
    if (max_index == kInvalidIndex)
    {
        return nullptr;
    }

    // Found a max
    auto& max_attached_effect = attached_effects.at(max_index);
    if (EnableExtraLogs())
    {
        LogDebug(receiver_id, "| FOUND max: {}", *max_attached_effect);
    }

    return max_attached_effect;
}

AttachedEffectStatePtr AttachedEffectsHelper::MergeAllAttachedEffects(
    const Entity& receiver_entity,
    const std::vector<AttachedEffectStatePtr>& attached_effects) const
{
    // Can't merge
    if (attached_effects.empty())
    {
        return nullptr;
    }

    // Just one effect, nothing to merge
    if (attached_effects.size() == 1)
    {
        return attached_effects[0];
    }

    const EntityID receiver_id = receiver_entity.GetID();

    // Keep data from the first attached effect as the primary
    const auto& first_attached_effect = attached_effects[0];
    const EntityID merge_sender_id = first_attached_effect->combat_unit_sender_id;
    EffectData merged_effect_data = first_attached_effect->effect_data;
    EffectState merged_effect_state = first_attached_effect->effect_state;

    // Set the duration of final effect to zero to not repeat the logic of choosing the best effect here
    // as it is implemented in MergeLifetimeOfEffectsData
    if (merged_effect_data.UsesDurationTime())
    {
        merged_effect_data.lifetime.duration_time_ms = 0;
    }
    if (merged_effect_data.lifetime.is_consumable &&
        merged_effect_data.lifetime.activations_until_expiry != kTimeInfinite)
    {
        merged_effect_data.lifetime.activations_until_expiry = 0;
    }
    MergeLifetimeOfEffectsData(receiver_entity, *first_attached_effect, merged_effect_data);

    // Calculate the expression of the current one
    ExpressionEvaluationContext expression_context(world_, merge_sender_id, receiver_id);

    // Add receiver stats even if they are not required by the current effect
    // they will most likely be required by others
    ExpressionStatsSource stats_source = expression_context.MakeStatsSource(
        merged_effect_data.GetRequiredDataSourceTypes() + MakeEnumSet(ExpressionDataSourceType::kReceiver));

    const bool with_captured_value = first_attached_effect->effect_data.type_id.UsesCapturedValue();
    auto get_effect_value = [&](const AttachedEffectState& state)
    {
        // Buffs/debuffs evaluate their value on application
        if (with_captured_value) return state.GetCapturedValue();

        return state.effect_data.GetExpression().Evaluate(expression_context, stats_source);
    };

    FixedPoint merged_expression_value = get_effect_value(*first_attached_effect);

    // Used for sanity checking
    EffectTypeID previous_type_id = merged_effect_data.type_id;

    for (size_t index = 1; index < attached_effects.size(); index++)
    {
        const AttachedEffectStatePtr& attached_effect = attached_effects[index];
        const EffectData& effect_data = attached_effect->effect_data;
        const EffectState& effect_state = attached_effect->effect_state;

        // Sanity check all effects are the same type id
        if (previous_type_id != effect_data.type_id)
        {
            assert(false);
            LogErr(
                receiver_id,
                "AttachedEffectsHelper::MergeAllAttachedEffects - FOUND TYPE ID different:{} for "
                "attached_effect: {}",
                previous_type_id,
                *attached_effect);
            continue;
        }

        // Merge the effect package attributes
        merged_effect_state.effect_package_attributes += effect_state.effect_package_attributes;
        merged_effect_data.attached_effect_package_attributes += effect_data.attached_effect_package_attributes;
        merged_effect_data.is_used_as_global_effect_attribute =
            merged_effect_data.is_used_as_global_effect_attribute || effect_data.is_used_as_global_effect_attribute;

        // Merge attached effects
        for (const auto& child_effect_data : effect_data.attached_effects)
        {
            merged_effect_data.attached_effects.push_back(child_effect_data);
        }

        // Merge lifetime
        MergeLifetimeOfEffectsData(receiver_entity, *attached_effect, merged_effect_data);

        // Merge expression
        if (merged_effect_data.UsesExpression())
        {
            if (effect_data.GetRequiredDataSourceTypes().Contains(ExpressionDataSourceType::kSender))
            {
                expression_context.UpdateSenderAndStatsSource(attached_effect->combat_unit_sender_id, stats_source);
            }

            // Calculate current effect value
            const FixedPoint expression_value = get_effect_value(*attached_effect);

            // Found higher
            if (expression_value > merged_expression_value)
            {
                merged_expression_value = expression_value;
            }
        }

        // Store the previous
        previous_type_id = effect_data.type_id;
    }

    // Convert expression to a flat value
    merged_effect_data.SetExpression(EffectExpression::FromValue(merged_expression_value));

    // Create the new merged effect
    auto merged_attached_effect = AttachedEffectState::Create(merge_sender_id, merged_effect_data, merged_effect_state);

    if (with_captured_value)
    {
        merged_attached_effect->captured_effect_value = merged_expression_value;
    }

    return merged_attached_effect;
}

AttachedEffectStatePtr AttachedEffectsHelper::StackAllAttachedEffects(
    const Entity& receiver_entity,
    const std::vector<AttachedEffectStatePtr>& attached_effects,
    bool* out_changed_value) const
{
    *out_changed_value = false;

    // Can't stack
    if (attached_effects.empty())
    {
        return nullptr;
    }

    const EntityID receiver_id = receiver_entity.GetID();

    if (EnableExtraLogs())
    {
        LogDebug(
            receiver_id,
            "AttachedEffectsHelper::StackAllAttachedEffects - attached_effects.size() = {}",
            attached_effects.size());
    }

    // Just one effect, nothing to stack
    if (attached_effects.size() == 1)
    {
        return attached_effects[0];
    }

    // We use the first effect as the one we merge all the effects coming in
    const auto& stack_attached_effect = attached_effects[0];

    // Keep track of stack values
    const int max_stacks = stack_attached_effect->effect_data.lifetime.max_stacks;

    // Add the first value to the history which is our first stack effect value itself
    const bool with_captured_value = stack_attached_effect->effect_data.type_id.UsesCapturedValue();

    auto get_effect_value = [&with_captured_value](
                                const AttachedEffectState& state,
                                const ExpressionEvaluationContext& context,
                                const ExpressionStatsSource& stats_source)
    {
        // Buffs/debuffs evaluate their value on application
        if (with_captured_value) return state.GetCapturedValue();

        return state.effect_data.GetExpression().Evaluate(context, stats_source);
    };
    ExpressionEvaluationContext expression_context(world_, stack_attached_effect->combat_unit_sender_id, receiver_id);
    ExpressionStatsSource stats_source = expression_context.MakeStatsSource(
        stack_attached_effect->effect_data.GetExpression().GatherRequiredDataSourceTypes() +
        MakeEnumSet(ExpressionDataSourceType::kReceiver));

    const FixedPoint initial_expression_value =
        get_effect_value(*stack_attached_effect, expression_context, stats_source);

    if (stack_attached_effect->current_stacks_count == 1 && stack_attached_effect->history_all_stack_values.empty())
    {
        stack_attached_effect->history_all_stack_values.push_back(initial_expression_value);

        if (EnableExtraLogs())
        {
            LogDebug(receiver_id, "| Pushing expression = {} to history_all_stack_values", initial_expression_value);
        }
    }

    // Used for sanity checking
    EffectTypeID previous_type_id = stack_attached_effect->effect_data.type_id;

    for (size_t index = 1; index < attached_effects.size(); index++)
    {
        const AttachedEffectStatePtr& attached_effect = attached_effects[index];
        const EffectData& effect_data = attached_effect->effect_data;

        // Sanity check all effects are the same type id
        if (previous_type_id != effect_data.type_id)
        {
            assert(false);
            LogErr(
                receiver_id,
                "| FOUND TYPE ID different:{} for "
                "attached_effect: {}",
                previous_type_id,
                *attached_effect);
            continue;
        }

        // Merge lifetime
        StackLifetimeOfEffectsData(receiver_entity, effect_data, stack_attached_effect->effect_data);

        // Increase stack count
        IncreaseStacksCount(receiver_entity, stack_attached_effect);

        // Calculate current effect value
        const EntityID sender_id = attached_effect->combat_unit_sender_id;
        ExpressionEvaluationContext current_expression_context = expression_context;
        ExpressionStatsSource current_stats_source = stats_source;
        if (effect_data.GetRequiredDataSourceTypes().Contains(ExpressionDataSourceType::kSender))
        {
            current_expression_context.UpdateSenderAndStatsSource(sender_id, current_stats_source);
        }

        const FixedPoint expression_value =
            get_effect_value(*attached_effect, current_expression_context, current_stats_source);
        stack_attached_effect->history_all_stack_values.push_back(expression_value);

        if (EnableExtraLogs())
        {
            LogDebug(receiver_id, "| Pushing expression = {} to history_all_stack_values", expression_value);
        }

        // Store the previous
        previous_type_id = effect_data.type_id;
    }

    // Update cached values on the entity

    // Sort the history of stack values to be always in descending order
    std::sort(
        stack_attached_effect->history_all_stack_values.begin(),
        stack_attached_effect->history_all_stack_values.end(),
        std::greater<>());

    // Update the current stacking expression value by taking the sum of the first max_stacks in the history
    FixedPoint stack_expression_value = 0_fp;
    const size_t history_all_stack_values_size = stack_attached_effect->history_all_stack_values.size();

    // Limit the stacks size to the history_all_stack_values_size as not to go out of bounds
    const size_t max_stacks_size =
        max_stacks == kTimeInfinite ? history_all_stack_values_size : static_cast<size_t>(max_stacks);
    for (size_t i = 0; i < max_stacks_size && i < history_all_stack_values_size; i++)
    {
        stack_expression_value += stack_attached_effect->history_all_stack_values[i];
    }

    // Update the value on the stacked effect
    bool use_value_expression = true;
    if (with_captured_value)
    {
        stack_attached_effect->captured_effect_value = stack_expression_value;
        if (!stack_attached_effect->effect_data.HasStaticFrequency())
        {
            // Dynamic effects will need stacked expression to update the value periodically
            EffectExpression expression{};
            expression.operation_type = EffectOperationType::kMultiply;
            expression.operands.push_back(
                EffectExpression::FromValue(FixedPoint::FromInt(static_cast<int>(history_all_stack_values_size))));
            expression.operands.push_back(stack_attached_effect->effect_data.GetExpression());
            stack_attached_effect->effect_data.SetExpression(std::move(expression));

            // To not override this expression below
            use_value_expression = false;
        }
    }

    if (use_value_expression)
    {
        stack_attached_effect->effect_data.SetExpression(EffectExpression::FromValue(stack_expression_value));
    }

    // Calculate the expression after so that we know if the value changed
    const FixedPoint after_expression_value =
        get_effect_value(*stack_attached_effect, expression_context, stats_source);

    stack_attached_effect->last_delta_value = after_expression_value - initial_expression_value;
    if (stack_attached_effect->last_delta_value != 0_fp)
    {
        *out_changed_value = true;
    }
    if (EnableExtraLogs())
    {
        LogDebug(
            receiver_id,
            "| initial_expression_value = {}, after_expression_value = {}",
            initial_expression_value,
            after_expression_value);
    }

    // Update the time step values and reset current timers
    stack_attached_effect->UpdateCachedValues();
    stack_attached_effect->ResetCurrentLifetime();

    return stack_attached_effect;
}

AttachedEffectStatePtr AttachedEffectsHelper::StackAllConditionAttachedEffects(
    const Entity& receiver_entity,
    const std::vector<AttachedEffectStatePtr>& attached_effects) const
{
    // Can't stack
    if (attached_effects.empty())
    {
        return nullptr;
    }

    // Just one effect, nothing to stack
    if (attached_effects.size() == 1)
    {
        return attached_effects[0];
    }

    const EntityID receiver_id = receiver_entity.GetID();

    // We use the first effect as the one we merge all the effects coming in
    const auto& stack_attached_effect = attached_effects[0];

    // Used for sanity checking
    EffectTypeID previous_type_id = stack_attached_effect->effect_data.type_id;

    // Stacks all the conditions, we just increase the stack count
    for (size_t index = 1; index < attached_effects.size(); index++)
    {
        const AttachedEffectStatePtr& attached_effect = attached_effects[index];
        const EffectData& effect_data = attached_effect->effect_data;

        // Sanity check all effects are the same type id
        if (previous_type_id != effect_data.type_id)
        {
            assert(false);
            LogErr(
                receiver_id,
                "AttachedEffectsHelper::StackAllConditionAttachedEffects - FOUND TYPE ID different:{} for "
                "attached_effect: {}",
                previous_type_id,
                *attached_effect);
            continue;
        }

        // Increase stack count
        IncreaseStacksCount(receiver_entity, stack_attached_effect);

        // Store the previous
        previous_type_id = effect_data.type_id;
    }

    return stack_attached_effect;
}

EffectExpression AttachedEffectsHelper::CreateStacksMultiplierExpression(const EffectExpression& lhs, const int stacks)
{
    EffectExpression expression;
    expression.operation_type = EffectOperationType::kMultiply;
    expression.operands = {lhs, EffectExpression::FromValue(FixedPoint::FromInt(stacks))};
    return expression;
}

void AttachedEffectsHelper::StackLifetimeOfEffectsData(const Entity&, const EffectData& src, EffectData& dst) const
{
    if (!dst.UsesDurationTime())
    {
        return;
    }

    // Find highest remaining duration
    dst.lifetime.duration_time_ms = std::max(dst.lifetime.duration_time_ms, src.lifetime.duration_time_ms);

    // Only if both consumable and activations until expiry
    if (dst.lifetime.is_consumable && src.lifetime.is_consumable &&
        dst.lifetime.activations_until_expiry != kTimeInfinite &&
        src.lifetime.activations_until_expiry != kTimeInfinite)
    {
        dst.lifetime.activations_until_expiry =
            std::max(dst.lifetime.activations_until_expiry, src.lifetime.activations_until_expiry);
    }
}

void AttachedEffectsHelper::MergeLifetimeOfEffectsData(
    const Entity& /* receiver_entity */,
    const AttachedEffectState& src,
    EffectData& dst) const
{
    if (!dst.UsesDurationTime())
    {
        return;
    }

    const auto& src_lifetime = src.effect_data.lifetime;

    if (src_lifetime.duration_time_ms == kTimeInfinite || dst.lifetime.duration_time_ms == kTimeInfinite)
    {
        dst.lifetime.duration_time_ms = kTimeInfinite;
    }
    else
    {
        // Find highest remaining duration
        const int src_remaining_duration =
            std::max(0, src_lifetime.duration_time_ms - Time::TimeStepsToMs(src.current_time_steps));
        dst.lifetime.duration_time_ms = std::max(dst.lifetime.duration_time_ms, src_remaining_duration);
    }

    // Only if both consumable and activations until expiry
    if (dst.lifetime.is_consumable && src.effect_data.lifetime.is_consumable &&
        dst.lifetime.activations_until_expiry != kTimeInfinite &&
        src_lifetime.activations_until_expiry != kTimeInfinite)
    {
        const int src_remaining_activations =
            std::max(src_lifetime.activations_until_expiry - src.current_activations, 0);
        dst.lifetime.activations_until_expiry =
            std::max(dst.lifetime.activations_until_expiry, src_remaining_activations);
    }
}

void AttachedEffectsHelper::IncreaseStacksCount(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect) const
{
    const int max_stacks = attached_effect->effect_data.lifetime.max_stacks;
    if (attached_effect->current_stacks_count < max_stacks || max_stacks == kTimeInfinite)
    {
        int increment = 1;
        if (attached_effect->effect_data.type_id.type == EffectType::kCondition &&
            !attached_effect->effect_data.GetStacksIncrement().IsEmpty())
        {
            increment = world_
                            ->EvaluateExpression(
                                attached_effect->effect_data.GetStacksIncrement(),
                                attached_effect->sender_id,
                                receiver_entity.GetID())
                            .AsInt();
            assert(increment >= 1);
        }

        attached_effect->current_stacks_count += increment;
        if (EnableExtraLogs())
        {
            LogDebug(
                receiver_entity.GetID(),
                "AttachedEffectsHelper::IncreaseStacksCount - current_stacks_count = {}, max_stacks = {} - {}",
                attached_effect->current_stacks_count,
                max_stacks,
                *attached_effect);
        }

        // Reached max stacks
        if (max_stacks != kTimeInfinite && attached_effect->current_stacks_count >= max_stacks)
        {
            attached_effect->current_stacks_count = max_stacks;
            OnReachedMaxStacks(receiver_entity, attached_effect);
        }
    }
}

void AttachedEffectsHelper::OnReachedMaxStacks(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect) const
{
    LogDebug(receiver_entity.GetID(), "AttachedEffectsHelper::OnReachedMaxStacks - {}", *attached_effect);

    // Handle conditions
    OnConditionReachedMaxStacks(receiver_entity, attached_effect);
}

void AttachedEffectsHelper::OnConditionReachedMaxStacks(
    const Entity& receiver_entity,
    const AttachedEffectStatePtr& attached_effect) const
{
    const EntityID receiver_id = receiver_entity.GetID();
    const EffectData& condition_effect_data = attached_effect->effect_data;
    const EffectTypeID& condition_effect_type_id = condition_effect_data.type_id;

    // Not a Condition
    if (condition_effect_type_id.type != EffectType::kCondition)
    {
        return;
    }

    LogDebug(receiver_id, "AttachedEffectsHelper::OnConditionReachedMaxStacks - {}", *attached_effect);

    // Fire effect that happens when we reach max stacks
    // TODO(vampy): Implement these using Mark, but exactly why do we need a mark here?
    EffectState effect_state = attached_effect->effect_state;
    effect_state.source_context.Add(SourceContextType::kReachedMaxStacks);
    switch (condition_effect_type_id.condition_type)
    {
    case EffectConditionType::kPoison:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44368654/Poison
        // Debuff of 100% to Health Gain Efficiency for 3 seconds.
        static constexpr FixedPoint debuff_percentage = 100_fp;
        static constexpr int debuff_duration_ms = 3000;
        const auto debuff_data = EffectData::CreateDebuff(
            StatType::kHealthGainEfficiencyPercentage,
            EffectExpression::FromValue(debuff_percentage),
            debuff_duration_ms);

        // Apply using EffectSystem
        world_->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            attached_effect->combat_unit_sender_id,
            receiver_id,
            debuff_data,
            effect_state);
        break;
    }
    case EffectConditionType::kWound:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44336079/Wound
        // Debuff of 100% to Crit Amp (CritAmplification) for 3 seconds.
        static constexpr FixedPoint debuff_percentage = 100_fp;
        static constexpr int debuff_duration_ms = 3000;
        const auto debuff_data = EffectData::CreateDebuff(
            StatType::kCritAmplificationPercentage,
            EffectExpression::FromValue(debuff_percentage),
            debuff_duration_ms);

        // Apply using EffectSystem
        world_->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            attached_effect->combat_unit_sender_id,
            receiver_id,
            debuff_data,
            effect_state);
        break;
    }
    case EffectConditionType::kBurn:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44368692/Burn
        // Zone of radius 20, 200 Pure Damage.
        static constexpr int zone_radius_units = 10;
        const auto zone_damage = 200_fp * world_->GetStatsConstantsScale();

        // Spawn zone
        ZoneData zone_data;
        zone_data.source_context = effect_state.source_context;
        zone_data.sender_id = attached_effect->combat_unit_sender_id;
        zone_data.original_sender_id = attached_effect->combat_unit_sender_id;
        zone_data.shape = ZoneEffectShape::kHexagon;
        zone_data.radius_sub_units = zone_radius_units;
        zone_data.apply_once = true;
        zone_data.destroy_with_sender = false;
        zone_data.duration_ms = 100;
        zone_data.frequency_ms = 100;
        zone_data.is_critical = effect_state.is_critical;

        // Fill in the zone skill data
        const auto skill_ptr = SkillData::Create();
        skill_ptr->AddEffect(
            EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(zone_damage)));
        zone_data.skill_data = skill_ptr;

        // Spawn the zone at the receiver position
        const auto& receiver_entity_position_component = receiver_entity.Get<PositionComponent>();
        const HexGridPosition& receiver_position = receiver_entity_position_component.GetPosition();
        EntityFactory::SpawnZone(
            *world_,
            world_->GetEntityTeam(attached_effect->combat_unit_sender_id),
            receiver_position,
            zone_data);
        break;
    }
    case EffectConditionType::kFrost:
    {
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/204243633/Frost
        // Direct Deployment of Frozen with a 2-second duration.
        static constexpr int negative_state_duration_ms = 2000;
        const auto negative_state_data =
            EffectData::CreateNegativeState(EffectNegativeState::kFrozen, negative_state_duration_ms);

        // Apply using EffectSystem
        world_->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            attached_effect->combat_unit_sender_id,
            receiver_id,
            negative_state_data,
            effect_state);
        break;
    }
    default:
        break;
    }

    // Cleanse?
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/280297481/Condition.CleanseAtMaxStacks
    if (condition_effect_data.lifetime.cleanse_at_max_stacks)
    {
        LogDebug(receiver_id, "| Cleansing because we reached max stacks");
        RemoveAttachedEffect(receiver_entity, attached_effect);
    }
}

bool AttachedEffectsHelper::CanUseEmpowerOrDisempower(
    const EntityID entity_id,
    const AbilityType ability_type,
    const AttachedEffectStatePtr& attached_effect,
    const bool is_empower,
    const bool is_critical) const
{
    const std::string_view log_prefix = is_empower ? "Empower" : "Disempower";

    if (attached_effect->IsExpired())
    {
        LogDebug(entity_id, "| {} can not activate because it is expired", log_prefix);
        return false;
    }

    if (!attached_effect->CanActivateConsumable())
    {
        LogDebug(entity_id, "| {} can not activate now because of the activation frequency", log_prefix);
        return false;
    }

    // Check if ability activated is the correct type, log required for this check
    if (!CanUseAttachedEffectForAbilityType(entity_id, attached_effect, ability_type, is_critical, true, log_prefix))
    {
        return false;
    }

    return true;
}

bool AttachedEffectsHelper::CanUseAttachedEffectForAbilityType(
    const EntityID entity_id,
    const AttachedEffectStatePtr& attached_effect,
    const AbilityType ability_activated,
    const bool is_critical,
    const bool use_log,
    const std::string_view& log_prefix) const
{
    const EffectData& effect_data = attached_effect->effect_data;

    // Invalid value
    if (effect_data.lifetime.activated_by == AbilityType::kNone)
    {
        // NOTE: Always warn about this
        LogWarn(entity_id, "| {} Has Invalid activated_by IGNORING", log_prefix);
        return false;
    }

    // Ignore if not correct type
    if (effect_data.lifetime.activate_on_critical == true && is_critical == false)
    {
        if (use_log)
        {
            LogDebug(entity_id, "| {} should activate on critical but ability was not critical", log_prefix);
        }
        return false;
    }

    // Log these case for sanity sake
    if (use_log)
    {
        if (effect_data.lifetime.activated_by == AbilityType::kAttack && ability_activated != AbilityType::kAttack)
        {
            LogDebug(
                entity_id,
                "| {} Has activated_by = kAttack but ability is not that ability type. IGNORING",
                log_prefix);
        }
        else if (effect_data.lifetime.activated_by == AbilityType::kOmega && ability_activated != AbilityType::kOmega)
        {
            LogDebug(
                entity_id,
                "| {} Has activated_by = kOmega but ability is not that ability type. IGNORING",
                log_prefix);
        }
    }

    const EffectPackageHelper& effect_package_helper = world_->GetEffectPackageHelper();
    return effect_package_helper.DoesAbilityTypesMatchActivatedAbility(
        {effect_data.lifetime.activated_by},
        ability_activated);
}

bool AttachedEffectsHelper::IncreaseConsumableActivations(
    const EntityID entity_id,
    const AttachedEffectStatePtr& attached_effect,
    const AbilityType ability_activated,
    const bool is_critical) const
{
    const EffectData& effect_data = attached_effect->effect_data;

    // Only interested in consumables
    if (!effect_data.lifetime.is_consumable)
    {
        return false;
    }

    // Check if ability activated is the correct type, no log required for this check
    if (!CanUseAttachedEffectForAbilityType(entity_id, attached_effect, ability_activated, is_critical))
    {
        return false;
    }

    // Don't consider expired effects
    if (attached_effect->IsExpired())
    {
        return false;
    }

    // Increase activations
    if (attached_effect->CanActivateConsumable())
    {
        attached_effect->current_activations++;
    }

    attached_effect->total_activations_lifetime++;
    return true;
}

void AttachedEffectsHelper::RefreshDynamicBuffsDebuffs(
    const Entity& receiver_entity,
    AttachedEffectState& attached_effect) const
{
    const EntityID receiver_id = receiver_entity.GetID();
    if (attached_effect.effect_data.type_id.stat_type != StatType::kMaxHealth)
    {
        attached_effect.CaptureEffectValue(*world_, receiver_id);
        return;
    }

    auto& stats_component = receiver_entity.Get<StatsComponent>();
    const FixedPoint hp_ratio = stats_component.GetCurrentHealth() / stats_component.GetMaxHealth();
    const FixedPoint prev_max_health = stats_component.GetMaxHealth();
    const FixedPoint prev_effect_value = attached_effect.GetCapturedValue();
    attached_effect.CaptureEffectValue(*world_, receiver_id);
    const FixedPoint new_effect_value = attached_effect.GetCapturedValue();

    if (prev_effect_value == new_effect_value)
    {
        return;
    }

    const FixedPoint new_hp = hp_ratio * (prev_max_health + new_effect_value - prev_effect_value);
    const FixedPoint delta_hp = new_hp - stats_component.GetCurrentHealth();
    if (delta_hp != 0_fp)
    {
        stats_component.SetCurrentHealth(new_hp);
        world_->BuildAndEmitEvent<EventType::kHealthChanged>(
            attached_effect.combat_unit_sender_id == kInvalidEntityID ? attached_effect.sender_id
                                                                      : attached_effect.combat_unit_sender_id,
            receiver_id,
            delta_hp);
    }
}

void AttachedEffectsHelper::AdjustFromBuffsDebuffs(
    const Entity& receiver_entity,
    const AttachedEffectState& attached_effect) const
{
    if (EnableExtraLogs())
    {
        LogDebug(receiver_entity.GetID(), "AttachedEffectsHelper::AdjustFromBuffsDebuffs - {}", attached_effect);
    }

    // MaxHealth
    AdjustHealthFromBuffs(receiver_entity, attached_effect);

    // Movement Speed
    AdjustMovementSpeedFromBuffs(receiver_entity, attached_effect);

    // If AttackDamage is buffed/debuffed all three damage types should be buffed/debuffed
    AdjustAttackDamageFromBuffs(receiver_entity, attached_effect);
}

void AttachedEffectsHelper::UndoAdjustFromBuffsDebuffs(
    const Entity& receiver_entity,
    const AttachedEffectState& attached_effect) const
{
    if (EnableExtraLogs())
    {
        LogDebug(receiver_entity.GetID(), "AttachedEffectsHelper::UndoAdjustFromBuffsDebuffs - {}", attached_effect);
    }

    // If max_health buff was deactivated we need to check to see if current_health should be
    // adjusted
    UndoAdjustHealthFromBuffs(receiver_entity, attached_effect);

    // Undo movement speed
    AdjustMovementSpeedFromBuffs(receiver_entity, attached_effect);
}

void AttachedEffectsHelper::AdjustHealthFromBuffs(
    const Entity& receiver_entity,
    const AttachedEffectState& attached_effect) const
{
    const auto& effect_data = attached_effect.effect_data;
    const EffectTypeID& effect_type_id = effect_data.type_id;

    if (effect_type_id.stat_type != StatType::kMaxHealth)
    {
        return;
    }
    if (effect_type_id.type != EffectType::kBuff && effect_type_id.type != EffectType::kDebuff)
    {
        return;
    }

    auto& stats_component = receiver_entity.Get<StatsComponent>();
    const EntityID combat_unit_sender_id = attached_effect.combat_unit_sender_id;
    const EntityID receiver_id = receiver_entity.GetID();

    // For stacked effects, only consider most recent change
    FixedPoint effect_value = 0_fp;
    if (attached_effect.current_stacks_count > 1)
    {
        effect_value = attached_effect.last_delta_value;
    }
    else
    {
        if (effect_data.type_id.UsesCapturedValue())
        {
            effect_value = attached_effect.GetCapturedValue();
        }
        else
        {
            effect_value = world_->EvaluateExpression(effect_data.GetExpression(), combat_unit_sender_id, receiver_id);
        }
    }

    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/44204561/Max+Health
    // When max_health is buffed/debuff current health is adjusted by the same value
    // This code works only because if the kBuff and kDebuff effects just by existing are
    // considered to be already applied
    if (effect_type_id.type == EffectType::kBuff)
    {
        const FixedPoint receiver_current_health = stats_component.GetCurrentHealth();
        const FixedPoint receiver_new_health = receiver_current_health + effect_value;

        stats_component.SetCurrentHealth(receiver_new_health);

        LogDebug(
            combat_unit_sender_id,
            "AttachedEffectsHelper::AdjustHealthFromBuffs - kHealthChanged receiver_new_health = {}, current_health "
            "buffed by max_health = {} buff value",
            receiver_new_health,
            effect_value);
        const FixedPoint delta = receiver_new_health - receiver_current_health;
        world_->BuildAndEmitEvent<EventType::kHealthChanged>(
            combat_unit_sender_id == kInvalidEntityID ? attached_effect.sender_id : combat_unit_sender_id,
            receiver_id,
            delta);
    }
    else if (effect_type_id.type == EffectType::kDebuff)
    {
        const FixedPoint receiver_current_health = stats_component.GetCurrentHealth();
        const FixedPoint receiver_new_health = receiver_current_health - effect_value;

        stats_component.SetCurrentHealth(receiver_new_health);

        LogDebug(
            combat_unit_sender_id,
            "AttachedEffectsHelper::AdjustHealthFromBuffs - kHealthChanged receiver_new_health ({}), current_health "
            "debuffed by max_health ({}) debuff value",
            receiver_new_health,
            effect_value);
        const FixedPoint delta = receiver_current_health - receiver_new_health;
        world_->BuildAndEmitEvent<EventType::kHealthChanged>(
            combat_unit_sender_id == kInvalidEntityID ? attached_effect.sender_id : combat_unit_sender_id,
            receiver_id,
            delta);
    }
}

void AttachedEffectsHelper::UndoAdjustHealthFromBuffs(
    const Entity& receiver_entity,
    const AttachedEffectState& attached_effect) const
{
    // Don't modify health after battle finished
    if (world_->IsBattleFinished())
    {
        return;
    }

    const auto& effect_data = attached_effect.effect_data;
    const EffectTypeID& effect_type_id = effect_data.type_id;
    if (effect_type_id.stat_type != StatType::kMaxHealth)
    {
        return;
    }
    if (effect_type_id.type != EffectType::kBuff && effect_type_id.type != EffectType::kDebuff)
    {
        return;
    }

    const EntityID receiver_id = receiver_entity.GetID();

    // Checks if current_health is higher than max_health after buff to max_health expires
    auto& stats_component = receiver_entity.Get<StatsComponent>();

    const FixedPoint current_health = stats_component.GetCurrentHealth();
    const FixedPoint max_health = stats_component.GetMaxHealth();

    const EntityID combat_unit_sender_id = attached_effect.combat_unit_sender_id;
    if (current_health > max_health)
    {
        stats_component.SetCurrentHealth(max_health);
        LogDebug(
            combat_unit_sender_id,
            "AttachedEffectsHelper::UndoAdjustHealthFromBuffs - kHealthChanged new health is max_health = {}, since "
            "current_health was higher than max_health after buff expired",
            max_health);
        const FixedPoint delta = current_health - max_health;
        world_->BuildAndEmitEvent<EventType::kHealthChanged>(
            combat_unit_sender_id == kInvalidEntityID ? attached_effect.sender_id : combat_unit_sender_id,
            receiver_id,
            delta);
    }
}

void AttachedEffectsHelper::AdjustMovementSpeedFromBuffs(
    const Entity& receiver_entity,
    const AttachedEffectState& attached_effect) const
{
    const auto& effect_data = attached_effect.effect_data;
    const EffectTypeID& effect_type_id = effect_data.type_id;

    if (effect_type_id.stat_type != StatType::kMoveSpeedSubUnits)
    {
        return;
    }
    if (effect_type_id.type != EffectType::kBuff && effect_type_id.type != EffectType::kDebuff)
    {
        return;
    }

    const FixedPoint current_live_move_speed_sub_units =
        world_->GetLiveStats(receiver_entity.GetID()).Get(StatType::kMoveSpeedSubUnits);
    auto& movement_component = receiver_entity.Get<MovementComponent>();
    movement_component.SetMovementSpeedSubUnits(current_live_move_speed_sub_units);
}

void AttachedEffectsHelper::AdjustAttackDamageFromBuffs(
    const Entity& receiver_entity,
    const AttachedEffectState& attached_effect) const
{
    const auto& effect_to_apply = attached_effect.effect_data;
    const EffectTypeID& effect_type_id = effect_to_apply.type_id;

    if (effect_type_id.stat_type != StatType::kAttackDamage)
    {
        return;
    }
    if (effect_type_id.type != EffectType::kBuff && effect_type_id.type != EffectType::kDebuff)
    {
        return;
    }

    const EntityID sender_id = attached_effect.sender_id;

    // When kAttackDamage is buffed/debuffed each damage type should also be buffed/debuffed
    const AttachedEffectsHelper& attached_effects_helper = world_->GetAttachedEffectsHelper();
    for (const StatType stat : kAttackDamageStatTypes)
    {
        EffectExpression expression = effect_to_apply.GetExpression();
        expression.ReplaceStat(StatType::kAttackDamage, stat);

        EffectData effect_data;
        effect_data.type_id.type = effect_type_id.type;
        effect_data.type_id.stat_type = stat;
        effect_data.SetExpression(expression);
        effect_data.lifetime = effect_to_apply.lifetime;

        attached_effects_helper.AddAttachedEffect(receiver_entity, sender_id, effect_data, EffectState{});
    }
}

}  // namespace simulation
