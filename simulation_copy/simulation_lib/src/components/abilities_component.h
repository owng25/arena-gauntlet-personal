#pragma once

#include <map>
#include <queue>
#include <set>

#include "data/ability_data.h"
#include "data/constants.h"
#include "data/enums.h"
#include "ecs/component.h"
#include "utility/custom_formatter.h"
#include "utility/enum_map.h"
#include "utility/math.h"
#include "utility/targeting_helper.h"
#include "utility/time.h"

namespace simulation
{
// Tells us in what state the current skill is in
enum class SkillStateType
{
    kNone = 0,
    kWaiting,
    kDeploying,
    kChanneling,
    kFinished
};

// Holds the current state of targeting
struct SkillTargetingState
{
    // Valid entity targets
    std::set<EntityID> available_targets;

    // Saved positions of available targets
    std::map<EntityID, HexGridPosition> targets_saved_data;

    // Caching value of TargetingHelper::SkillTargetFindResult true_sender_id
    EntityID true_sender_id = kInvalidEntityID;

    // Clear targeting state
    void Clear()
    {
        available_targets.clear();
        targets_saved_data.clear();
        true_sender_id = kInvalidEntityID;
    }

    // Check whether is targeting valid for a deployment type
    bool IsValidFor(const SkillDeploymentType deployment_type) const
    {
        if (deployment_type == SkillDeploymentType::kDash)
        {
            // Dash seems always have a valid targeting
            return true;
        }

        // By default targeting state is valid when we have at least one entity or its last known location
        return !available_targets.empty() || !targets_saved_data.empty();
    }

    // Merge new valid entities to current targeting state
    void MergeValidTargets(
        const World& world,
        const TargetingHelper::SkillTargetFindResult& find_result,
        const size_t max_targets);

    // Create a new targeting state from find result
    void CreateFromFindResult(const World& world, const TargetingHelper::SkillTargetFindResult& find_result);

    // Create a new targeting state from single target
    void CreateFromTarget(const World& world, const EntityID receiver_id, const EntityID sender_id)
    {
        const TargetingHelper::SkillTargetFindResult target{{receiver_id}, sender_id};
        CreateFromFindResult(world, target);
    }

    // Get last known locations of lost targeted entities
    std::vector<HexGridPosition> GetLostTargetsLastKnownPosition() const
    {
        std::vector<HexGridPosition> locations;
        for (const auto& [entity, location] : targets_saved_data)
        {
            // The target entity may be considered lost if that was erased from available targets
            if (available_targets.count(entity) == 0)
            {
                locations.emplace_back(location);
            }
        }

        // We need to sort values to keep deterministic behavior
        std::sort(locations.begin(), locations.end());

        return locations;
    }

    // Get targeted entities
    std::vector<EntityID> GetAvailableTargetsVector() const
    {
        // Return new vector created from the available targets set
        return std::vector<EntityID>(available_targets.begin(), available_targets.end());
    }

    size_t GetTotalTargetsSize() const
    {
        return targets_saved_data.size();
    }

private:
    void SaveTargetData(const World& world, const EntityID target_id);
};

// Holds the current state for a skill
struct SkillState
{
    // Update the cached timer time steps from the ability total_duration_ms
    void UpdateTimers(const int total_duration_ms, const bool truncate_times_to_nearest_time_step)
    {
        // Calculate the duration of the skill based on the percentage of the ability total
        // duration
        duration_ms = Math::PercentageOf(data->percentage_of_ability_duration, total_duration_ms);

        // Calculate pre deployment delay
        pre_deployment_delay_ms = Math::PercentageOf(data->deployment.pre_deployment_delay_percentage, duration_ms);

        // Calculate pre deployment retargeting frame
        pre_deployment_retargeting_ms =
            Math::PercentageOf(data->deployment.pre_deployment_retargeting_percentage, pre_deployment_delay_ms);

        if (truncate_times_to_nearest_time_step)
        {
            duration_ms = Time::TimeStepsToMs(Time::MsToTimeSteps(duration_ms));
            pre_deployment_delay_ms = Time::TimeStepsToMs(Time::MsToTimeSteps(pre_deployment_delay_ms));
            pre_deployment_retargeting_ms = Time::TimeStepsToMs(Time::MsToTimeSteps(pre_deployment_retargeting_ms));
        }

        // Calculate channel time and clamp to fit in duration
        channel_time_ms = std::clamp(data->channel_time_ms, 0, duration_ms - pre_deployment_delay_ms);
    }
    void ResetState()
    {
        state = SkillStateType::kNone;
        is_deployed = false;
    }

    // Is this an instant skill, all happens in the same time step
    // This means this does not have any duration
    bool IsInstant() const
    {
        return duration_ms == 0;
    }

    // Checks if skill has valid targeting state for given deployment
    bool HasValidTargeting() const
    {
        return targeting_state.IsValidFor(data->deployment.type);
    }

    // A skill is critical if:
    // - skill.is_critical - this was rolled inside Activate
    // - skill.data->is_critical - skill was predefined as it should crit
    // - EffectPackage.AlwaysCrit - said it should always crit
    bool CheckAllIfIsCritical() const
    {
        return is_critical || data->is_critical || data->effect_package.attributes.always_crit;
    }

    // Current index inside the vector
    size_t index = kInvalidIndex;

    // Current state
    SkillStateType state = SkillStateType::kNone;

    // pre_deployment_delay_percentage converted into time
    int pre_deployment_delay_ms = 0;

    // pre_deployment_retargeting_percentage converted into time
    int pre_deployment_retargeting_ms = 0;

    // Timers
    int duration_ms = 0;
    int channel_time_ms = 0;

    // Was this skill deployed?
    bool is_deployed = false;

    // Is this skill a critical?
    bool is_critical = false;

    // Current targeting state of skill
    SkillTargetingState targeting_state;

    // Point to the original data
    std::shared_ptr<SkillData> data;
};

// Helper struct to figure out the activator context
struct AbilityStateActivatorContext
{
    // The entity who activated this (can be anything)
    EntityID sender_entity_id = kInvalidEntityID;

    // The combat unit entity who activated this.
    // For spawned entities, this is the parent of the entity_id most likely
    EntityID sender_combat_unit_entity_id = kInvalidEntityID;

    // The entity which is a target of event which activates this trigger (can be anything)
    EntityID receiver_entity_id = kInvalidEntityID;

    // The entity which is a target of event which activates this trigger (can be anything)
    // For spawned entities, this is the parent of the entity_id most likely
    EntityID receiver_combat_unit_entity_id = kInvalidEntityID;

    // The ability type being activated
    AbilityType ability_type = AbilityType::kNone;

    // Event that caused this activation (optional)
    Event event = Event(EventType::kNone);

    void FormatTo(fmt::format_context& ctx) const;
};

class AbilityState;
using AbilityStatePtr = std::shared_ptr<AbilityState>;

class AttachedEffectState;

// Holds the current state of the ability
class AbilityState
{
public:
    // Create a new instance
    static AbilityStatePtr Create()
    {
        return std::make_shared<AbilityState>();
    }

    // Update the cached timers
    void UpdateTimers(const int total_duration_ms);

    // Activate this ability
    void Activate(const int time_step);

    // Deactivate this ability
    void Deactivate()
    {
        is_active = false;
        applied_effect_package_attributes = false;
    }

    // Ability has finished, should most likely be deactivated
    bool IsFinished() const
    {
        return current_skill_index >= skills.size();
    }

    // Helper method to get the current skill state
    SkillState& GetCurrentSkillState()
    {
        return skills.at(current_skill_index);
    }
    const SkillState& GetCurrentSkillState() const
    {
        return skills.at(current_skill_index);
    }

    // Can the current skill get to the kDeploying state in the same time step?
    // Basically skipping the kWaiting state
    bool CanDeployCurrentSkillInstantly() const
    {
        if (IsFinished())
        {
            return false;
        }

        // Can this skill get to the kDeploying state in the same time step?
        // Basically skipping the kWaiting state
        // This is for when the previous attack overran by so much that
        // we've gone past the pre deploy time of the new attack
        // NOTE: also equivallent when comparing to just pre_deployment_delay_ms == 0
        const SkillState& skill_state = GetCurrentSkillState();
        return current_skill_time_ms >= skill_state.pre_deployment_delay_ms;
    }

    // Can the current skill get into the kDeploying state
    bool CanDeployCurrentSkill() const
    {
        if (IsFinished())
        {
            return false;
        }

        // Ability current time is over the total
        if (current_ability_time_ms >= total_duration_ms)
        {
            return false;
        }

        // Check skill
        return current_skill_time_ms < GetCurrentSkillState().duration_ms;
    }

    // Can the current skill get into the kChanneling state
    bool CanChannelCurrentSkill() const
    {
        if (IsFinished())
        {
            return false;
        }

        // Ability current time is over the total
        if (current_ability_time_ms >= total_duration_ms)
        {
            return false;
        }

        // Can not even channel
        const SkillState& skill_state = GetCurrentSkillState();
        if (skill_state.channel_time_ms <= 0)
        {
            return false;
        }

        const int channel_expire_time_ms = skill_state.channel_time_ms + skill_state.pre_deployment_delay_ms;
        return current_skill_time_ms < channel_expire_time_ms;
    }

    // Can the current skill find a new target
    bool CanRetargetCurrentSkill() const
    {
        if (IsFinished())
        {
            return false;
        }

        // Ability current time is over the total
        if (current_ability_time_ms >= total_duration_ms)
        {
            return false;
        }

        return CanAlwaysRetarget() || current_skill_time_ms <= GetCurrentSkillState().pre_deployment_retargeting_ms;
    }

    // Can the current skill always retarget its skill
    bool CanAlwaysRetarget() const
    {
        // Targeting type in zone can always retarget
        if (GetCurrentSkillState().data->targeting.type == SkillTargetingType::kInZone)
        {
            return true;
        }

        // At 100 % we always allow to retarget
        return GetCurrentSkillState().data->deployment.pre_deployment_retargeting_percentage == kMaxPercentage;
    }

    // Is this an instant ability, all happens in the same time step
    // This means this does not have any duration
    bool IsInstant() const
    {
        return total_duration_ms == 0;
    }

    // Helper to determine if this ability has skills with movement
    bool HasSkillWithMovement() const;

    void IncreaseCurrentTimeMs(const int added_time_ms)
    {
        current_ability_time_ms += added_time_ms;
        current_skill_time_ms += added_time_ms;
        total_current_time_ms += added_time_ms;
    }
    void IncrementCurrentSkillIndex()
    {
        current_skill_index++;
        current_skill_time_ms = 0;
    }
    size_t GetCurrentSkillIndex() const
    {
        return current_skill_index;
    }

    // Time the current skill has been around
    int GetCurrentSkillTimeMs() const
    {
        return current_skill_time_ms;
    }

    // Time the current ability has been around
    int GetCurrentAbilityTimeMs() const
    {
        return current_ability_time_ms;
    }

    void SetCurrentAbilityTimeMs(const int new_time_ms)
    {
        current_skill_time_ms = new_time_ms;
        current_ability_time_ms = new_time_ms;
    }

    // Check that counter condition is satisfied
    bool CanActivateAbilityByTriggerCounter() const;

    void FormatTo(fmt::format_context& ctx) const;

    // Current index inside the vector
    size_t index = kInvalidIndex;

    // Is this ability active?
    bool is_active = false;

    // Type of the ability being used
    AbilityType ability_type = AbilityType::kNone;

    // Total time, see AbilityData
    int total_duration_ms = 0;

    // How many times this ability has activated
    int activation_count = 0;

    // The time step this ability was last activated at
    int last_activation_time_step = 0;

    // The last time the ability was activated in ms
    int last_activation_ms = 0;

    // Activation time limit in TimeSteps
    // NOTE: Only used for innate abilities
    int activation_time_limit_time_steps = 0;

    // At what time interval should this ability activate
    // NOTE: Only used for innate abilities
    int activate_every_time_steps = 0;

    // How many time steps are required to pass between activations
    int activation_cooldown_time_steps = 0;

    // Skills for this ability
    std::vector<SkillState> skills;

    // Point to the original data
    std::shared_ptr<const AbilityData> data;

    // Internal counter to keep track of the total current_time_steps
    int total_current_time_ms = 0;

    // Overflow time of previous attack
    int previous_overflow_ms = 0;

    // Keep track of all the activator contexts
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/256379211/Activator+List
    std::vector<AbilityStateActivatorContext> activator_contexts;

    // Indicates if the effect package attributes were already applied for a skill
    bool applied_effect_package_attributes = false;

    // Stores a number of potential trigger activation that met all trigger conditions
    // except comparison on this counter. For Innates only
    int trigger_counter = 0;

    // Stores the set of empowers that were actually used by the current ability activation
    std::unordered_set<AttachedEffectState*> consumable_empowers_used;

protected:
    // Time this ability has been around
    int current_ability_time_ms = 0;

    // Time the current skill has been around
    // NOTE: for an ability with just one skill this corresponds to the current_time_ms
    int current_skill_time_ms = 0;

    // The current skill index we are currently executing
    size_t current_skill_index = 0;
};

// A vector of state abilities that also specifies the current state index
struct AbilitiesState
{
    // The list of abilities
    // State data for all the attack abilities
    // Maps from index (from data_abilities_.abilities) => AbilityState
    std::vector<AbilityStatePtr> abilities;

    // The current chosen ability index
    // Default: kInvalidIndex as we will set it on the first iteration in ChooseAbility
    size_t current_ability_index = kInvalidIndex;

    // Keeps track of how many times an ability of this category has activated for EveryXActivations
    int total_activations_count = 0;
};

/* -------------------------------------------------------------------------------------------------------
 * AbilitiesComponent
 *
 * This class holds the attack and omega abilities data for an entity
 * --------------------------------------------------------------------------------------------------------
 */
class AbilitiesComponent : public Component
{
public:
    // Statistics per ability type
    struct AbilityTypeStats
    {
        // Number of received effect packages from ability of specified type
        int effect_packages_received = 0;

        // Number of deployed skills from ability of specified type
        int skills_deployed = 0;
    };
    struct AbilityTypeInfo
    {
        // Abilities data
        AbilitiesData data;

        // Abilities actual state
        AbilitiesState state;
    };

    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override;
    void Init() override {}

    // Get/Set data for the attack abilities
    const AbilitiesData& GetDataAttackAbilities() const
    {
        return GetAbilities(AbilityType::kAttack).data;
    }

    void SetAbilitiesData(const AbilitiesData& data, const AbilityType ability_type)
    {
        AbilityTypeInfo& info = GetAbilities(ability_type);
        info.data = data;

        InitializeAbilitiesStateFromData(info.data, ability_type, &info.state);

        if (ability_type == AbilityType::kInnate)
        {
            RebuildTriggerableAbilitiesMap();
        }
    }

    // Get/Set data for the omega abilities
    const AbilitiesData& GetDataOmegaAbilities() const
    {
        return GetAbilities(AbilityType::kOmega).data;
    }

    // Wherever there is an active ability
    bool HasActiveAbility() const
    {
        return active_ability_ != nullptr;
    }

    AbilityType GetActiveAbilityType() const
    {
        return active_ability_ ? active_ability_->ability_type : AbilityType::kNone;
    }

    size_t GetActiveAbilityIndex() const
    {
        return active_ability_->index;
    }

    bool HasAbility(const AbilityStatePtr& ability) const
    {
        auto& abilities = GetAbilities(ability->ability_type);
        return ability->index < abilities.state.abilities.size() &&
               ability == abilities.state.abilities[ability->index];
    }

    void SetActiveAbility(AbilityStatePtr ability)
    {
        assert(ability == nullptr || HasAbility(ability));
        active_ability_ = std::move(ability);
    }

    // Is the active ability an omega
    bool HasOmegaActiveAbility() const
    {
        return HasActiveAbility() && GetActiveAbilityType() == AbilityType::kOmega;
    }

    // Is the movement locked by active ability
    bool IsMovementLocked() const
    {
        // Locked when we have active ability and movement_lock is true
        return HasActiveAbility() && GetActiveAbility()->data->movement_lock;
    }

    // Gets the current active ability state data if any
    AbilityStatePtr GetActiveAbility() const
    {
        if (!HasActiveAbility())
        {
            return nullptr;
        }

        const AbilitiesState& active_abilities_state = GetActiveAbilitiesState();
        if (active_ability_->index >= active_abilities_state.abilities.size())
        {
            assert(false);
            return nullptr;
        }

        return active_abilities_state.abilities.at(active_ability_->index);
    }

    // Gets the current active skill state data if any
    SkillState* GetActiveSkill() const
    {
        const auto active_ability = GetActiveAbility();
        if (active_ability == nullptr || active_ability->IsFinished())
        {
            return nullptr;
        }

        return &active_ability->GetCurrentSkillState();
    }

    // Chooses a attack ability and returns the state data for it
    // The chosen ability depends on the selection type for the attack abilities
    AbilityStatePtr ChooseAttackAbility() const
    {
        const AbilityTypeInfo& info = GetAbilities(AbilityType::kAttack);
        return ChooseAbility(*GetOwnerWorld(), GetOwnerEntityID(), info.data, info.state);
    }

    // Chooses a omega ability and returns the state data for it
    // The chosen ability depends on the selection type for the omega abilities
    AbilityStatePtr ChooseOmegaAbility() const
    {
        const AbilityTypeInfo& info = GetAbilities(AbilityType::kOmega);
        return ChooseAbility(*GetOwnerWorld(), GetOwnerEntityID(), info.data, info.state);
    }

    // The chosen ability depends on the selection type for the innate abilities
    // Sets the cache for our class fast lookup
    void OnAbilityActivated(const AbilityStatePtr& ability, const int time_step)
    {
        auto& abilities = GetAbilities(ability->ability_type);
        abilities.state.current_ability_index = ability->index;

        ability->Activate(time_step);

        SetActiveAbility(ability);

        abilities.state.total_activations_count++;
    }
    void OnAbilityDeactivated()
    {
        SetActiveAbility(nullptr);
    }

    // Gets the current state of the attack/omega/Innate abilities
    std::vector<AbilityStatePtr>& GetStateAttackAbilities()
    {
        return GetAbilities(AbilityType::kAttack).state.abilities;
    }
    std::vector<AbilityStatePtr>& GetStateOmegaAbilities()
    {
        return GetAbilities(AbilityType::kOmega).state.abilities;
    }
    std::vector<AbilityStatePtr>& GetStateInnateAbilities()
    {
        return GetAbilities(AbilityType::kInnate).state.abilities;
    }

    // Get the total activated counts for attack/omega/innate
    int GetTotalActivatedAbilitiesCount(const AbilityType ability_type) const
    {
        return GetAbilities(ability_type).state.total_activations_count;
    }

    int GetTotalActivatedAbilitiesCount(const EnumSet<AbilityType>& types) const
    {
        int count = 0;
        for (const AbilityType type : types)
        {
            count += GetTotalActivatedAbilitiesCount(type);
        }

        return count;
    }

    // Does this component have any ability of specified type?
    bool HasAnyAbility(const AbilityType type) const
    {
        return !GetAbilities(type).state.abilities.empty();
    }

    // Helper function to get current active AbilitiesState
    AbilitiesState& GetMutableActiveAbilitiesState()
    {
        return GetAbilities(GetActiveAbilityType()).state;
    }
    const AbilitiesState& GetActiveAbilitiesState() const
    {
        return GetAbilities(GetActiveAbilityType()).state;
    }

    bool DoesOmegaRequireFocusToStart() const;

    //
    // Innates
    //

    // Get/Set data for the innate abilities
    const AbilitiesData& GetDataInnateAbilities() const
    {
        return GetAbilities(AbilityType::kInnate).data;
    }

    // Add the new innates over the existing ones
    void AddDataInnateAbilities(const std::vector<std::shared_ptr<const AbilityData>>& new_abilities)
    {
        for (const auto& ability : new_abilities)
        {
            AddDataInnateAbility(ability);
        }
    }

    void AddDataInnateAbility(const std::shared_ptr<const AbilityData>& ability_data);

    // Removes the innate that was attached from this entity_id.
    // Returns the number of removed abilities.
    void RemoveInnateAbilityByAttachedEntityID(const EntityID attached_entity_id);

    // Removes the innate if predicate returns true.
    // Returns the number of removed abilities.
    void RemoveInnateAbilityIf(const std::function<bool(const AbilityData&)>& predicate);

    // Chooses am innate ability and returns the state data for it
    // The chosen ability depends on the trigger type provided
    std::vector<AbilityStatePtr>& ChooseInnateAbility(const ActivationTriggerType trigger_type)
    {
        if (triggerable_abilities_.Contains(trigger_type))
        {
            return triggerable_abilities_.Get(trigger_type);
        }

        // Return empty vector by reference if that trigger type was not found
        static std::vector<AbilityStatePtr> empty_vector;
        return empty_vector;
    }

    // Add the innate to the waiting activation queue
    bool AddInnateWaitingActivation(const AbilityStatePtr& ability);
    bool HasAnyInnateWaitingActivation() const
    {
        return !innate_abilities_waiting_activation_.empty() || HasAnyInstantInnateWaitingActivation();
    }

    // Has instant innates waiting activation in the same time step
    bool HasAnyInstantInnateWaitingActivation() const
    {
        return !instant_innate_abilities_waiting_activation_.empty();
    }

    // Gets the pops the innate waiting activation that has a timer
    AbilityStatePtr PopInnateWaitingActivation();

    // Gets and pops the instant innate waiting activation that has a timer
    AbilityStatePtr PopInstantInnateWaitingActivation();

    // Gets instant innate waiting activation
    AbilityStatePtr GetInstantInnateWaitingActivation() const
    {
        if (!HasAnyInstantInnateWaitingActivation())
        {
            return nullptr;
        }

        return instant_innate_abilities_waiting_activation_.front();
    }

    size_t GetSizeInnatesWaitingActivation() const
    {
        return innate_abilities_waiting_activation_.size() + instant_innate_abilities_waiting_activation_.size();
    }

    size_t GetSizeInstantInnatesWaitingActivation() const
    {
        return instant_innate_abilities_waiting_activation_.size();
    }

    void IncrementDeployedSkillsCount(const AbilityType ability_type)
    {
        AddToAbilityTypeStat(ability_type, &AbilityTypeStats::skills_deployed, 1);
    }

    void IncrementReceivedEffectPackagesCount(const AbilityType ability_type)
    {
        AddToAbilityTypeStat(ability_type, &AbilityTypeStats::effect_packages_received, 1);
    }

    // Returns effect packages count received from specified abilities types
    int GetReceivedEffectPackagesCount(const EnumSet<AbilityType>& abilities_types) const
    {
        return GetAbilityTypeStat(abilities_types, &AbilityTypeStats::effect_packages_received);
    }

    // Returns deployed skills count received from specified abilities types
    int GetDeployedSkillsCount(const EnumSet<AbilityType>& abilities_types) const
    {
        return GetAbilityTypeStat(abilities_types, &AbilityTypeStats::skills_deployed);
    }

    AbilityTypeInfo& GetAbilities(const AbilityType ability_type)
    {
        return abilities_[EnumToIndex(ability_type)];
    }
    const AbilityTypeInfo& GetAbilities(const AbilityType ability_type) const
    {
        return abilities_[EnumToIndex(ability_type)];
    }

    // Helper function to choose an ability
    static AbilityStatePtr
    ChooseAbility(const World& world, const EntityID entity_id, const AbilitiesData& data, const AbilitiesState& state);

private:
    // Helper function to initialize the state values with the current values from data
    static void
    InitializeAbilitiesStateFromData(const AbilitiesData& data, const AbilityType ability_type, AbilitiesState* state);

    void InitializeInnateAbilitiesStateForNewAbilities()
    {
        auto& info = GetAbilities(AbilityType::kInnate);
        auto& abilities = info.data.abilities;
        auto& abilities_states = info.state.abilities;
        assert(abilities_states.size() <= abilities.size());
        while (abilities_states.size() < abilities.size())
        {
            const size_t index = abilities_states.size();
            abilities_states.push_back(CreateAbilityStateFromData(abilities[index], AbilityType::kInnate, index));
        }
    }

    // Helper function to initialize the state values with the current values from data
    static AbilityStatePtr CreateAbilityStateFromData(
        const std::shared_ptr<const AbilityData>& ability_data_ptr,
        const AbilityType ability_type,
        size_t ability_index);

    void RebuildTriggerableAbilitiesMap()
    {
        auto& innates_info = GetAbilities(AbilityType::kInnate);
        // Build fast map
        triggerable_abilities_.Clear();
        for (const auto& ability_state : innates_info.state.abilities)
        {
            triggerable_abilities_.GetOrAdd(ability_state->data->activation_trigger_data.trigger_type)
                .push_back(ability_state);
        }
    }

    // Returns specified stat for specified ability type
    int GetAbilityTypeStat(const EnumSet<AbilityType>& abilities_types, int AbilityTypeStats::*member_ptr) const
    {
        int total = 0;
        for (const AbilityType ability_type : abilities_types)
        {
            if (abilities_types_stats_.Contains(ability_type))
            {
                total += abilities_types_stats_.Get(ability_type).*member_ptr;
            }
        }
        return total;
    }

    void AddToAbilityTypeStat(const AbilityType ability_type, int AbilityTypeStats::*member_ptr, int value)
    {
        switch (ability_type)
        {
        case AbilityType::kAttack:
        case AbilityType::kOmega:
        case AbilityType::kInnate:
            abilities_types_stats_.GetOrAdd(ability_type).*member_ptr += value;
            break;

        default:
            assert(false);
            break;
        }
    }

private:
    static constexpr size_t kAbilitiesTypesCount = GetEnumEntriesCount<AbilityType>();
    std::array<AbilityTypeInfo, kAbilitiesTypesCount> abilities_;

    // Current active ability index and type
    AbilityStatePtr active_ability_;

    //
    // Innate abilities
    //

    // Queue for innates that are not instant waiting
    std::deque<AbilityStatePtr> innate_abilities_waiting_activation_;

    // Queue of instant innates waiting activation
    std::deque<AbilityStatePtr> instant_innate_abilities_waiting_activation_;

    // Fast lookup map to check if an innate is waiting activation
    std::unordered_set<AbilityStatePtr> innate_abilities_waiting_activation_set_;

    // Helper map for innates to figure out what abilities are for each activation trigger type
    // Key: The activation trigger type
    // Value: vector of abilities
    EnumMap<ActivationTriggerType, std::vector<AbilityStatePtr>> triggerable_abilities_;

    // Key: The ability type
    // Value: statistics for that ability type
    EnumMap<AbilityType, AbilityTypeStats> abilities_types_stats_;
};

// This is simplified formatter for ability state which
// prints just ability and trigger type
struct AbilityTypeContextFormat
{
    const AbilityState& state;  // NOLINT
    void FormatTo(fmt::format_context& ctx) const;
};

}  // namespace simulation

// Formatters

ILLUVIUM_MAKE_STRUCT_FORMATTER(AbilityStateActivatorContext, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(AbilityState, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(AbilityTypeContextFormat, FormatTo);
