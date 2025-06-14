#pragma once

#include "effect_expression.h"

namespace simulation
{
class ExpressionEvaluationContext;
class ExpressionStatsSource;
class EffectExpressionCustomFunctions
{
public:
    static FixedPoint GetShieldAmount(
        const ExpressionDataSourceType data_srouce_type,
        const ExpressionEvaluationContext& context,
        const ExpressionStatsSource& stats_source);
};
}  // namespace simulation
