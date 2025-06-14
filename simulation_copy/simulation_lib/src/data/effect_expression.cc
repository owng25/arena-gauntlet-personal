#include "effect_expression.h"

#include "ecs/world.h"
#include "utility/ensure_enum_size.h"
#include "utility/enum.h"
#include "utility/hyper_helper.h"
#include "utility/stats_helper.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{

ExpressionStatsSource ExpressionEvaluationContext::MakeStatsSource(
    const EnumSet<ExpressionDataSourceType> required_sources) const
{
    ExpressionStatsSource stats_source;

    if (!world_)
    {
        return stats_source;
    }

    if (required_sources.Contains(ExpressionDataSourceType::kSender))
    {
        stats_source.Set(ExpressionDataSourceType::kSender, world_->GetEntityDataForExpression(sender_id_));
    }

    if (required_sources.Contains(ExpressionDataSourceType::kReceiver))
    {
        // Try to copy sender stats if possible
        if (!CopyStatsIfSameEntity(
                ExpressionDataSourceType::kSender,
                ExpressionDataSourceType::kReceiver,
                stats_source))
        {
            stats_source.Set(ExpressionDataSourceType::kReceiver, world_->GetEntityDataForExpression(receiver_id_));
        }
    }

    if (required_sources.Contains(ExpressionDataSourceType::kSenderFocus))
    {
        // Try to copy receiver stats if possible
        if (!CopyStatsIfSameEntity(
                ExpressionDataSourceType::kReceiver,
                ExpressionDataSourceType::kSenderFocus,
                stats_source))
        {
            stats_source.Set(
                ExpressionDataSourceType::kSenderFocus,
                stats_source.Get(ExpressionDataSourceType::kReceiver));
        }
    }

    return stats_source;
}

void ExpressionEvaluationContext::UpdateSenderAndStatsSource(
    const EntityID entity_id,
    ExpressionStatsSource& stats_source)
{
    if (sender_id_ == entity_id && stats_source.Contains(ExpressionDataSourceType::kSender))
    {
        return;
    }

    sender_id_ = entity_id;
    if (!CopyStatsIfSameEntity(ExpressionDataSourceType::kReceiver, ExpressionDataSourceType::kSender, stats_source))
    {
        stats_source.Set(ExpressionDataSourceType::kSender, world_->GetEntityDataForExpression(sender_id_));
    }
}

bool ExpressionEvaluationContext::CopyStatsIfSameEntity(
    const ExpressionDataSourceType from,
    const ExpressionDataSourceType to,
    ExpressionStatsSource& stats_source) const
{
    const EntityID from_entity_id = GetEntityIDForSourceType(from);
    const EntityID to_entity_id = GetEntityIDForSourceType(to);
    if (from_entity_id != kInvalidEntityID && from_entity_id == to_entity_id && stats_source.Contains(from))
    {
        stats_source.Set(to, stats_source.Get(from));
        return true;
    }

    return false;
}

FixedPoint EffectExpression::Evaluate(
    const ExpressionEvaluationContext& context,
    const ExpressionStatsSource& stats_source) const
{
    // Evaluate base value
    if (operation_type == EffectOperationType::kNone)
    {
        return base_value.Evaluate(context, stats_source);
    }

    // We handle percentage types differently because we don't have floats
    // So we must simulate it
    bool is_first_value_a_percentage = false;
    bool handled_first_value_percentage = false;

    // Evaluate operands
    bool is_first_value_set = false;

    FixedPoint final_value = 0_fp;
    for (const EffectExpression& operand : operands)
    {
        const FixedPoint operand_value = operand.Evaluate(context, stats_source);

        // For the first value we just set it without doing any operation with it
        if (!is_first_value_set)
        {
            if (operand.IsABaseValueStat() && operand.base_value.IsStatAPercentageType())
            {
                is_first_value_a_percentage = true;
            }

            final_value = operand_value;
            is_first_value_set = true;
            continue;
        }

        ILLUVIUM_ENSURE_ENUM_SIZE(EffectOperationType, 10);
        switch (operation_type)
        {
        case EffectOperationType::kAdd:
        {
            final_value += operand_value;
            break;
        }
        case EffectOperationType::kSubtract:
        {
            final_value -= operand_value;
            break;
        }
        case EffectOperationType::kMultiply:
        {
            if (operand.IsABaseValueStat() && operand.base_value.IsStatAPercentageType())
            {
                // Operand is a percentage stat type handle operand_value (which is an int) as a
                // float operand_value percentage of final_value <=> operand_value / 100 *
                // final_value
                final_value = operand_value.AsPercentageOf(final_value);
            }
            else if (is_first_value_a_percentage && !handled_first_value_percentage)
            {
                // Handle edge-cases for when the first value is a percentage stat type
                // final_value is a percentage so we must evaluate the percentage against the
                // current operand_value.
                // Example: OmegaPower (Sender) * 2000 final_value = OmegaPower (Sender) operand_value = 200
                final_value = final_value.AsPercentageOf(operand_value);
                handled_first_value_percentage = true;
            }
            else
            {
                // Simple case
                final_value *= operand_value;
            }
            break;
        }
        case EffectOperationType::kDivide:
        {
            final_value /= operand_value;
            break;
        }
        case EffectOperationType::kIntegralDivision:
        {
            final_value = (final_value / operand_value).Floor();
            break;
        }

        case EffectOperationType::kPercentageOf:
        {
            // final_value is percentage of previous final value
            final_value = final_value.AsPercentageOf(operand_value);
            break;
        }
        case EffectOperationType::kHighPrecisionPercentageOf:
        {
            // final_value is percentage of previous final value
            final_value = final_value.AsHighPrecisionPercentageOf(operand_value);
            break;
        }
        case EffectOperationType::kMin:
        {
            final_value = std::min(final_value, operand_value);
            break;
        }
        case EffectOperationType::kMax:
        {
            final_value = std::max(final_value, operand_value);
            break;
        }
        default:
            break;
        }
    }

    // Handle special case when operation_type is multiply and all operands are a stat percentage
    // type
    if (operation_type == EffectOperationType::kMultiply && is_first_value_a_percentage &&
        !handled_first_value_percentage)
    {
        final_value /= kMaxPercentageFP;
    }

    return final_value;
}

FixedPoint EffectValue::Evaluate(const ExpressionEvaluationContext& context, const ExpressionStatsSource& stats_source)
    const
{
    ILLUVIUM_ENSURE_ENUM_SIZE(EffectValueType, 8);
    switch (type)
    {
    case EffectValueType::kValue:
    {
        // Example: 250
        return value;
    }
    case EffectValueType::kStat:
    {
        // Example: CurrentShield (Receiver)
        // NOTE: We don't use the value here
        const FullStatsData& stats = stats_source.Get(data_source_type).stats;
        return stats.Get(stat, stat_evaluation_type);
    }
    case EffectValueType::kStatPercentage:
    case EffectValueType::kStatHighPrecisionPercentage:
    {
        // Example: 20% Armour (Receiver)
        const FullStatsData& stats = stats_source.Get(data_source_type).stats;
        const FixedPoint stat_value = stats.Get(stat, stat_evaluation_type);

        // NOTE: we use the value as a percentage here
        if (type == EffectValueType::kStatPercentage)
        {
            return value.AsPercentageOf(stat_value);
        }

        // kStatHighPrecisionPercentage
        return value.AsHighPrecisionPercentageOf(stat_value);
    }
    case EffectValueType::kSynergyCount:
    {
        // Find the synergy stacks count for this combat_synergy
        const std::vector<SynergyOrder>* psynergies = stats_source.Get(data_source_type).synergies;
        if (psynergies)
        {
            for (const SynergyOrder& synergy_order : *psynergies)
            {
                if (synergy_order.combat_synergy == combat_synergy)
                {
                    return FixedPoint::FromInt(synergy_order.synergy_stacks);
                }
            }
        }

        // Does not have any value
        return 0_fp;
    }
    case EffectValueType::kMeteredStatPercentage:
    {
        const FullStatsData& stats = stats_source.Get(data_source_type).stats;
        const FixedPoint corresponding_max = stats.live.GetMeteredStatCorrespondingMaxValue(stat);
        const FixedPoint stat_value = stats.Get(stat, stat_evaluation_type);

        if (corresponding_max == 0_fp)
        {
            return 0_fp;
        }

        return stat_value * kMaxPercentageFP / corresponding_max;
    }
    case EffectValueType::kCustomFunction:
    {
        return custom_function(data_source_type, context, stats_source);
    }
    case EffectValueType::kHyperEffectiveness:
    {
        EntityID entity = kInvalidEntityID;
        switch (data_source_type)
        {
        case ExpressionDataSourceType::kSender:
            entity = context.GetSenderID();
            break;
        case ExpressionDataSourceType::kReceiver:
            entity = context.GetReceiverID();
            break;
        default:
            assert(false);
            break;
        }

        if (entity == kInvalidEntityID)
        {
            assert(false);
            return 0_fp;
        }

        return FixedPoint::FromInt(HyperHelper::CalculateHyperGrowthEffectiveness(*context.GetWorld(), entity));
    }
    default:
        return 0_fp;
    }
}

bool EffectValue::IsStatAPercentageType() const
{
    return StatsHelper::IsPercentageTypeForExpression(stat);
}

std::string_view EffectValue::GetStatEvaluationTypeAsString() const
{
    // For metered stats it does not make sense to display the group.
    // As we do not have live/base/bonus for the current health for example, there is just current health.
    return StatsHelper::IsMeteredStatType(stat) ? "" : Enum::StatEvaluationTypeToString(stat_evaluation_type);
}

void EffectValue::FormatTo(fmt::format_context& ctx) const
{
    ILLUVIUM_ENSURE_ENUM_SIZE(EffectValueType, 8);
    StructFormattingHelper h(ctx);
    switch (type)
    {
    case EffectValueType::kValue:
    {
        // Example: 250
        h.Write("{}", value);
        break;
    }
    case EffectValueType::kStat:
    {
        // Example: LiveAttackDamage (Receiver)
        // NOTE: We don't use the value here
        h.Write("{}{} ({})", GetStatEvaluationTypeAsString(), stat, data_source_type);
        break;
    }
    case EffectValueType::kStatPercentage:
    {
        // Example: 20% LiveAttackDamage (Receiver)
        h.Write("{}% {}{} ({})", value, GetStatEvaluationTypeAsString(), stat, data_source_type);
        break;
    }
    case EffectValueType::kStatHighPrecisionPercentage:
    {
        // Example: 25.25% LiveAttackDamage (Receiver)
        // Shows last two decimals
        h.Write("{:.2f}% {}{} ({})", value.AsFloat() / 100.f, GetStatEvaluationTypeAsString(), stat, data_source_type);
        break;
    }
    case EffectValueType::kSynergyCount:
    {
        // Example: SynergyCount Water
        h.Write("SynergyCount {}", combat_synergy);
        break;
    }
    case EffectValueType::kMeteredStatPercentage:
    {
        // NOTE: Assumes always that stat_evaluation_type is StatEvaluationType::kLive
        // Current Health % (Receiver) = Current Health / Max Health (Receiver)
        h.Write("{}% {} ({}) = Stat Current Value / Stat Max Value", value, stat, data_source_type);
        break;
    }
    case EffectValueType::kHyperEffectiveness:
    {
        h.Write("HyperEffectiveness");
        break;
    }
    default:
        h.Write("NOT IMPLEMENTED");
        break;
    }
}

void EffectExpression::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("(");
    // To String base value
    if (operation_type == simulation::EffectOperationType::kNone)
    {
        h.Write("{}", base_value);
    }
    else
    {
        // Make a separator object that wil me printed as " op "
        // And join operands using that separator
        std::string separator = fmt::format(" {} ", operation_type);
        h.Write("{}", fmt::join(operands, separator));
    }

    if (is_used_as_percentage)
    {
        h.Write(" % (IsUsedAsPercentage)");
    }
    h.Write(")");
}

void EffectExpression::ReplaceStat(const StatType to_replace, const StatType replace_with)
{
    if (base_value.stat == to_replace)
    {
        base_value.stat = replace_with;
    }

    for (auto& operand_expression : operands)
    {
        operand_expression.ReplaceStat(to_replace, replace_with);
    }
}

}  // namespace simulation
