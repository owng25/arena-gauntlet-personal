#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "data/effect_data_attributes.h"
#include "data/effect_enums.h"
#include "data/effect_expression.h"
#include "data/effect_package_attributes.h"
#include "data/effect_type_id.h"
#include "data/enums.h"
#include "data/source_context.h"
#include "data/stats_data.h"
#include "ecs/event.h"
#include "utility/custom_formatter.h"
#include "validation_data.h"

namespace simulation
{
class AbilityData;
struct SkillData;

// Holds all the info data for the lifetime of an effect
struct EffectLifetime
{
    // Copyable or Movable
    EffectLifetime() = default;
    EffectLifetime(const EffectLifetime&) = default;
    EffectLifetime& operator=(const EffectLifetime&) = default;
    EffectLifetime(EffectLifetime&&) = default;
    EffectLifetime& operator=(EffectLifetime&&) = default;
    ~EffectLifetime() = default;

    // Implicit from an int
    // is_consumable = false
    //
    explicit EffectLifetime(const int new_duration_ms) : is_consumable(false), duration_time_ms(new_duration_ms) {}

    void FormatTo(fmt::format_context& ctx) const;

    // Flag to deactivate effect if validation list is not valid
    // Example: The positive state should activate when health is below 2000. If health is above 2000 and this attribute
    // is set to true, the positive state is deactivates.
    bool deactivate_if_validation_list_not_valid = false;

    // Is it based on number of Activations or Time
    bool is_consumable = false;

    // This is a value that represents the highest the Stack Counter can go.
    int max_stacks = kTimeInfinite;

    // Is this attached effect cleansed when it reaches the max stacks?
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/280297481/Condition.CleanseAtMaxStacks
    bool cleanse_at_max_stacks = false;

    // Override for the overlap process type for the same ability.
    // See: https://illuvium.atlassian.net/wiki/spaces/AB/pages/72417779/Effect+Overlap+Processing
    // NOTE: for different ability the default will be used.
    EffectOverlapProcessType overlap_process_type = EffectOverlapProcessType::kNone;

    // Used with EffectPackage blocks.
    // Tells how many times an effect can be used to block another effect
    int blocks_until_expiry = kTimeInfinite;

    //
    // Consumable
    //

    // Only used if is_consumable = true
    // Represents the total_activations amount we have for this effect
    // If this is 10 and we do 10 attack abilities then after the effect is dead
    // Only used if the effect type is: EffectType::kEmpower
    int activations_until_expiry = kTimeInfinite;

    // When is this empowered activated
    // NOTE: Only used if type is EffectType::kEmpower
    AbilityType activated_by = AbilityType::kNone;

    // How often should the empower activate
    int consumable_activation_frequency = 1;

    //
    // Not consumable
    //

    // Duration of the effect in milliseconds (kTimeInfinite == infinite)
    // NOTE: This is used depending on the effect type and context
    // Only used if is_consumable = false
    int duration_time_ms = kTimeInfinite;

    // At what frequency do the attached effects tick
    int frequency_time_ms = kDefaultAttachedEffectsFrequencyMs;

    // Determines if the effect should only execute if the attack is a critical
    bool activate_on_critical = false;
};

// A single entry in the effects package data list
class EffectData
{
public:
    constexpr bool HasStaticFrequency() const
    {
        return lifetime.frequency_time_ms == 0;
    }

    // Create an effect with just an expression
    static EffectData Create(const EffectType type, const EffectExpression& expression)
    {
        EffectData data;
        data.type_id.type = type;
        data.SetExpression(expression);
        return data;
    }

    static EffectData CreateShieldBurn(const EffectExpression& expression)
    {
        EffectData data;
        data.type_id.type = EffectType::kInstantShieldBurn;
        data.SetExpression(expression);
        return data;
    }

    // Create a damage effect with just an expression
    static EffectData CreateDamage(const EffectDamageType damage_type, const EffectExpression& expression)
    {
        EffectData data;
        data.type_id.type = EffectType::kInstantDamage;
        data.type_id.damage_type = damage_type;
        data.SetExpression(expression);
        return data;
    }

    // Create an effect of type EffectType::kSpawnShield
    static EffectData CreateSpawnShield(const EffectExpression& expression, const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kSpawnShield;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kSpawnShield with skills
    static EffectData CreateSpawnShield(
        const EffectExpression& expression,
        const int duration_time_ms,
        const std::unordered_map<EventType, std::shared_ptr<SkillData>>& skills)
    {
        EffectData data;
        data.type_id.type = EffectType::kSpawnShield;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.event_skills = skills;
        return data;
    }

    // Create an effect of type EffectType::kSpawnMark with ability
    static EffectData CreateSpawnMark(const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kSpawnMark;
        data.lifetime.duration_time_ms = duration_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kPositiveState
    static EffectData CreatePositiveState(const EffectPositiveState positive_state, const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kPositiveState;
        data.type_id.positive_state = positive_state;
        data.lifetime.duration_time_ms = duration_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kNegativeState
    static EffectData CreateNegativeState(const EffectNegativeState negative_state, const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kNegativeState;
        data.type_id.negative_state = negative_state;
        data.lifetime.duration_time_ms = duration_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kPlaneChange
    static EffectData CreatePlaneChange(const EffectPlaneChange plane_change, const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kPlaneChange;
        data.type_id.plane_change = plane_change;
        data.lifetime.duration_time_ms = duration_time_ms;
        return data;
    }

    static EffectData CreateStatOverride(const StatType stat_type, FixedPoint new_stat_value)
    {
        EffectData data;
        data.type_id.type = EffectType::kStatOverride;
        data.type_id.stat_type = stat_type;
        data.expression_.base_value.value = new_stat_value;
        return data;
    }

    // Create an effect of type EffectType::kBuff
    static EffectData CreateBuff(const StatType stat, const EffectExpression& expression, const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kBuff;
        data.type_id.stat_type = stat;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.lifetime.frequency_time_ms = 0;
        return data;
    }

    static EffectData CreateDynamicBuff(
        const StatType stat,
        const EffectExpression& expression,
        const int duration_time_ms,
        const int refresh_frequency)
    {
        assert(refresh_frequency > 0);
        auto data = CreateBuff(stat, expression, duration_time_ms);
        data.lifetime.frequency_time_ms = refresh_frequency;
        return data;
    }

    // Create an effect of type EffectType::kDebuff
    static EffectData CreateDebuff(const StatType stat, const EffectExpression& expression, const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kDebuff;
        data.type_id.stat_type = stat;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.lifetime.frequency_time_ms = 0;
        return data;
    }

    static EffectData CreateDynamicDebuff(
        const StatType stat,
        const EffectExpression& expression,
        const int duration_time_ms,
        const int refresh_frequency)
    {
        assert(refresh_frequency > 0);
        auto data = CreateDebuff(stat, expression, duration_time_ms);
        data.lifetime.frequency_time_ms = refresh_frequency;
        return data;
    }

    // Create an effect of type EffectType::kDamageOverTime
    static EffectData CreateDamageOverTime(
        const EffectDamageType damage_type,
        const EffectExpression& expression,
        const int duration_time_ms,
        const int frequency_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kDamageOverTime;
        data.type_id.damage_type = damage_type;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.lifetime.frequency_time_ms = frequency_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kEnergyBurnOverTime
    static EffectData CreateEnergyBurnOverTime(
        const EffectExpression& expression,
        const int duration_time_ms,
        const int frequency_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kEnergyBurnOverTime;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.lifetime.frequency_time_ms = frequency_time_ms;
        return data;
    }

    // Create a Damage over time Condition type
    // NOTE: This has predefined values and duration
    static EffectData CreateCondition(const EffectConditionType condition_type, const int frequency_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kCondition;
        data.type_id.condition_type = condition_type;
        data.lifetime.frequency_time_ms = frequency_time_ms;
        return data;
    }

    // Create a heal effect with just an expression
    static EffectData CreateHeal(const EffectHealType heal_type, const EffectExpression& expression)
    {
        EffectData data;
        data.type_id.type = EffectType::kInstantHeal;
        data.type_id.heal_type = heal_type;
        data.SetExpression(expression);
        return data;
    }

    // Create an effect of type EffectType::kHealOverTime
    static EffectData CreateHealOverTime(
        const EffectHealType heal_type,
        const EffectExpression& expression,
        const int duration_time_ms,
        const int frequency_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kHealOverTime;
        data.type_id.heal_type = heal_type;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.lifetime.frequency_time_ms = frequency_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kEnergyGainOverTime
    static EffectData CreateEnergyGainOverTime(
        const EffectExpression& expression,
        const int duration_time_ms,
        const int frequency_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kEnergyGainOverTime;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.lifetime.frequency_time_ms = frequency_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kHyperGainOverTime
    static EffectData
    CreateHyperGainOverTime(const EffectExpression& expression, const int duration_time_ms, const int frequency_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kHyperGainOverTime;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.lifetime.frequency_time_ms = frequency_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kHyperBurnOverTime
    static EffectData
    CreateHyperBurnOverTime(const EffectExpression& expression, const int duration_time_ms, const int frequency_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kHyperBurnOverTime;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.lifetime.frequency_time_ms = frequency_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kDisplacement
    static EffectData CreateDisplacement(const EffectDisplacementType displacement_type, const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kDisplacement;
        data.type_id.displacement_type = displacement_type;
        data.lifetime.duration_time_ms = duration_time_ms;
        return data;
    }

    // Create an effect of type EffectType::kExecute
    static EffectData CreateExecute(
        const EffectExpression& expression,
        const int duration_time_ms,
        const std::vector<AbilityType>& ability_types)
    {
        EffectData data;
        data.type_id.type = EffectType::kExecute;
        data.SetExpression(expression);
        data.lifetime.duration_time_ms = duration_time_ms;
        data.ability_types = ability_types;
        return data;
    }

    // Create an effect of type EffectType::kBlink
    static EffectData
    CreateBlink(const ReservedPositionType blink_target, const int blink_delay_ms, const int duration_time_ms)
    {
        EffectData data;
        data.type_id.type = EffectType::kBlink;
        data.blink_target = blink_target;
        data.blink_delay_ms = blink_delay_ms;
        data.lifetime.duration_time_ms = duration_time_ms;
        return data;
    }

    bool IsWound() const
    {
        return type_id.type == EffectType::kCondition && type_id.condition_type == EffectConditionType::kWound;
    }

    // The effect type uses the expression value.
    static bool UsesExpression(const EffectType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
        switch (type)
        {
        case EffectType::kSpawnShield:
        case EffectType::kInstantDamage:
        case EffectType::kInstantHeal:
        case EffectType::kDamageOverTime:
        case EffectType::kHealOverTime:
        case EffectType::kEnergyGainOverTime:
        case EffectType::kHyperGainOverTime:
        case EffectType::kHyperBurnOverTime:
        case EffectType::kDebuff:
        case EffectType::kBuff:
        case EffectType::kInstantEnergyBurn:
        case EffectType::kInstantEnergyGain:
        case EffectType::kInstantHyperBurn:
        case EffectType::kInstantHyperGain:
        case EffectType::kEnergyBurnOverTime:
        case EffectType::kExecute:
        case EffectType::kInstantShieldBurn:
            return true;
        default:
            return false;
        }
    }
    bool UsesExpression() const
    {
        return UsesExpression(type_id.type);
    }

    // The effect type uses the duration_time_ms value.
    static bool UsesDurationTime(const EffectType type)
    {
        switch (type)
        {
        case EffectType::kPositiveState:
        case EffectType::kNegativeState:
        case EffectType::kHealOverTime:
        case EffectType::kEnergyGainOverTime:
        case EffectType::kHyperGainOverTime:
        case EffectType::kDamageOverTime:
        case EffectType::kBuff:
        case EffectType::kDebuff:
        case EffectType::kEmpower:
        case EffectType::kDisempower:
        case EffectType::kSpawnShield:
        case EffectType::kSpawnMark:
        case EffectType::kDisplacement:
        case EffectType::kEnergyBurnOverTime:
        case EffectType::kHyperBurnOverTime:
        case EffectType::kExecute:
        case EffectType::kBlink:
        case EffectType::kPlaneChange:
            return true;
        default:
            return false;
        }
    }
    bool UsesDurationTime() const
    {
        return UsesDurationTime(type_id.type);
    }

    // Check if this attached effects component has an active effect of this type
    bool HasCondition(const ConditionType condition) const
    {
        return required_conditions.Contains(condition);
    }

    // Checks if the effect type is a instant metered combat stat
    // See https://illuvium.atlassian.net/wiki/spaces/AB/pages/252379619/Instant+Metered+Combat+Stat+Change+Effect
    static bool IsInstantMeteredType(const EffectType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
        switch (type)
        {
        case EffectType::kInstantDamage:
        case EffectType::kInstantHeal:
        case EffectType::kInstantEnergyBurn:
        case EffectType::kInstantEnergyGain:
        case EffectType::kInstantHyperGain:
        case EffectType::kInstantHyperBurn:
            return true;
        default:
            return false;
        }
    }
    bool IsInstantMeteredType() const
    {
        return IsInstantMeteredType(type_id.type);
    }

    // Check if the effect is a detrimental effect type
    static bool IsTypeDetrimental(const EffectType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
        switch (type)
        {
        case EffectType::kInstantDamage:
        case EffectType::kNegativeState:
        case EffectType::kDamageOverTime:
        case EffectType::kCondition:
        case EffectType::kDebuff:
        case EffectType::kDisempower:
        case EffectType::kDisplacement:
        case EffectType::kInstantEnergyBurn:
        case EffectType::kInstantHyperBurn:
        case EffectType::kEnergyBurnOverTime:
        case EffectType::kInstantShieldBurn:
            return true;
        default:
            return false;
        }
    }

    bool IsTypeDetrimental() const
    {
        if (IsTypeDetrimental(type_id.type))
        {
            return true;
        }

        if (type_id.type == EffectType::kSpawnMark)
        {
            return type_id.mark_type == MarkEffectType::kDetrimental;
        }

        return false;
    }

    // Check if the effect is a beneficial effect type
    static bool IsTypeBeneficial(const EffectType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
        switch (type)
        {
        case EffectType::kInstantHeal:
        case EffectType::kBuff:
        case EffectType::kSpawnShield:
        case EffectType::kEmpower:
        case EffectType::kPositiveState:
        case EffectType::kInstantEnergyGain:
        case EffectType::kInstantHyperGain:
        case EffectType::kExecute:
            return true;
        default:
            return false;
        }
    }

    bool IsTypeBeneficial() const
    {
        if (IsTypeBeneficial(type_id.type))
        {
            return true;
        }

        if (type_id.type == EffectType::kSpawnMark)
        {
            return type_id.mark_type == MarkEffectType::kBeneficial;
        }

        return false;
    }

    // Copyable or Movable
    EffectData() = default;
    EffectData(const EffectData&) = default;
    EffectData& operator=(const EffectData&) = default;
    EffectData(EffectData&&) = default;
    EffectData& operator=(EffectData&&) = default;
    ~EffectData() = default;

    // Helper function to create a deep copy of this EffectData
    EffectData CreateDeepCopy() const;

    void FormatTo(fmt::format_context& ctx) const;

    // Unique identifier
    std::string id;

    // Unique type ID of the effect data
    EffectTypeID type_id{};

    // Can the attached effect be removed
    bool can_cleanse = true;

    // Skills triggered on events
    std::unordered_map<EventType, std::shared_ptr<SkillData>> event_skills;

    // Used to check if this effect should apply, if all the conditions from here are true then this
    // effect applies
    EnumSet<ConditionType> required_conditions;

    //
    // Time/Duration
    //

    EffectLifetime lifetime;

    //
    // EffectType::kEmpower || EffectType::kDisempower || EffectType::kSpawnShield || EffectSystem::kSpawnMark
    //

    // A list of Effects that must be included for certain effect types.
    std::vector<EffectData> attached_effects;

    // Attached Effect package attributes
    EffectPackageAttributes attached_effect_package_attributes;

    // Is this effect data used as an global effect attribute for empowers
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect-Attribute
    bool is_used_as_global_effect_attribute = false;

    // For what effect type id should this empower be applied to
    // NOTE: only used if is_used_as_global_effect_attribute is true
    EffectTypeID empower_for_effect_type_id;

    // Attached abilities
    // NOTE: Only used for mark
    std::vector<std::shared_ptr<const AbilityData>> attached_abilities;

    // Should the attached entity (shield/mark) be destroyed if the combat unit who sent this dies?
    bool should_destroy_attached_entity_on_sender_death = false;

    //
    // EffectType::kDisplacement
    //

    // Distance for displacement (knock back)
    int displacement_distance_sub_units = 0;

    //
    // EffectType::kBlink
    //

    // Target for blink effects
    ReservedPositionType blink_target = ReservedPositionType::kNone;

    //
    // type_id.type is EffectType::kImmune and
    // type_id.positive_state is EffectPositiveState::kImmune
    //

    // List if effects types immuned by this effect
    // Empty list means absolute immunity
    std::vector<EffectTypeID> immuned_effect_types;

    // Delay before applying blink payload
    int blink_delay_ms = 0;

    // Should blink reserve previous position after blink
    bool blink_reserve_previous_position = false;

    //
    // Other attributes
    //

    // Optional attributes
    EffectDataAttributes attributes;

    // Used depending on context, used by types:
    // EffectType::kExecute, EffectType::kNegativeState, EffectType::kPositiveState
    std::vector<AbilityType> ability_types;

    // Effect validation data
    EffectValidations validations{};

    // Radius for effects that have radius
    // NOTE: Used for buffs and debuffs inside auras
    int radius_units = 0;

    const EffectExpression& GetExpression() const
    {
        return expression_;
    }

    const EffectExpression& GetStacksIncrement() const
    {
        return stacks_increment_;
    }

    void SetStacksIncrement(const EffectExpression& stacks)
    {
        stacks_increment_ = stacks;
    }

    void SetExpression(const EffectExpression& expression)
    {
        expression_ = expression;
        UpdateDataSources();
    }

    void SetExpression(EffectExpression&& expression)
    {
        expression_ = std::move(expression);
        UpdateDataSources();
    }

    const EnumSet<ExpressionDataSourceType>& GetRequiredDataSourceTypes() const
    {
        return data_source_types_;
    }

protected:
    void UpdateDataSources()
    {
        // This only caches data sources for an expression at the moment.
        // Lets keep this for time when we also need to compute data sources from
        // attached effects / attached package attributes expressions.
        data_source_types_ = expression_.GatherRequiredDataSourceTypes();
    }

private:
    // The value of this effect represented as an expression.
    // Call expression.Evaluate() to get the final value.
    EffectExpression expression_{};

    // How many stacks should this add to the condition
    EffectExpression stacks_increment_{};

    // Cached data sources required by this effect
    EnumSet<ExpressionDataSourceType> data_source_types_{};
};

// The current state in the game for an effect data
struct EffectState
{
    // If this a critical hit.
    bool is_critical = false;

    // The source of the effect
    SourceContextData source_context;

    // The full stats data of the entity that sent this effect.
    FullStatsData sender_stats{};

    // The full captured (at the moment of effect application) stats of combat unit focused by the sender
    FullStatsData sender_focus_stats{};

    // The effect package attributes at the moment of sending
    EffectPackageAttributes effect_package_attributes{};

    // The ID of the entity that attached this ability data
    // NOTE: Only used by marks atm
    EntityID attached_from_entity_id = kInvalidEntityID;

    void FormatTo(fmt::format_context& ctx) const;
};

// Contains a vector of effects replacements
class EffectDataReplacements
{
public:
    static std::shared_ptr<EffectDataReplacements> Create()
    {
        return std::make_shared<EffectDataReplacements>();
    }

    bool ContainsID(const std::string& id) const;
    const EffectData* GetEffectDataForID(const std::string& id) const;

    std::vector<EffectData> replacements;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectLifetime, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectState, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectData, FormatTo);
