#include "abilities_component.h"

#include <cassert>
#include <sstream>

#include "components/position_component.h"
#include "data/ability_data.h"
#include "data/enums.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/hex_grid_position.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"
#include "utility/time.h"

namespace simulation
{
void SkillTargetingState::MergeValidTargets(
    const World& world,
    const TargetingHelper::SkillTargetFindResult& find_result,
    const size_t max_targets)
{
    const std::vector<EntityID>& targets = find_result.receiver_ids;

    // Just replace current state with input data
    if (max_targets != 0 && targets.size() == max_targets)
    {
        CreateFromFindResult(world, find_result);
        return;
    }

    // Merge
    for (const EntityID target : targets)
    {
        if (available_targets.count(target) == 0)
        {
            available_targets.emplace(target);
            SaveTargetData(world, target);
        }
    }

    // max_targets = 0 means no limit
    if (max_targets == 0)
    {
        return;
    }

    // Remove extra targets from available_targets so that it matches max_targets size
    for (auto it = available_targets.begin(); it != available_targets.end() && available_targets.size() > max_targets;)
    {
        if (!VectorHelper::ContainsValue(targets, *it))
        {
            it = available_targets.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void SkillTargetingState::SaveTargetData(const World& world, const EntityID entity_id)
{
    if (!world.HasEntity(entity_id))
    {
        return;
    }

    const Entity& entity = world.GetByID(entity_id);
    if (!entity.Has<PositionComponent>())
    {
        return;
    }

    const auto& position_component = entity.Get<PositionComponent>();
    const HexGridPosition hex_grid_position = position_component.GetPosition();

    targets_saved_data[entity_id] = hex_grid_position;
}

// Create a new targeting state from find result
void SkillTargetingState::CreateFromFindResult(
    const World& world,
    const TargetingHelper::SkillTargetFindResult& find_result)
{
    Clear();
    for (const EntityID entity_id : find_result.receiver_ids)
    {
        if (!world.HasEntity(entity_id))
        {
            continue;
        }

        const Entity& entity = world.GetByID(entity_id);
        if (!entity.Has<PositionComponent>())
        {
            continue;
        }

        SaveTargetData(world, entity_id);

        available_targets.insert(entity_id);
    }

    true_sender_id = find_result.true_sender_id;
}

void AbilityState::UpdateTimers(const int new_total_duration_ms)
{
    total_duration_ms = new_total_duration_ms;

    // Build for each skill
    for (SkillState& state_skill : skills)
    {
        bool truncate_times_to_nearest_time_step = false;
        // Attack ability
        if (ability_type == AbilityType::kAttack)
        {
            state_skill.data->percentage_of_ability_duration = kMaxPercentage;
        }
        else
        {
            // Normal attacks use exact timing because of the overflow system and don't have their
            // times truncated, but all other abilities have their times truncated to be simpler.
            truncate_times_to_nearest_time_step = true;
        }

        // Update timers
        state_skill.UpdateTimers(new_total_duration_ms, truncate_times_to_nearest_time_step);
    }
}

void AbilityState::Activate(const int time_step)
{
    is_active = true;
    activation_count++;
    last_activation_time_step = time_step;
    last_activation_ms = Time::TimeStepsToMs(last_activation_time_step);
    previous_overflow_ms = current_ability_time_ms - total_duration_ms;
    current_ability_time_ms = 0;
    current_skill_time_ms = 0;

    // Reset skills states
    current_skill_index = 0;
    for (auto& skill : skills)
    {
        skill.ResetState();
    }
}

std::shared_ptr<Component> AbilitiesComponent::CreateDeepCopyFromInitialState() const
{
    // NOTE: Create a new copy and initialize that
    auto new_component = std::make_shared<AbilitiesComponent>();

    // Reinitialize the states
    new_component->SetAbilitiesData(GetDataAttackAbilities(), AbilityType::kAttack);
    new_component->SetAbilitiesData(GetDataOmegaAbilities(), AbilityType::kOmega);
    new_component->GetAbilities(AbilityType::kInnate).data = {};
    new_component->AddDataInnateAbilities(GetDataInnateAbilities().abilities);

    return new_component;
}

bool AbilitiesComponent::DoesOmegaRequireFocusToStart() const
{
    const auto& omega_abilities = GetDataOmegaAbilities();
    if (omega_abilities.abilities.empty())
    {
        return false;
    }

    const auto& first_ability_data = omega_abilities.abilities.front();
    if (first_ability_data->skills.empty())
    {
        return false;
    }

    const auto& first_skill_data = first_ability_data->skills.front();
    return first_skill_data->targeting.type == SkillTargetingType::kCurrentFocus;
}

AbilityStatePtr AbilitiesComponent::CreateAbilityStateFromData(
    const std::shared_ptr<const AbilityData>& ability_data_ptr,
    const AbilityType ability_type,
    size_t ability_index)
{
    // Create new state ability
    AbilityStatePtr state_ability_ptr = AbilityState::Create();
    auto& state_ability = *state_ability_ptr;

    const AbilityData& ability_data = *ability_data_ptr;

    state_ability.index = ability_index;
    state_ability.ability_type = ability_type;
    state_ability.activation_time_limit_time_steps =
        Time::MsToTimeSteps(ability_data.activation_trigger_data.activation_time_limit_ms);
    state_ability.activate_every_time_steps =
        Time::MsToTimeSteps(ability_data.activation_trigger_data.activate_every_time_ms);
    state_ability.activation_cooldown_time_steps =
        Time::MsToTimeSteps(ability_data.activation_trigger_data.activation_cooldown_ms);

    // Copy pointer to the data
    state_ability.data = ability_data_ptr;

    // Build for each skill
    state_ability.skills.resize(ability_data.skills.size());
    state_ability.skills.shrink_to_fit();

    for (size_t skill_index = 0; skill_index < ability_data.skills.size(); skill_index++)
    {
        auto& state_skill = state_ability.skills[skill_index];
        state_skill.index = skill_index;

        // Copy the pointer to the data
        state_skill.data = ability_data.skills[skill_index];
    }

    // Convert timers to time steps
    state_ability.UpdateTimers(ability_data.total_duration_ms);

    return state_ability_ptr;
}

void AbilitiesComponent::InitializeAbilitiesStateFromData(
    const AbilitiesData& data,
    const AbilityType ability_type,
    AbilitiesState* state)
{
    *state = {};
    auto& state_abilities = state->abilities;

    // Shrink to fit
    state_abilities.resize(data.abilities.size());
    state_abilities.shrink_to_fit();

    // Build for each ability
    for (size_t ability_index = 0; ability_index < data.abilities.size(); ability_index++)
    {
        state_abilities[ability_index] =
            CreateAbilityStateFromData(data.abilities[ability_index], ability_type, ability_index);
    }
}

AbilityStatePtr AbilitiesComponent::ChooseAbility(
    const World& world,
    const EntityID entity_id,
    const AbilitiesData& data,
    const AbilitiesState& state)
{
    size_t ability_index = kInvalidIndex;
    const size_t size = state.abilities.size();
    switch (data.selection_type)
    {
    case AbilitySelectionType::kCycle:
        if (state.current_ability_index == kInvalidIndex)
        {
            ability_index = 0;
        }
        else
        {
            ability_index = (state.current_ability_index + 1) % size;
        }
        break;

    case AbilitySelectionType::kSelfAttributeCheck:
    {
        // This Selection Type can only be used if the Ability list is of length 2
        if (data.abilities.size() != 2)
        {
            assert(false);
            world.LogErr(
                entity_id,
                "AbilitiesComponent::ChooseAbility - for selection_type = kSelfAttributeCheck "
                "failed "
                "because abilities length must be exactly 2");
            break;
        }

        // Check for 0 percentage (dead)
        if (data.activation_check_value == 0_fp || data.activation_check_stat_type == StatType::kNone)
        {
            assert(false);
            world.LogErr(
                entity_id,
                "AbilitiesComponent::ChooseAbility - for selection_type = kSelfAttributeCheck "
                "failed "
                "because the activation check value is 0");
            break;
        }
        const auto& stats = world.GetLiveStats(entity_id);

        const auto stat_value = stats.Get(data.activation_check_stat_type);
        if (stat_value >= data.activation_check_value)
        {
            ability_index = 0;
        }
        else
        {
            ability_index = 1;
        }

        break;
    }
    case AbilitySelectionType::kSelfHealthCheck:
    {
        // This Selection Type can only be used if the Ability list is of length 2
        if (data.abilities.size() != 2)
        {
            assert(false);
            world.LogErr(
                entity_id,
                "AbilitiesComponent::ChooseAbility - for selection_type = kSelfHealthCheck failed "
                "because abilities length must be exactly 2");
            break;
        }

        // Check for 0 percentage (dead)
        if (data.activation_check_value == 0_fp)
        {
            assert(false);
            world.LogErr(
                entity_id,
                "AbilitiesComponent::ChooseAbility - for selection_type = kSelfHealthCheck failed "
                "because the activation check value is 0");
            break;
        }

        const auto& live_stats = world.GetLiveStats(entity_id);
        const FixedPoint health_percentage =
            live_stats.Get(StatType::kCurrentHealth).AsProportionalPercentageOf(live_stats.Get(StatType::kMaxHealth));

        if (health_percentage >= data.activation_check_value)
        {
            ability_index = 0;
        }
        else
        {
            ability_index = 1;
        }

        break;
    }

    case AbilitySelectionType::kEveryXActivations:
    {
        // This Selection Type can only be used if the Ability list is of length 2
        if (data.abilities.size() != 2)
        {
            assert(false);
            world.LogErr(
                entity_id,
                "AbilitiesComponent::ChooseAbility - for selection_type = kEveryXActivations "
                "failed "
                "because abilities length must be exactly 2");
            break;
        }

        // Check for 0 activation cadence
        if (data.activation_cadence == 0)
        {
            assert(false);
            world.LogErr(
                entity_id,
                "AbilitiesComponent::ChooseAbility - for selection_type = kEveryXActivations "
                "failed "
                "because activation cadence is 0");

            break;
        }

        // Special case when cadence is 1 it is the same as kCycle
        // TODO(vampy): Ideally this should also check for the activation count and not choose
        // the next index. But in practice every time we choose an ability we also use it, so this
        // should match
        if (data.activation_cadence == 1)
        {
            if (state.current_ability_index == kInvalidIndex)
            {
                ability_index = 0;
            }
            else
            {
                ability_index = (state.current_ability_index + 1) % size;
            }
            break;
        }

        // No activations, set the default
        if (state.total_activations_count == 0)
        {
            ability_index = 0;
            break;
        }

        // Every activation_cadence choose ability 1
        // Otherwise choose ability 0
        if (state.total_activations_count % data.activation_cadence == 0)
        {
            ability_index = 1;
        }
        else
        {
            ability_index = 0;
        }

        break;
    }
    case AbilitySelectionType::kNone:
    default:
        ability_index = 0;
        break;
    }

    if (state.abilities.size() > ability_index)
    {
        return state.abilities[ability_index];
    }

    return nullptr;
}

bool AbilityState::HasSkillWithMovement() const
{
    // Check skills for movement
    for (const auto& skill : skills)
    {
        switch (skill.data->deployment.type)
        {
        case SkillDeploymentType::kDash:
            // Dash is movement skill, return true
            return true;
        default:
            break;
        }

        for (const auto& effect : skill.data->effect_package.effects)
        {
            switch (effect.type_id.type)
            {
            case EffectType::kBlink:
                // Blink is movement skill, return true
                return true;
            default:
                continue;
            }
        }
    }

    return false;
}

void AbilitiesComponent::AddDataInnateAbility(const std::shared_ptr<const AbilityData>& ability_data)
{
    auto& abilities_data = GetAbilities(AbilityType::kInnate).data;

    const auto is_battle_start_blink_ability = [&](const AbilityData& ability)
    {
        if (ability.activation_trigger_data.trigger_type == ActivationTriggerType::kOnBattleStart)
        {
            for (const auto& skill : ability.skills)
            {
                for (const auto& effect : skill->effect_package.effects)
                {
                    if (effect.type_id.type == EffectType::kBlink)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    };

    // Prevent double blink on battle start
    if (is_battle_start_blink_ability(*ability_data))
    {
        for (size_t index = 0; index != abilities_data.abilities.size(); ++index)
        {
            const auto& existing_ability = abilities_data.abilities[index];
            if (is_battle_start_blink_ability(*existing_ability))
            {
                return;
            }
        }
    }

    abilities_data.abilities.push_back(ability_data);
    InitializeInnateAbilitiesStateForNewAbilities();
    RebuildTriggerableAbilitiesMap();
}

void AbilitiesComponent::RemoveInnateAbilityByAttachedEntityID(const EntityID attached_entity_id)
{
    RemoveInnateAbilityIf(
        [&](const AbilityData& ability_data)
        {
            return ability_data.attached_from_entity_id == attached_entity_id;
        });
}

void AbilitiesComponent::RemoveInnateAbilityIf(const std::function<bool(const AbilityData&)>& predicate)
{
    auto& innates = GetAbilities(AbilityType::kInnate);
    const size_t prev_size = innates.data.abilities.size();
    for (size_t index = 0; index != prev_size; ++index)
    {
        auto& ability_data = innates.data.abilities[index];
        if (predicate(*ability_data))
        {
            ability_data = nullptr;
            innates.state.abilities[index] = nullptr;
        }
    }

    // Remove from queues
    auto EraseValuesFromQueueForCondition = [this, &predicate](std::deque<AbilityStatePtr>& queue)
    {
        for (auto it = queue.begin(); it != queue.end();)
        {
            AbilityStatePtr ability_state_ptr = *it;
            if (ability_state_ptr == nullptr || ability_state_ptr->data == nullptr ||
                predicate(*ability_state_ptr->data))
            {
                it = queue.erase(it);  // erase() returns iterator to next element
                innate_abilities_waiting_activation_set_.erase(ability_state_ptr);
            }
            else
            {
                ++it;
            }
        }
    };
    EraseValuesFromQueueForCondition(innate_abilities_waiting_activation_);
    EraseValuesFromQueueForCondition(instant_innate_abilities_waiting_activation_);

    VectorHelper::EraseValuesForCondition(
        innates.data.abilities,
        [](const std::shared_ptr<const AbilityData>& ability)
        {
            return ability == nullptr;
        });

    VectorHelper::EraseValuesForCondition(
        innates.state.abilities,
        [](const AbilityStatePtr& state)
        {
            return state == nullptr;
        });

    const size_t new_size = innates.data.abilities.size();
    if (new_size != prev_size)
    {
        // Update ability index in ability state
        for (size_t index = 0; index != new_size; ++index)
        {
            innates.state.abilities[index]->index = index;
        }

        RebuildTriggerableAbilitiesMap();
    }
}

bool AbilitiesComponent::AddInnateWaitingActivation(const AbilityStatePtr& ability)
{
    // Already exists
    if (innate_abilities_waiting_activation_set_.contains(ability))
    {
        return false;
    }

    if (ability->IsInstant())
    {
        instant_innate_abilities_waiting_activation_.push_back(ability);
    }
    else
    {
        innate_abilities_waiting_activation_.push_back(ability);
    }

    innate_abilities_waiting_activation_set_.insert(ability);
    return true;
}

AbilityStatePtr AbilitiesComponent::PopInnateWaitingActivation()
{
    if (innate_abilities_waiting_activation_.empty())
    {
        return nullptr;
    }

    AbilityStatePtr ability = innate_abilities_waiting_activation_.front();

    // Remove from queue
    innate_abilities_waiting_activation_.pop_front();
    innate_abilities_waiting_activation_set_.erase(ability);

    return ability;
}

AbilityStatePtr AbilitiesComponent::PopInstantInnateWaitingActivation()
{
    AbilityStatePtr ability = GetInstantInnateWaitingActivation();
    if (!ability)
    {
        return nullptr;
    }

    // Remove from queue
    instant_innate_abilities_waiting_activation_.pop_front();
    innate_abilities_waiting_activation_set_.erase(ability);

    return ability;
}

bool AbilityState::CanActivateAbilityByTriggerCounter() const
{
    const auto& activation_trigger_data = data->activation_trigger_data;
    if (activation_trigger_data.trigger_type != ActivationTriggerType::kOnVanquish)
    {
        return true;
    }

    if (activation_trigger_data.every_x)
    {
        if (trigger_counter % activation_trigger_data.trigger_value != 0)
        {
            return false;
        }
    }
    else
    {
        if (!EvaluateComparison(
                activation_trigger_data.comparison_type,
                FixedPoint::FromInt(trigger_counter),
                FixedPoint::FromInt(activation_trigger_data.trigger_value)))
        {
            return false;
        }
    }

    return true;
}

void AbilityStateActivatorContext::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("ability_type", ability_type);

    if (sender_entity_id != simulation::kInvalidEntityID)
    {
        h.Field("sender_entity_id", sender_entity_id);
        h.Field("sender_combat_unit_entity_id", sender_combat_unit_entity_id);
    }

    if (receiver_entity_id != simulation::kInvalidEntityID)
    {
        h.Field("receiver_entity_id", receiver_entity_id);
        h.Field("receiver_combat_unit_entity_id", receiver_combat_unit_entity_id);
    }

    h.Write("}}");
}

void AbilityState::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("index", index);
    h.Field("ability_type", ability_type);
    h.Field("is_active", is_active);
    h.Field("current_ability_time_ms", current_ability_time_ms);
    h.Field("total_duration_ms", total_duration_ms);
    h.FieldIf(ability_type == AbilityType::kAttack, "previous_overflow_ms", previous_overflow_ms);
    h.Field("activation_count", activation_count);

    if (data)
    {
        h.Field("data", *data);
    }
    else
    {
        h.Field("data", "MISSING DATA FIELD");
    }

    // NOTE: This is very spammy
    // h.FieldIf(!activator_contexts.empty(), "activator_contexts", activator_contexts);
    h.Write("}}");
}

void AbilityTypeContextFormat::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("ability_type", state.ability_type);
    h.EnumFieldIfNotNone("trigger_type", state.data->activation_trigger_data.trigger_type);
    h.Write("}}");
}

}  // namespace simulation
