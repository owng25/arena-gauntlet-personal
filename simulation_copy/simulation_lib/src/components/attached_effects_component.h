#pragma once

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "data/constants.h"
#include "data/effect_data.h"
#include "ecs/component.h"
#include "utility/custom_formatter.h"
#include "utility/fixed_point.h"
#include "utility/time.h"

namespace simulation
{

class AttachedEffectState;
using AttachedEffectStatePtr = std::shared_ptr<AttachedEffectState>;

// A single attached effect state
class AttachedEffectState final : public std::enable_shared_from_this<AttachedEffectState>
{
public:
    struct ApplyOverTime
    {
        // Keep track of the current applied total value.
        FixedPoint current_value = 0_fp;

        // Final total value of the applied effect we want reach
        // For example if we do 250 pure damage over 8 seconds this should
        // have the value of 250
        FixedPoint total_value = 0_fp;

        // Actual value we should apply per frequency time step
        // (total_value * frequency_time_steps) / duration_time_steps
        FixedPoint value_per_frequency_time_step = 0_fp;
    };

    // Create a new instance
    static AttachedEffectStatePtr
    Create(const EntityID sender_id, const EffectData& effect_data, const EffectState& effect_state)
    {
        auto ptr = std::make_shared<AttachedEffectState>();
        ptr->sender_id = sender_id;
        // NOTE: combat_unit_sender_id is set on AttachedEffectsHelper::AddAttachedEffect
        ptr->effect_data = effect_data;
        ptr->effect_state = effect_state;

        // Add the context for this effect
        ptr->effect_state.source_context.Add(SourceContextType::kAttachedEffect);

        // Clearing effect_state.effect_package_attributes to avoid gaining one bonus a few time for
        // "over time" group of effects
        ptr->effect_state.effect_package_attributes = {};
        ptr->effect_state.effect_package_attributes.vampiric_percentage =
            effect_state.effect_package_attributes.vampiric_percentage;

        ptr->UpdateCachedValues();

        return ptr;
    }

    // Evalutes effect expressions and saves result into captured_effect_value variable.
    // Currently done by buffs and debuffs. Depends on which kind of buff is being processed,
    // this method will either use actual live stats (for static buffs) or previous
    // live stats taken from the world cache (dynamic buffs)
    void CaptureEffectValue(const World& world, const EntityID receiver_id);

    // Updates all the cached conversion from ms to time steps
    void UpdateCachedValues()
    {
        duration_time_steps = Time::MsToTimeSteps(effect_data.lifetime.duration_time_ms);
        frequency_time_steps = Time::MsToTimeSteps(effect_data.lifetime.frequency_time_ms);
        blink_delay_time_steps = Time::MsToTimeSteps(effect_data.blink_delay_ms);
    }

    // Resets the current duration/activations
    void ResetCurrentLifetime()
    {
        current_time_steps = 0;
        current_blocks = 0;
        current_activations = 0;
        total_activations_lifetime = 0;
    }

    // Is this attached event expired?
    constexpr bool IsExpired() const
    {
        // Check consumable first
        if (IsConsumable())
        {
            if (effect_data.lifetime.activations_until_expiry == kTimeInfinite)
            {
                // Lives forever
                return false;
            }

            // Expired
            if (current_activations >= effect_data.lifetime.activations_until_expiry)
            {
                return true;
            }
        }

        // check blocks count for effect ability block
        if (effect_data.lifetime.blocks_until_expiry != kTimeInfinite &&
            current_blocks >= effect_data.lifetime.blocks_until_expiry)
        {
            return true;
        }

        // Check duration
        if (duration_time_steps == kTimeInfinite)
        {
            // Lives forever
            return false;
        }

        return current_time_steps >= duration_time_steps;
    }

    // Can this consumable attachable effect be activated?
    constexpr bool CanActivateConsumable() const
    {
        return total_activations_lifetime % effect_data.lifetime.consumable_activation_frequency == 0;
    }

    constexpr bool IsConsumable() const
    {
        return effect_data.lifetime.is_consumable;
    }

    // Helper method to add a child to this effect type
    void AddChild(const AttachedEffectStatePtr& child_effect)
    {
        children.push_back(child_effect);
        child_effect->parent = weak_from_this();
    }

    // Does this effect have no parent?
    bool IsRootType() const
    {
        return parent.expired();
    }

    // Is this going to be destroyed?
    bool IsValid() const
    {
        return state != AttachedEffectStateType::kDestroyed;
    }

    // Can this effect type be cleansed
    // NOTE: Only root attached effects can be cleansed
    bool CanCleanse() const
    {
        return IsRootType() && effect_data.can_cleanse;
    }

    void FormatTo(fmt::format_context& ctx) const;

    FixedPoint GetCapturedValue() const
    {
        return captured_effect_value.value_or(0_fp);
    }

    // The Effect package data
    EffectData effect_data{};

    // The state of the effect data
    EffectState effect_state{};

    // Keeps track of all the previous stack values
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/280330241/Stacking+Processing
    std::vector<FixedPoint> history_all_stack_values;

    // An attached effect can spawn other attached effects, keep track of them
    std::vector<AttachedEffectStatePtr> children;

    // Data only use if apply_over_time = true;
    ApplyOverTime apply_over_time_data{};

    // Parent of this effect if any
    std::weak_ptr<AttachedEffectState> parent;

    // Track the most recent change to the value, if any
    FixedPoint last_delta_value = 0_fp;

    // Here buffs and debuffs save the result of evaluated expression
    std::optional<FixedPoint> captured_effect_value;

    // The id of the entity which applies this effect on us
    // If invalid this defaults to self
    // NOTE: This can be the id of a Zone/Projectile/Combat unit
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who attached this effect
    // NOTE: This can equal to the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The current state of this effect
    AttachedEffectStateType state = AttachedEffectStateType::kPendingActivation;

    // Total Duration of the effect in time steps (kTimeInfinite = infinite).
    // NOTE: this is the same as EffectData::duration_time_ms but converted here to time steps
    // for caching purposes
    int duration_time_steps = kTimeInfinite;

    // The current stack count used
    int current_stacks_count = 1;

    // Duration after which the effect will be sent
    int frequency_time_steps = Time::MsToTimeSteps(kDefaultAttachedEffectsFrequencyMs);

    // Delay for blink in time steps
    int blink_delay_time_steps = 0;

    // Current time passed in time steps
    int current_time_steps = 0;

    // Current amount of effect package blocks done
    int current_blocks = 0;

    // Current amount of activations done
    // Only used if effect_data.lifetime.is_consumable is true
    int current_activations = 0;

    // Current total number of activations this effect live through
    // Most of the time this is equal to current_activations, but if activation_frequency != 1 then
    // this total_activations_lifetime > current_activations
    int total_activations_lifetime = 0;

    // -1 means destroy at the same time step when marked for delete
    int time_step_when_destroy = -1;

    // Is the current state data initialized?
    bool is_data_initialized = false;

    // If this is true the effect is not a shot and forget event
    // We must apply this effect over time.
    // Examples: Damage over time, buffs, debuffs.
    bool apply_over_time = false;

    // If this is true, then we don't need to apply the effect, it is already applied
    // Positive states/negative states for example
    bool is_default_applied = false;

    // What it says, see  AttachedEffectsSystem::OnAbilityDeactivated
    bool can_be_consumed_by_ability_deactivations = true;
};

// Struct that holds the current state for all the buffs/debuffs
struct AttachedEffectBuffsState
{
    // Keep track for buffs for each StatType
    // Key: StatType
    // Value: All the active buffs for this StatType
    std::unordered_map<StatType, std::vector<AttachedEffectStatePtr>> buffs;

    // Keep track for buffs for each StatType
    // Key: StatType
    // Value: All the active debuffs for this StatType
    std::unordered_map<StatType, std::vector<AttachedEffectStatePtr>> debuffs;
};

/* -------------------------------------------------------------------------------------------------------
 * AttachEffectsComponent
 *
 * This class holds the details of attached effects
 * --------------------------------------------------------------------------------------------------------
 */
class AttachedEffectsComponent : public Component
{
public:
    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        // NOTE: Do not copy anything from this component, as we are only interested in the initial state which should
        // be empty
        return std::make_shared<AttachedEffectsComponent>();
    }

    // Returns the attached effects
    const std::vector<AttachedEffectStatePtr>& GetAttachedEffects() const
    {
        return attached_effects_;
    }

    // Gets all the effects of this type grouped by the ability name.
    //
    // Return:
    // - out_same_abilities - Map group of all similar abilities
    //      Key: Ability Name
    //      Value: Array of at least 2 effects that come from the same Ability Name
    // - out_different_abilities - Array of effects each that come from a unique ability name
    //
    // NOTE: This only gets the root effects aka effects without any parents, as the lifetime of the leaf nodes is
    // handled by the parent
    size_t GetAllRootEffectsOfTypeIDPerAbilityName(
        const EffectTypeID& effect_type_id,
        std::map<std::string_view, std::vector<AttachedEffectStatePtr>>* out_same_abilities,
        std::vector<AttachedEffectStatePtr>* out_different_abilities) const;

    // Adds the attached effect into the active container
    // NOTE: It is assumed that this effect already exists inside the attached_effects_ vector
    void AddAttachedEffectToActive(const AttachedEffectStatePtr& attached_effect);

    // Removes the attached effect from the active container
    // NOTE: It is assumed that this effect already exists inside the attached_effects_ vector
    void RemoveAttachedEffectFromActive(const AttachedEffectStatePtr& attached_effect);

    // Does the attached effect exist in the active container
    bool IsAttachedEffectInActive(const AttachedEffectStatePtr& attached_effect) const;

    // Gets all cleansable all Negative States attached effects
    std::vector<AttachedEffectStatePtr> GetAllCleansableNegativeStates() const;

    // Gets all cleansable debuffs
    std::vector<AttachedEffectStatePtr> GetAllCleansableDebuffs() const;

    // Gets all cleansable DOT conditions
    std::vector<AttachedEffectStatePtr> GetAllCleansableConditions() const;

    // GEts all cleansable DOTs
    std::vector<AttachedEffectStatePtr> GetAllCleansableDOTs() const;

    // Gets all cleansable energy burn over time effects
    std::vector<AttachedEffectStatePtr> GetAllCleansableEnergyBurnOverTimes() const;

    void EraseDestroyedEffects(const bool always_destroy_delayed = false);

    // Mark all Negative States of type for destruction in the next time step
    void RemoveNegativeStateNextTimeStep(const EffectNegativeState negative_state);

    // Check if this attached effects component has an active energy burn
    bool HasEnergyBurn() const
    {
        return active_energy_burns_over_time_.size() > 0;
    }

    // Check if there is an active displacement
    bool HasDisplacement() const
    {
        for (const auto& attached_effect : attached_effects_)
        {
            if (attached_effect->effect_data.type_id.type == EffectType::kDisplacement &&
                attached_effect->state != AttachedEffectStateType::kDestroyed)
            {
                return true;
            }
        }

        return false;
    }

    // Check if there is an active displacement
    bool HasDisplacementType(EffectDisplacementType type) const
    {
        for (const auto& attached_effect : attached_effects_)
        {
            if (attached_effect->effect_data.type_id.type == EffectType::kDisplacement &&
                attached_effect->effect_data.type_id.displacement_type == type &&
                attached_effect->state != AttachedEffectStateType::kDestroyed)
            {
                return true;
            }
        }

        return false;
    }

    // Check if there is an active execute effect
    bool HasExecute() const
    {
        return active_executes_.size() > 0;
    }

    // Check if there is an active blink effect
    bool HasBlink() const
    {
        return active_blinks_.size() > 0;
    }

    // Gets the state of all the buffs and debuffs
    const AttachedEffectBuffsState& GetBuffsState() const
    {
        return active_buffs_;
    }

    // Gets all the active positive states
    const std::unordered_map<EffectPositiveState, std::vector<AttachedEffectStatePtr>>& GetPositiveStates() const
    {
        return active_positive_states_;
    }

    // Gets all the active negative states
    const std::unordered_map<EffectNegativeState, std::vector<AttachedEffectStatePtr>>& GetNegativeStates() const
    {
        return active_negative_states_;
    }

    // Gets all the active plane changes
    const std::unordered_map<EffectPlaneChange, std::vector<AttachedEffectStatePtr>>& GetPlaneChanges() const
    {
        return active_plane_changes_;
    }

    // Gets all the active conditions
    const std::unordered_map<EffectConditionType, std::vector<AttachedEffectStatePtr>>& GetActiveConditions() const
    {
        return active_conditions_;
    }

    // Gets all the active empowers
    const std::vector<AttachedEffectStatePtr>& GetEmpowers() const
    {
        return active_empowers_;
    }

    // Gets all the active disempowers
    const std::vector<AttachedEffectStatePtr>& GetDisempowers() const
    {
        return active_disempowers_;
    }

    // Gets all the active Dots
    const std::vector<AttachedEffectStatePtr>& GetDots() const
    {
        return active_damages_over_time_;
    }

    // Gets all the active burn over time effects
    const std::vector<AttachedEffectStatePtr>& GetBots() const
    {
        return active_energy_burns_over_time_;
    }

    // Gets all the active execute effects
    const std::vector<AttachedEffectStatePtr>& GetExecutes() const
    {
        return active_executes_;
    }

    // Check if this attached effects component has an active positive state of this type
    bool HasPositiveState(const EffectPositiveState positive_state) const
    {
        return active_positive_states_.count(positive_state) > 0;
    }

    // Check if this has an active negative state of this type
    bool HasNegativeState(const EffectNegativeState negative_state) const
    {
        return active_negative_states_.count(negative_state) > 0;
    }

    // Check if this has an active plane change of this type
    bool HasPlaneChange(const EffectPlaneChange plane_change) const
    {
        return active_plane_changes_.count(plane_change) > 0;
    }

    // Check if this has an active condition of this type
    bool HasCondition(const EffectConditionType condition_type) const
    {
        return active_conditions_.count(condition_type) > 0;
    }

    // Checks if this has any buff for this stat type
    bool HasBuffFor(const StatType type) const
    {
        return active_buffs_.buffs.count(type) > 0 && !active_buffs_.buffs.at(type).empty();
    }

    // Checks if this has any debuff for this stat type
    bool HasDebuffFor(const StatType type) const
    {
        return active_buffs_.debuffs.count(type) > 0 && !active_buffs_.debuffs.at(type).empty();
    }

    bool HasImmunityTo(const EffectTypeID& type_id) const;

    bool HasImmunityToNegativeState(EffectNegativeState state) const;

    bool HasNonImmunizedNegativeState(EffectNegativeState state) const
    {
        return HasNegativeState(state) && !HasImmunityToNegativeState(state);
    }

    bool HasImmunityToAllDetrimentalEffects() const;

    // NOTE: These are public because we don't want to create getters/setters for everything

    // Currently attached effects
    std::vector<AttachedEffectStatePtr> attached_effects_;

    // Keep track of all the active positive states on this entity
    std::unordered_map<EffectPositiveState, std::vector<AttachedEffectStatePtr>> active_positive_states_;

    // Keep track of all the active negative states on this entity
    std::unordered_map<EffectNegativeState, std::vector<AttachedEffectStatePtr>> active_negative_states_;

    // Keep track of all the active plane changes on this entity
    std::unordered_map<EffectPlaneChange, std::vector<AttachedEffectStatePtr>> active_plane_changes_;

    // Keep track of all the active conditions on this entity
    std::unordered_map<EffectConditionType, std::vector<AttachedEffectStatePtr>> active_conditions_;

    // Keep track of all the active empowers
    std::vector<AttachedEffectStatePtr> active_empowers_;

    // Keep track of all the active disempowers
    std::vector<AttachedEffectStatePtr> active_disempowers_;

    // Keep track of all the DOT effects
    std::vector<AttachedEffectStatePtr> active_damages_over_time_;

    // Keep track of all the energy burn over time effects
    std::vector<AttachedEffectStatePtr> active_energy_burns_over_time_;

    // Keep track of all the active heal over time effects
    std::vector<AttachedEffectStatePtr> active_hots_;

    // Keep track of all the active pure heal over time effects
    std::vector<AttachedEffectStatePtr> active_pure_hots_;

    // Keep track of all the active energy gain over time effects
    std::vector<AttachedEffectStatePtr> active_energy_gains_over_time_;

    // Keep track of all the active hyper gain over time effects
    std::vector<AttachedEffectStatePtr> active_hyper_gains_;

    // Keep track of all the active hyper burn over time effects
    std::vector<AttachedEffectStatePtr> active_hyper_burns_;

    // Keep track of all the active buffs/debuffs
    AttachedEffectBuffsState active_buffs_;

    // Keep track of all the execute effects
    std::vector<AttachedEffectStatePtr> active_executes_;

    // Keep track of all blink effects
    std::vector<AttachedEffectStatePtr> active_blinks_;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(AttachedEffectState, FormatTo);
