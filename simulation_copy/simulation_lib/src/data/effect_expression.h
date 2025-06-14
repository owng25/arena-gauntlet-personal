#pragma once

#include <array>

#include "data/combat_synergy_bonus.h"
#include "data/stats_data.h"
#include "data/synergy_order.h"
#include "utility/ensure_enum_size.h"
#include "utility/enum_as_index.h"
#include "utility/enum_set.h"
#include "utility/fixed_point.h"

namespace simulation
{
class World;

// Tells us the type of the zone
enum class EffectValueType
{
    // NOTE: kNone value was omitted on purpose

    // Simple flat value amount.
    // Example: 250
    kValue = 0,

    // Simple stat type without a percentage
    // Example: CurrentShield (Receiver)
    kStat,

    // We compute the value as a percentage of a stat
    // Example: 20% Physical Resist (Receiver)
    kStatPercentage,

    // Same as kStatPercentage but only with high precision.
    // See FixedPoint::AsHighPrecisionPercentageOf
    kStatHighPrecisionPercentage,

    // Used to count how many combat affinities or combat classes there is
    // Example: SynergyCount Water
    kSynergyCount,

    // Compute the metered value as a percentage of a stat
    // Example: Current Health % (Receiver) = Current Health / Max Health (Receiver)
    kMeteredStatPercentage,

    // Used to count the current hyper bonus of the unit
    kHyperEffectiveness,

    // Custom function
    kCustomFunction,

    // -new values can be added above this line
    kNum
};

// Type of operation done on effects
enum class EffectOperationType
{
    // No Operation
    kNone = 0,

    // +
    kAdd,

    // -
    kSubtract,

    // *
    kMultiply,

    // /
    kDivide,

    // //
    kIntegralDivision,

    // %
    kPercentageOf,

    // Same as kPercentageOf but with only high precision
    // See FixedPoint::AsHighPrecisionPercentageOf
    kHighPrecisionPercentageOf,

    // Returns minimum value of two operands
    kMin,

    // Resturns maximum value of two operands
    kMax,

    // -new values can be added above this line
    kNum
};

enum class ExpressionDataSourceType
{
    kNone = 0,

    // Use sender combat unit
    // i.e. if sender is not a combat unit it will try it's parent
    kSender,

    // Use receiver
    kReceiver,

    // Use data from the sender combat unit currently focused unit.
    // Focused unit will be selected during the first effect application.
    kSenderFocus,

    // -new values can be added above this line
    kNum,
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(ExpressionDataSourceType);

// Stores all data required for one entity in EffectExpression
struct EntityDataForExpression
{
    FullStatsData stats;
    const std::vector<SynergyOrder>* synergies = nullptr;
};

// Stores EntityDataForExpression for each ExpressionDataSourceType
// Note: world.h has a bunch of utilities to make an instance of this objects
// in the simplest way possible
class ExpressionStatsSource
{
public:
    ExpressionStatsSource() = default;

    void Set(
        const ExpressionDataSourceType data_source_type,
        const FullStatsData& stats,
        const std::vector<SynergyOrder>* synergies)
    {
        Set(data_source_type, EntityDataForExpression{stats, synergies});
    }

    bool Contains(const ExpressionDataSourceType data_source_type) const
    {
        return sources_present_.Contains(data_source_type);
    }

    void Set(const ExpressionDataSourceType data_source_type, const EntityDataForExpression& data)
    {
        all_sources_[ToIndex(data_source_type)] = data;
        sources_present_.Add(data_source_type);
    }

    const EntityDataForExpression& Get(const ExpressionDataSourceType data_source_type) const
    {
        // assert(sources_present_.Contains(data_source_type));
        return all_sources_[ToIndex(data_source_type)];
    }

private:
    // converts ExpressionDataSourceType to index in internal array
    static constexpr size_t ToIndex(const ExpressionDataSourceType data_source_type)
    {
        assert(data_source_type != ExpressionDataSourceType::kNone);
        // -1 because ExpressionDataSourceType::kNone is not a valid index
        const size_t index = static_cast<size_t>(data_source_type) - 1;
        return index;
    }

private:
    // -1 here because ExpressionDataSourceType has None entry
    static constexpr size_t kMaxPossibleSources = static_cast<size_t>(ExpressionDataSourceType::kNum) - 1;

    // An array with all possbile data sources for effect
    // Indexed by ExpressionDataSourceType, which should be converted using ToIndex method
    std::array<EntityDataForExpression, kMaxPossibleSources> all_sources_{};

    // Tells us what sources were set
    EnumSet<ExpressionDataSourceType> sources_present_{};
};

struct EffectExpression;

// Context, required to evaluate an effect expression
// Just ExpressionStatsSource is not enough because we are going to use
// not only stats but also other information related to sender and receiver
// entities in effect expressions (for example, shields, zones and
// any non-combat unit entity in general).
// This data will then be passed to CustomEvaluationFunction of effect value.
class ExpressionEvaluationContext
{
public:
    constexpr ExpressionEvaluationContext() = default;
    constexpr ExpressionEvaluationContext(
        const World* world,
        const EntityID sender_id,
        const EntityID receiver_id,
        const EntityID sender_focus = kInvalidEntityID)
        : world_(world),
          sender_id_(sender_id),
          receiver_id_(receiver_id),
          sender_focus_id_(sender_focus)
    {
    }

    ExpressionStatsSource MakeStatsSource(const EnumSet<ExpressionDataSourceType> required_sources) const;

    inline ExpressionStatsSource MakeStatsSource(const EffectExpression& effect_expression) const;

    // Sets new sender entity id and updates stats in related stats source
    void UpdateSenderAndStatsSource(const EntityID entity_id, ExpressionStatsSource& stats_sources_to_update);

    const World* GetWorld() const
    {
        return world_;
    }

    EntityID GetSenderID() const
    {
        return sender_id_;
    }

    EntityID GetReceiverID() const
    {
        return receiver_id_;
    }

private:
    bool CopyStatsIfSameEntity(
        const ExpressionDataSourceType from,
        const ExpressionDataSourceType to,
        ExpressionStatsSource& stats_sources_to_update) const;

    EntityID GetEntityIDForSourceType(const ExpressionDataSourceType source) const
    {
        switch (source)
        {
        case ExpressionDataSourceType::kSender:
            return sender_id_;
        case ExpressionDataSourceType::kReceiver:
            return receiver_id_;
        case ExpressionDataSourceType::kSenderFocus:
            return sender_focus_id_;

        default:
            assert(false);
            break;
        }

        return kInvalidEntityID;
    }

private:
    const World* world_ = nullptr;
    EntityID sender_id_ = kInvalidEntityID;
    EntityID receiver_id_ = kInvalidEntityID;
    EntityID sender_focus_id_ = kInvalidEntityID;
};

// Base value struct use for all values inside the EffectExpression
struct EffectValue
{
    using CustomEvaluationFunction = FixedPoint (*)(
        const ExpressionDataSourceType data_source_type,
        const ExpressionEvaluationContext& context,
        const ExpressionStatsSource& stats_source);

    // Evaluate this EffectValue returning the final value
    FixedPoint Evaluate(const ExpressionEvaluationContext& context, const ExpressionStatsSource& stats_source) const;

    // Is stat a stat percentage type
    bool IsStatAPercentageType() const;

    // Helper to convert stat_evaluation_type to string for the current stat type
    std::string_view GetStatEvaluationTypeAsString() const;

    // Divide the base value by this divisor
    EffectValue& operator/=(const FixedPoint divisor)
    {
        if (divisor == 0_fp)
        {
            return *this;
        }

        value /= divisor;
        return *this;
    }

    constexpr bool IsEmpty() const
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(EffectValueType, 8);
        switch (type)
        {
        case EffectValueType::kValue:
            return value == 0_fp;

        case EffectValueType::kStat:
        case EffectValueType::kStatPercentage:
        case EffectValueType::kStatHighPrecisionPercentage:
        case EffectValueType::kMeteredStatPercentage:
            return stat == StatType::kNone;

        case EffectValueType::kSynergyCount:
            return combat_synergy.IsEmpty();

        case EffectValueType::kCustomFunction:
            return custom_function == nullptr;

        case EffectValueType::kHyperEffectiveness:
            return false;

        default:
            return true;
        }
    }

    EnumSet<ExpressionDataSourceType> GatherRequiredDataSourceTypes() const
    {
        EnumSet<ExpressionDataSourceType> data_source_types;
        ILLUVIUM_ENSURE_ENUM_SIZE(EffectValueType, 8);
        switch (type)
        {
        case EffectValueType::kStat:
        case EffectValueType::kStatPercentage:
        case EffectValueType::kStatHighPrecisionPercentage:
        case EffectValueType::kMeteredStatPercentage:
        case EffectValueType::kSynergyCount:
        case EffectValueType::kCustomFunction:
        case EffectValueType::kHyperEffectiveness:
            if (data_source_type != ExpressionDataSourceType::kNone)
            {
                data_source_types.Add(data_source_type);
            }
            break;

        default:
            break;
        }

        return data_source_types;
    }

    void FormatTo(fmt::format_context& ctx) const;

    // Type of this effect value
    EffectValueType type = EffectValueType::kValue;

    // If type is kAmount this is just a flat amount
    // If type is kStat then this is not used
    // If type is kStatPercentage then this is considered to be a percentage of stat
    FixedPoint value = 0_fp;

    // Only used if type is: kStat and kStatPercentage
    StatType stat = StatType::kNone;

    // The group that stat belongs to.
    StatEvaluationType stat_evaluation_type = StatEvaluationType::kLive;

    // Only used if type is: kSynergyCount
    CombatSynergyBonus combat_synergy;

    // Source of data (stats, synregies...) used for evaluation
    ExpressionDataSourceType data_source_type = ExpressionDataSourceType::kSender;

    // This function may be used if expression evaluates something other than combat unit stats.
    // Only relevant for EffectValueType::kCustomFunction
    CustomEvaluationFunction custom_function = nullptr;
};

// Abstraction for an expression that can be a simple expression (just an amount) value
// or a complex expression.
// Example complex expression:
// 15% Max HP (Receiver) * OmegaPower (Sender) + 300 * OmegaPower (Sender)
struct EffectExpression
{
    // Helper method to create a kValue expression
    // Example: 250
    static EffectExpression FromValue(const FixedPoint value)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kValue;
        expression.base_value.value = value;
        return expression;
    }

    static EffectExpression FromCustomEvaluationFunction(
        EffectValue::CustomEvaluationFunction custom_function,
        const ExpressionDataSourceType required_stat_source_type = ExpressionDataSourceType::kNone)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kCustomFunction;
        expression.base_value.custom_function = custom_function;
        expression.base_value.data_source_type = required_stat_source_type;
        return expression;
    }

    EnumSet<ExpressionDataSourceType> GatherRequiredDataSourceTypes() const
    {
        if (operation_type == EffectOperationType::kNone)
        {
            return base_value.GatherRequiredDataSourceTypes();
        }

        EnumSet<ExpressionDataSourceType> types;
        for (const auto& operand : operands)
        {
            types.Add(operand.GatherRequiredDataSourceTypes());
        }

        return types;
    }

    // Helper method to create a kStat expression
    // Example: CurrentShield (Receiver)
    static EffectExpression FromStat(
        const StatType stat,
        const StatEvaluationType stat_evaluation_type,
        const ExpressionDataSourceType data_source_type)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kStat;
        expression.base_value.stat = stat;
        expression.base_value.stat_evaluation_type = stat_evaluation_type;
        expression.base_value.data_source_type = data_source_type;
        return expression;
    }

    static EffectExpression FromSenderFocusLiveStat(const StatType stat)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kStat;
        expression.base_value.stat = stat;
        expression.base_value.stat_evaluation_type = StatEvaluationType::kLive;
        expression.base_value.data_source_type = ExpressionDataSourceType::kSenderFocus;
        return expression;
    }

    static EffectExpression FromSenderLiveStat(const StatType stat)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kStat;
        expression.base_value.stat = stat;
        expression.base_value.stat_evaluation_type = StatEvaluationType::kLive;
        expression.base_value.data_source_type = ExpressionDataSourceType::kSender;
        return expression;
    }

    // Helper method to create a kStatPercentage expression
    // Example: 20% Physical Resist (Receiver)
    static EffectExpression FromStatPercentage(
        const FixedPoint percentage,
        const StatType stat,
        const StatEvaluationType stat_evaluation_type,
        const ExpressionDataSourceType data_source_type)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kStatPercentage;
        expression.base_value.value = percentage;
        expression.base_value.stat = stat;
        expression.base_value.stat_evaluation_type = stat_evaluation_type;
        expression.base_value.data_source_type = data_source_type;
        return expression;
    }

    // Helper method to create a kStatHighPrecisionPercentage expression
    // Example: 20.25% Physical Resist (Receiver)
    static EffectExpression FromStatHighPrecisionPercentage(
        const FixedPoint high_precision_percentage,
        const StatType stat,
        const StatEvaluationType stat_evaluation_type,
        const ExpressionDataSourceType data_source_type)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kStatHighPrecisionPercentage;
        expression.base_value.value = high_precision_percentage;
        expression.base_value.stat = stat;
        expression.base_value.stat_evaluation_type = stat_evaluation_type;
        expression.base_value.data_source_type = data_source_type;
        return expression;
    }

    // Helper method to create a kSynergyCount expression
    // Example: SynergyCount Water
    static EffectExpression FromSynergyCount(
        const CombatSynergyBonus combat_synergy,
        const ExpressionDataSourceType data_source_type)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kSynergyCount;
        expression.base_value.combat_synergy = combat_synergy;
        expression.base_value.data_source_type = data_source_type;
        return expression;
    }

    static EffectExpression FromHyperEffectiveness(const ExpressionDataSourceType data_source_type)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kHyperEffectiveness;
        expression.base_value.data_source_type = data_source_type;
        return expression;
    }

    static EffectExpression FromMeteredStatPercentage(
        const StatType stat,
        const StatEvaluationType stat_evaluation_type,
        const ExpressionDataSourceType data_source_type)
    {
        EffectExpression expression;
        expression.base_value.type = EffectValueType::kMeteredStatPercentage;
        expression.base_value.stat = stat;
        expression.base_value.stat_evaluation_type = stat_evaluation_type;
        expression.base_value.data_source_type = data_source_type;
        return expression;
    }

    // Evaluate this expression returning the final value (variant where you can pass changed or cached stat source)
    FixedPoint Evaluate(const ExpressionEvaluationContext& context, const ExpressionStatsSource& stats_source) const;

    // Evaluate this expression returning the final value
    FixedPoint Evaluate(const ExpressionEvaluationContext& context) const
    {
        return Evaluate(context, context.MakeStatsSource(*this));
    }

    // Is this a base value expression
    bool IsABaseValue() const
    {
        return operation_type == EffectOperationType::kNone;
    }

    // Is this a base value expression and the base value is a stat value type
    bool IsABaseValueStat() const
    {
        return IsABaseValue() && base_value.type == EffectValueType::kStat;
    }

    void FormatTo(fmt::format_context& ctx) const;

    void ReplaceStat(const StatType to_replace, const StatType replace_with);

    // Copyable or Movable
    EffectExpression() = default;
    EffectExpression(const EffectExpression&) = default;
    EffectExpression& operator=(const EffectExpression&) = default;
    EffectExpression(EffectExpression&&) = default;
    EffectExpression& operator=(EffectExpression&&) = default;
    ~EffectExpression() = default;

    // Base value of this expression, only used if operation_type is None
    EffectValue base_value;

    // Does this expression evaluate to a percentage
    // Used for scaling buffs
    // See:
    // - https://illuvium.atlassian.net/wiki/spaces/AB/pages/44336245/Buff+Effect
    // - https://illuvium.atlassian.net/wiki/spaces/AB/pages/100270284/Standard+Percentage+Value+Type+Combat+Stat
    bool is_used_as_percentage = false;

    // Type of operation to do with the operands
    // If this None then the expression evaluates to base_value
    EffectOperationType operation_type = EffectOperationType::kNone;

    // List of expressions that are the operands for the operation_type
    // Only used if operation_type is valid
    // At the leafs of this everything evaluates to base_value
    std::vector<EffectExpression> operands;

    // Is this expression empty?
    constexpr bool IsEmpty() const
    {
        if (operation_type == EffectOperationType::kNone)
        {
            return base_value.IsEmpty();
        }

        return operands.empty();
    }

    // Add this expression to another by using + operation type onto itself
    EffectExpression& operator+=(const EffectExpression& rhs)
    {
        if (IsEmpty())
        {
            // LHS is empty, so just assign rhs, so that we don't create useless copies
            *this = rhs;
        }
        else
        {
            // Create a copy of this, which is the lhs
            const EffectExpression lhs = *this;

            operation_type = EffectOperationType::kAdd;
            operands = {lhs, rhs};
        }

        return *this;
    }

    // Add this expression to another by using - operation type onto itself
    EffectExpression& operator-=(const EffectExpression& rhs)
    {
        // RHS is empty, just return lhs unmodified
        if (rhs.IsEmpty())
        {
            return *this;
        }

        // Create a copy of this, which is the lhs
        const EffectExpression lhs = *this;

        operation_type = EffectOperationType::kSubtract;
        operands = {lhs, rhs};
        return *this;
    }

    // Divide this expression by another by using / operation type onto itself
    EffectExpression& operator/=(const FixedPoint divisor)
    {
        if (divisor == 0_fp)
        {
            return *this;
        }

        // Create a new divide operation
        const EffectExpression lhs = *this;
        const EffectExpression rhs = EffectExpression::FromValue(divisor);
        operation_type = EffectOperationType::kDivide;
        operands = {lhs, rhs};

        return *this;
    }
};

ExpressionStatsSource ExpressionEvaluationContext::MakeStatsSource(const EffectExpression& effect_expression) const
{
    return MakeStatsSource(effect_expression.GatherRequiredDataSourceTypes());
}

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectValue, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectExpression, FormatTo);
