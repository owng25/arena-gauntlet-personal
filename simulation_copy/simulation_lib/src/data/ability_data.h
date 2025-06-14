#pragma once

#include <string>

#include "data/skill_data.h"
#include "data/source_context.h"
#include "utility/enum_set.h"

namespace simulation
{
enum class AbilitySelectionType
{
    kNone = 0,

    // This is the default method of changing between abilities. When a combat unit has a single
    // Ability this would cycle from Ability [0] → Ability [0] since the Ability list is of length
    // one. If there were more than one Ability it would increment from [0] → [1] → [2] etc. The
    // method for doing this would be to select Ability i++ % Abilities.length
    kCycle,

    // Use this when you want to select an Ability based on an attribute's value. Need to pass in
    // the attribute to check as well as the value to check it against. Ability 0 is always used
    // when above that value, and Ability 1 is used when below.
    kSelfAttributeCheck,

    // This Selection Type can only be used if the Ability list is of length 2. It is used for
    // things like "When above X% of Max Health, use Ability [0], otherwise use Ability [1]."
    kSelfHealthCheck,

    // Ability[0] will activate x times, after that ability[1] will activate then cycle over
    kEveryXActivations,

    // -new values can be added above this line
    kNum
};

enum class AbilityTriggerContextRequirement
{
    kNone = 0,

    // Require omega being active
    kDuringOmega,

    // -new values can be added above this line
    kNum
};

// Filter for the activation trigger type
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/257525830/Trigger+Attribute
struct AbilityActivationTriggerData
{
    AbilityActivationTriggerData() = default;
    AbilityActivationTriggerData(const AbilityActivationTriggerData&) = default;
    AbilityActivationTriggerData(AbilityActivationTriggerData&&) = default;
    AbilityActivationTriggerData& operator=(const AbilityActivationTriggerData& another) = default;
    AbilityActivationTriggerData& operator=(AbilityActivationTriggerData&& another) = default;
    ~AbilityActivationTriggerData() = default;

    bool IsEmpty() const;
    void FormatTo(fmt::format_context& ctx) const;

    ActivationTriggerType trigger_type = ActivationTriggerType::kNone;

    // Determines for what abilities types this trigger for.
    // Empty set means any ability.
    EnumSet<AbilityType> ability_types = EnumSet<AbilityType>::MakeFull();

    // Specifies the allegiance criteria for entity that sends a combat event
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/258441226/Trigger.Allegiance
    AllegianceType sender_allegiance = AllegianceType::kAll;

    // Specifies the allegiance criteria for entity that receives a combat event
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/258441226/Trigger.Allegiance
    AllegianceType receiver_allegiance = AllegianceType::kAll;

    // Specifies a maximum range for an event before it will be ignored.
    // Note: Measured in Hex
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/257657003/Trigger.Range
    int range_units = 0;

    // Specifies a minimum time for a Combat Event to be considered
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/258441217/Trigger.NotBefore
    int not_before_ms = 0;

    // Specifies a maximum time for a Combat Event to be considered.
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/258408462/Trigger.NotAfter
    int not_after_ms = 0;

    // Number of activations that this ability can perform.
    // NOTE: Only used for innate abilities
    int max_activations = 0;

    // Number of TimeSteps that this ability is able to be activated
    // NOTE: Only used for innate abilities.
    int activation_time_limit_ms = 0;

    // At what time interval should this ability activate
    // Example: Every 15 seconds
    // NOTE: Only used for innate abilities.
    int activate_every_time_ms = 0;

    // Lower Percentage of the health limit
    // Example: If health is below 50% of maximum health, HealthInRange emits.
    // NOTE: Only used for innate abilities.
    FixedPoint health_lower_limit_percentage = 0_fp;

    // The number of abilities after which the ability activates
    // Example: Berserker deals additional damage after 5 attacks
    // NOTE: Only used for innate abilities.
    int number_of_abilities_activated = 1;

    // Range between two Illuvials
    // Example: If Poisoned enemy is within 20 hex range of Poison Illuvial, kInRange emits.
    // NOTE: Only used for innate abilities.
    int activation_radius_units = 0;

    // How many ms are required to pass between activations
    int activation_cooldown_ms = 0;

    // Required conditions for sender
    // NOTE: Only used for innate abilities.
    EnumSet<ConditionType> required_sender_conditions{};

    // Required conditions for receiver
    // NOTE: Only used for innate abilities.
    EnumSet<ConditionType> required_receiver_conditions{};

    // Required damage type. Empty set means "any type of damage"
    // NOTE: Only used for OnDealDamage and OnTakeDamage events
    EnumSet<EffectDamageType> damage_types{};

    // Required amount of damage for trigger to work
    FixedPoint damage_threshold = 0_fp;

    // The number of effect packages received after which the ability activates
    // The type of ability to count determined by 'ability_type'
    // NOTE: Only used for kOnReceiveXEffectPackages trigger of innate ability.
    int number_of_effect_packages_received = 1;

    // The way to compare the some number to another
    // What numbers are going to be compared depends on trigger type.
    // For kOnReceiveXEffectPackages we compare runtime number of received package to number_of_effect_packages_received
    // For kOnVanquish we compare AbilityState::trigger_counter to trigger_value
    ComparisonType comparison_type = ComparisonType::kEqual;

    // If this value is not zero, the modulo operator will be applied to the number
    // of received effect packages before comparing it to number_of_effect_packages_received.
    // This allows making cycling ability trigger.
    // NOTE: Only used for kOnReceiveXEffectPackages trigger of innate ability.
    int number_of_effect_packages_received_modulo = 0;

    // Value to use to compare to the Ability.TriggerNumber variable.
    // NOTE: Only used for kOnVanquish trigger of innate ability.
    int trigger_value = 1;

    // The number of skills deployed after which the ability activates
    // The type of ability to count determined by 'ability_type'
    // NOTE: Only used for kOnDeployXSkills trigger of innate ability.
    int number_of_skills_deployed = 1;

    // Activate trigger only if context is matches requirement
    AbilityTriggerContextRequirement context_requirement = AbilityTriggerContextRequirement::kNone;

    // Put ability into queue when triggered
    bool force_add_to_queue_on_activation = false;

    // If we should trigger every time we reach a multiple of the
    // number_of_abilities_activated or just once.
    bool every_x = false;

    // Specifies to only listen to a Combat Event if it is emitted by the Focus.
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/258408471/Trigger.OnlyFocus
    bool only_focus = false;

    // Should this ability trigger only once per spawned entity (zone/chain/etc)
    bool once_per_spawned_entity = false;

    // Should this ability be triggered only by events from parent combat unit
    bool only_from_parent = false;

    // Indicates whether this ability must be activated immediately upon triggering, disallowing queuing.
    // You can find examples here: https://illuvium.atlassian.net/browse/ARENA-7040
    bool immediate_activation_only = false;

    // If true requires hyper to be active on this unit in order to trigger ability
    bool requires_hyper_active = false;
};

// Defines the data for an ability
// An ability is just a list of skills
class AbilityData
{
public:
    // Create a new instance
    static std::shared_ptr<AbilityData> Create()
    {
        return std::make_shared<AbilityData>();
    }

    // Helper function to create a deep copy of this AbilityData
    std::shared_ptr<AbilityData> CreateDeepCopy() const
    {
        std::shared_ptr<AbilityData> copy = std::make_shared<AbilityData>(*this);

        // Copy skills
        copy->skills.clear();
        for (const auto& skill_data : skills)
        {
            copy->skills.push_back(skill_data->CreateDeepCopy());
        }

        return copy;
    }

    // Adds a new skill
    void AddSkill(std::shared_ptr<SkillData> skill)
    {
        skills.emplace_back(std::move(skill));
    }
    SkillData& AddSkill()
    {
        auto skill = SkillData::Create();
        auto* skill_ptr = skill.get();
        AddSkill(std::move(skill));
        return *skill_ptr;
    }
    const SkillData* GetSkillData(const size_t skill_index) const
    {
        if (skill_index >= skills.size())
        {
            return nullptr;
        }

        return skills.at(skill_index).get();
    }

    void FormatTo(fmt::format_context& ctx) const;

    // Unique identifier for the ability name, used by the effects overlap processing
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/72417779/Effect+Overlap+Processing
    std::string name;

    // What should this ability do based on the name
    AbilityUpdateType update_type = AbilityUpdateType::kNone;

    // If true this ability will only activate once
    bool is_use_once = false;

    // The total duration of this ability in milliseconds
    int total_duration_ms = 0;

    // If true the ability will ignore the attack abilities speed timers
    bool ignore_attack_speed = false;

    // A list of the skills that make up this ability
    std::vector<std::shared_ptr<SkillData>> skills;

    // Activation event for innate ability
    // NOTE: Only used for innate abilities
    AbilityActivationTriggerData activation_trigger_data;

    // The ID of the entity that attached this ability data
    // NOTE: Only used by marks atm
    EntityID attached_from_entity_id = kInvalidEntityID;

    // The source of the ability
    SourceContextData source_context;

    // MovementLock is a ability attribute that determines if a combat unit is allowed to move during this ability.
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/262472011/SubAbility.MovementLock
    bool movement_lock = true;
};

// A vector of abilities that also specifies how we select this ability
struct AbilitiesData
{
    // Adds a new ability
    void AddAbility(std::shared_ptr<const AbilityData> ability)
    {
        abilities.emplace_back(std::move(ability));
    }
    AbilityData& AddAbility()
    {
        auto ability = AbilityData::Create();
        auto* ability_ptr = ability.get();
        AddAbility(std::move(ability));
        return *ability_ptr;
    }

    AbilitiesData CreateDeepCopy() const
    {
        // Copy everything
        AbilitiesData copy = *this;

        // Copy abilities
        copy.abilities.clear();
        for (const std::shared_ptr<const AbilityData>& ability : abilities)
        {
            copy.abilities.emplace_back(ability->CreateDeepCopy());
        }

        return copy;
    }

    // Merge this AbilitiesData with the Other AbilitiesData
    void MergeAbilities(const AbilitiesData& other);

    // How we choose from this list of abilities
    AbilitySelectionType selection_type = AbilitySelectionType::kNone;

    // The list of abilities
    std::vector<std::shared_ptr<const AbilityData>> abilities;

    // Determines how many times ability[0] should activate before ability[1] activates
    // Only to be used with Selection Type kEveryXActivations
    int activation_cadence = 0;

    // Determines the % of either a selected stat for Attribute activation type
    // or % (MaxHealth - CurrentHealth) that must be at or above to activate ability[0]
    // otherwise  ability[1]
    // Only to be used with Selection Type kSelfHealthCheck or kSelfAttributeCheck
    FixedPoint activation_check_value = 0_fp;

    // Needed for Attribute activation types, this is the attribute that gets checked
    StatType activation_check_stat_type = StatType::kNone;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(AbilityActivationTriggerData, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(AbilityData, FormatTo);
