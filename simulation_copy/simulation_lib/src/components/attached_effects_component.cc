#include "attached_effects_component.h"

#include <cassert>

#include "components/focus_component.h"
#include "ecs/world.h"
#include "utility/enum.h"
#include "utility/struct_formatting_helper.h"
#include "utility/vector_helper.h"

namespace simulation
{
size_t AttachedEffectsComponent::GetAllRootEffectsOfTypeIDPerAbilityName(
    const EffectTypeID& effect_type_id,
    std::map<std::string_view, std::vector<AttachedEffectStatePtr>>* out_same_abilities,
    std::vector<AttachedEffectStatePtr>* out_different_abilities) const
{
    out_same_abilities->clear();
    out_different_abilities->clear();

    // Filter and count how many of the same ability type we have
    std::unordered_map<std::string_view, int> ability_names_counts;
    std::vector<AttachedEffectStatePtr> same_type_attached_effects;
    for (const auto& attached_effect : attached_effects_)
    {
        if (!attached_effect)
        {
            continue;
        }

        // We are only interested in root types
        if (!attached_effect->IsRootType())
        {
            continue;
        }

        // Not marked for destruction
        if (!attached_effect->IsValid())
        {
            continue;
        }

        // Is the same type id :)
        if (attached_effect->effect_data.type_id == effect_type_id)
        {
            const std::string_view ability_name = attached_effect->effect_state.source_context.ability_name;

            // Keep track and increase count of this ability name
            same_type_attached_effects.push_back(attached_effect);
            ability_names_counts[ability_name]++;
        }
    }

    // Group into the out return values
    for (const auto& attached_effect : same_type_attached_effects)
    {
        // Has at least two of the same group
        const std::string_view ability_name = attached_effect->effect_state.source_context.ability_name;
        if (ability_names_counts.count(ability_name) > 0 && ability_names_counts.at(ability_name) >= 2)
        {
            // Group by same
            (*out_same_abilities)[ability_name].push_back(attached_effect);
        }
        else
        {
            // Different
            out_different_abilities->push_back(attached_effect);
        }
    }

    return same_type_attached_effects.size();
}

void AttachedEffectState::CaptureEffectValue(const World& world, const EntityID receiver_id)
{
    static constexpr std::string_view method_name = "AttachedEffectState::CaptureEffectValue";

    bool all_requirements_available = true;
    EntityID sender_focus_id = kInvalidEntityID;
    ExpressionStatsSource effect_stats_source;
    effect_stats_source.Set(ExpressionDataSourceType::kReceiver, world.GetEntityDataForExpression(receiver_id));

    const auto& expr = effect_data.GetExpression();
    const auto required_sources = expr.GatherRequiredDataSourceTypes();

    const bool use_previous_live_stats = !effect_data.HasStaticFrequency();
    auto get_entity_data = [&](const EntityID id)
    {
        EntityDataForExpression data{};
        data.stats.base = world.GetBaseStats(id);
        data.synergies = &world.GetAllSynergiesOfEntityID(id);
        if (use_previous_live_stats)
        {
            data.stats.live = world.GetPreviousLiveStats(id);
        }
        else
        {
            data.stats.live = world.GetLiveStats(id);
        }

        return data;
    };

    if (required_sources.Contains(ExpressionDataSourceType::kSender))
    {
        effect_stats_source.Set(ExpressionDataSourceType::kSender, get_entity_data(combat_unit_sender_id));
    }

    if (required_sources.Contains(ExpressionDataSourceType::kReceiver))
    {
        // Try to reduce an influence of added value from this effect while computing a new value
        EntityDataForExpression receiver_data = get_entity_data(receiver_id);
        FixedPoint stat_value_without_effect_influence = receiver_data.stats.live.Get(effect_data.type_id.stat_type);
        switch (effect_data.type_id.type)
        {
        case EffectType::kBuff:
            stat_value_without_effect_influence -= GetCapturedValue();
            break;
        case EffectType::kDebuff:
            stat_value_without_effect_influence += GetCapturedValue();
            break;
        default:
            world.LogErr("{} - Unsupported aura effect type {}", method_name, effect_data.type_id.type);
            break;
        }
        receiver_data.stats.live.Set(effect_data.type_id.stat_type, stat_value_without_effect_influence);

        effect_stats_source.Set(ExpressionDataSourceType::kReceiver, receiver_data);
    }
    if (required_sources.Contains(ExpressionDataSourceType::kSenderFocus))
    {
        // find out who's focused at the moment (if any)
        if (const auto& sender_entity_ptr = world.GetByIDPtr(combat_unit_sender_id))
        {
            if (const auto* focus_component = sender_entity_ptr->GetPtr<FocusComponent>())
            {
                const EntityID focused_entity_id = focus_component->GetFocusID();
                if (world.HasEntity(focused_entity_id))
                {
                    sender_focus_id = focused_entity_id;
                }
            }
        }

        if (sender_focus_id != kInvalidEntityID)
        {
            effect_stats_source.Set(ExpressionDataSourceType::kSenderFocus, get_entity_data(sender_focus_id));
        }
        else
        {
            all_requirements_available = false;
            world.LogWarn("{} - Failed to get stats of sender current focus", method_name, combat_unit_sender_id);
        }
    }

    if (all_requirements_available)
    {
        captured_effect_value = effect_data.GetExpression().Evaluate(
            ExpressionEvaluationContext(&world, combat_unit_sender_id, receiver_id, sender_focus_id),
            effect_stats_source);
    }
    else
    {
        captured_effect_value = 0_fp;
    }
}

void AttachedEffectsComponent::AddAttachedEffectToActive(const AttachedEffectStatePtr& attached_effect)
{
    const EffectData& effect_data = attached_effect->effect_data;
    const EffectTypeID& effect_type_id = effect_data.type_id;

    ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
    switch (effect_type_id.type)
    {
    case EffectType::kPositiveState:
        active_positive_states_[effect_type_id.positive_state].emplace_back(attached_effect);
        break;
    case EffectType::kNegativeState:
        active_negative_states_[effect_type_id.negative_state].emplace_back(attached_effect);
        break;
    case EffectType::kBuff:
        active_buffs_.buffs[effect_type_id.stat_type].emplace_back(attached_effect);
        break;
    case EffectType::kDebuff:
        active_buffs_.debuffs[effect_type_id.stat_type].emplace_back(attached_effect);
        break;
    case EffectType::kPlaneChange:
        active_plane_changes_[effect_type_id.plane_change].emplace_back(attached_effect);
        break;
    case EffectType::kCondition:
        active_conditions_[effect_type_id.condition_type].emplace_back(attached_effect);
        break;
    case EffectType::kEmpower:
        active_empowers_.emplace_back(attached_effect);
        break;
    case EffectType::kDisempower:
        active_disempowers_.emplace_back(attached_effect);
        break;
    case EffectType::kDamageOverTime:
        active_damages_over_time_.emplace_back(attached_effect);
        break;
    case EffectType::kEnergyBurnOverTime:
        active_energy_burns_over_time_.emplace_back(attached_effect);
        break;
    case EffectType::kEnergyGainOverTime:
        active_energy_gains_over_time_.emplace_back(attached_effect);
        break;
    case EffectType::kHealOverTime:
        active_hots_.emplace_back(attached_effect);
        break;
    case EffectType::kHyperGainOverTime:
        active_hyper_gains_.emplace_back(attached_effect);
        break;
    case EffectType::kHyperBurnOverTime:
        active_hyper_burns_.emplace_back(attached_effect);
        break;
    case EffectType::kExecute:
        active_executes_.emplace_back(attached_effect);
        break;
    case EffectType::kBlink:
        active_blinks_.emplace_back(attached_effect);
        break;
    default:
        break;
    }
}

template <typename Key>
inline static bool RemoveEffectStateFromMap(
    std::unordered_map<Key, std::vector<AttachedEffectStatePtr>>& map,
    const Key key,
    const AttachedEffectStatePtr& attached_effect)
{
    if (map.count(key) == 0)
    {
        return false;
    }

    auto& vector = map[key];
    const size_t prev_size = vector.size();
    VectorHelper::EraseValue(vector, attached_effect);
    const size_t new_size = vector.size();

    // Cleanup the key
    if (vector.empty())
    {
        map.erase(key);
    }

    return prev_size != new_size;
}

void AttachedEffectsComponent::RemoveAttachedEffectFromActive(const AttachedEffectStatePtr& attached_effect)
{
    const EffectData& effect_data = attached_effect->effect_data;
    const EffectTypeID& effect_type_id = effect_data.type_id;

    ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
    switch (effect_type_id.type)
    {
    case EffectType::kPositiveState:
        RemoveEffectStateFromMap(active_positive_states_, effect_type_id.positive_state, attached_effect);
        break;
    case EffectType::kNegativeState:
        RemoveEffectStateFromMap(active_negative_states_, effect_type_id.negative_state, attached_effect);
        break;
    case EffectType::kPlaneChange:
        RemoveEffectStateFromMap(active_plane_changes_, effect_type_id.plane_change, attached_effect);
        break;
    case EffectType::kCondition:
        RemoveEffectStateFromMap(active_conditions_, effect_type_id.condition_type, attached_effect);
        break;
    case EffectType::kBuff:
        RemoveEffectStateFromMap(active_buffs_.buffs, effect_type_id.stat_type, attached_effect);
        break;
    case EffectType::kDebuff:
        RemoveEffectStateFromMap(active_buffs_.debuffs, effect_type_id.stat_type, attached_effect);
        break;
    case EffectType::kEmpower:
        VectorHelper::EraseValue(active_empowers_, attached_effect);
        break;
    case EffectType::kDisempower:
        VectorHelper::EraseValue(active_disempowers_, attached_effect);
        break;
    case EffectType::kDamageOverTime:
        VectorHelper::EraseValue(active_damages_over_time_, attached_effect);
        break;
    case EffectType::kEnergyBurnOverTime:
        VectorHelper::EraseValue(active_energy_burns_over_time_, attached_effect);
        break;
    case EffectType::kEnergyGainOverTime:
        VectorHelper::EraseValue(active_energy_gains_over_time_, attached_effect);
        break;
    case EffectType::kHyperGainOverTime:
        VectorHelper::EraseValue(active_hyper_gains_, attached_effect);
        break;
    case EffectType::kHyperBurnOverTime:
        VectorHelper::EraseValue(active_hyper_burns_, attached_effect);
        break;
    case EffectType::kHealOverTime:
        VectorHelper::EraseValue(active_hots_, attached_effect);
        break;
    case EffectType::kExecute:
        VectorHelper::EraseValue(active_executes_, attached_effect);
        break;
    case EffectType::kBlink:
        VectorHelper::EraseValue(active_blinks_, attached_effect);
        break;
    default:
        break;
    }
}

template <typename Key>
inline static bool MapContainsEffectState(
    const std::unordered_map<Key, std::vector<AttachedEffectStatePtr>>& map,
    const Key key,
    const AttachedEffectStatePtr& attached_effect)
{
    return map.count(key) > 0 && VectorHelper::ContainsValue(map.at(key), attached_effect);
}

bool AttachedEffectsComponent::IsAttachedEffectInActive(const AttachedEffectStatePtr& attached_effect) const
{
    const EffectData& effect_data = attached_effect->effect_data;
    const EffectTypeID& effect_type_id = effect_data.type_id;

    ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
    switch (effect_type_id.type)
    {
    case EffectType::kPositiveState:
        return MapContainsEffectState(active_positive_states_, effect_type_id.positive_state, attached_effect);

    case EffectType::kNegativeState:
        return MapContainsEffectState(active_negative_states_, effect_type_id.negative_state, attached_effect);

    case EffectType::kBuff:
        return MapContainsEffectState(active_buffs_.buffs, effect_type_id.stat_type, attached_effect);

    case EffectType::kDebuff:
        return MapContainsEffectState(active_buffs_.debuffs, effect_type_id.stat_type, attached_effect);

    case EffectType::kPlaneChange:
        return MapContainsEffectState(active_plane_changes_, effect_type_id.plane_change, attached_effect);

    case EffectType::kCondition:
        return MapContainsEffectState(active_conditions_, effect_type_id.condition_type, attached_effect);

    case EffectType::kEmpower:
        return VectorHelper::ContainsValue(active_empowers_, attached_effect);

    case EffectType::kDisempower:
        return VectorHelper::ContainsValue(active_disempowers_, attached_effect);

    case EffectType::kDamageOverTime:
        return VectorHelper::ContainsValue(active_damages_over_time_, attached_effect);

    case EffectType::kEnergyBurnOverTime:
        return VectorHelper::ContainsValue(active_energy_burns_over_time_, attached_effect);

    case EffectType::kEnergyGainOverTime:
        return VectorHelper::ContainsValue(active_energy_gains_over_time_, attached_effect);

    case EffectType::kHealOverTime:
        return VectorHelper::ContainsValue(active_hots_, attached_effect);

    case EffectType::kHyperGainOverTime:
        return VectorHelper::ContainsValue(active_hyper_gains_, attached_effect);

    case EffectType::kHyperBurnOverTime:
        return VectorHelper::ContainsValue(active_hyper_burns_, attached_effect);

    case EffectType::kExecute:
        return VectorHelper::ContainsValue(active_executes_, attached_effect);

    case EffectType::kBlink:
        return VectorHelper::ContainsValue(active_blinks_, attached_effect);

    default:
        break;
    }

    return false;
}

std::vector<AttachedEffectStatePtr> AttachedEffectsComponent::GetAllCleansableDebuffs() const
{
    // Build effects to remove
    std::vector<AttachedEffectStatePtr> effects_to_remove;
    for (const auto& [stat_type, debuffs] : active_buffs_.debuffs)
    {
        for (const auto& attached_effect : debuffs)
        {
            if (attached_effect->CanCleanse())
            {
                effects_to_remove.emplace_back(attached_effect);
            }
        }
    }

    return effects_to_remove;
}

std::vector<AttachedEffectStatePtr> AttachedEffectsComponent::GetAllCleansableNegativeStates() const
{
    // Build effects to remove
    std::vector<AttachedEffectStatePtr> effects_to_remove;
    for (const auto& [negative_state, attached_effect_vector] : active_negative_states_)
    {
        for (const auto& attached_effect : attached_effect_vector)
        {
            if (attached_effect->CanCleanse())
            {
                effects_to_remove.emplace_back(attached_effect);
            }
        }
    }

    return effects_to_remove;
}

std::vector<AttachedEffectStatePtr> AttachedEffectsComponent::GetAllCleansableConditions() const
{
    // Build effects to remove
    std::vector<AttachedEffectStatePtr> effects_to_remove;
    for (const auto& [condition_type, attached_effect_vector] : active_conditions_)
    {
        for (const auto& attached_effect : attached_effect_vector)
        {
            if (attached_effect->CanCleanse())
            {
                effects_to_remove.emplace_back(attached_effect);
            }
        }
    }

    return effects_to_remove;
}

std::vector<AttachedEffectStatePtr> AttachedEffectsComponent::GetAllCleansableDOTs() const
{
    // Build effects to remove
    std::vector<AttachedEffectStatePtr> effects_to_remove;
    for (const auto& dot_effect : active_damages_over_time_)
    {
        if (dot_effect->CanCleanse())
        {
            effects_to_remove.emplace_back(dot_effect);
        }
    }

    return effects_to_remove;
}

std::vector<AttachedEffectStatePtr> AttachedEffectsComponent::GetAllCleansableEnergyBurnOverTimes() const
{
    // Build effects to remove
    std::vector<AttachedEffectStatePtr> effects_to_remove;
    for (const auto& dot_effect : active_energy_burns_over_time_)
    {
        if (dot_effect->CanCleanse())
        {
            effects_to_remove.emplace_back(dot_effect);
        }
    }

    return effects_to_remove;
}

void AttachedEffectsComponent::EraseDestroyedEffects(const bool always_destroy_delayed)
{
    const int current_time_step = GetOwnerWorld()->GetTimeStepCount();
    VectorHelper::EraseValuesForCondition(
        attached_effects_,
        [&](const AttachedEffectStatePtr& effect)
        {
            if (effect->state == AttachedEffectStateType::kDestroyed)
            {
                bool must_be_destroyed = always_destroy_delayed;
                must_be_destroyed |= effect->time_step_when_destroy < 0;
                must_be_destroyed |= effect->time_step_when_destroy >= current_time_step;
                return must_be_destroyed;
            }
            return false;
        });
}

void AttachedEffectsComponent::RemoveNegativeStateNextTimeStep(const EffectNegativeState negative_state)
{
    auto& world = *GetOwnerWorld();
    auto& attached_effects_helper = world.GetAttachedEffectsHelper();
    constexpr int destroy_delay = 1;  // On the next time step
    if (active_negative_states_.count(negative_state) > 0)
    {
        const auto states_to_remove = active_negative_states_.at(negative_state);
        for (const auto& state_to_remove : states_to_remove)
        {
            attached_effects_helper.RemoveAttachedEffect(*GetOwnerEntity(), state_to_remove, destroy_delay);
        }
    }
}

void AttachedEffectState::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");

    h.Field("type", effect_data.type_id.type);
    h.Field("combat_unit_sender_id", combat_unit_sender_id);
    h.Field("children.size()", children.size());
    h.Field("state", state);

    if (IsConsumable())
    {
        h.Field("current_activations", current_activations);
        h.Field("activations_until_expiry", effect_data.lifetime.activations_until_expiry);
    }

    if (effect_data.lifetime.blocks_until_expiry != kTimeInfinite)
    {
        h.Field("current_blocks", current_blocks);
        h.Field("blocks_until_expiry", effect_data.lifetime.blocks_until_expiry);
    }

    h.Field("current_time_steps", current_time_steps);
    h.Field("duration_time_steps", duration_time_steps);
    h.Field("current_stacks_count", current_stacks_count);
    h.Field("data", effect_data);
    h.Write("}}");
}

bool AttachedEffectsComponent::HasImmunityTo(const EffectTypeID& type_id) const
{
    if (active_positive_states_.count(EffectPositiveState::kImmune) == 0)
    {
        return false;
    }

    for (const auto& effect_state_ptr : active_positive_states_.at(EffectPositiveState::kImmune))
    {
        const auto& immuned_types = effect_state_ptr->effect_data.immuned_effect_types;
        // Absolute immunity, means we are immune to everything.
        if (immuned_types.empty())
        {
            return true;
        }

        // TODO(kostiantyn): need another comparison method which will not take into
        // consideration fields that are not significant here
        for (auto& immuned_type_id : immuned_types)
        {
            if (immuned_type_id == type_id)
            {
                return true;
            }
        }
    }

    return false;
}

bool AttachedEffectsComponent::HasImmunityToNegativeState(EffectNegativeState state) const
{
    EffectTypeID type_id;
    type_id.type = EffectType::kNegativeState;
    type_id.negative_state = state;
    return HasImmunityTo(type_id);
}

bool AttachedEffectsComponent::HasImmunityToAllDetrimentalEffects() const
{
    if (active_positive_states_.count(EffectPositiveState::kImmune) == 0)
    {
        return false;
    }

    for (const auto& effect_state_ptr : active_positive_states_.at(EffectPositiveState::kImmune))
    {
        const AttachedEffectState& effect_state = *effect_state_ptr;
        if (effect_state.effect_data.immuned_effect_types.empty())
        {
            return true;
        }
    }

    return false;
}

}  // namespace simulation
