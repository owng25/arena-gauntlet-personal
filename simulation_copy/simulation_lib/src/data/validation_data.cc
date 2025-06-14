#include "data/validation_data.h"

#include <cassert>

#include "ecs/world.h"
#include "utility/targeting_helper.h"

namespace simulation
{
bool ValidationDistanceCheck::Check(const World& world, const EntityID entity_id) const
{
    const TargetingHelper& targeting_helper = world.GetTargetingHelper();
    const auto withing_range_entities =
        targeting_helper.GetEntitiesWithinRange(entity_id, allegiance, distance_units, false, false, {});

    const auto withing_range_entities_count = FixedPoint::FromInt(static_cast<int>(withing_range_entities.size()));
    return EvaluateComparison(comparison_type, withing_range_entities_count, FixedPoint::FromInt(entities_count));
}

bool ValidationEffectExpressionComparison::Check(
    const ExpressionEvaluationContext& context,
    const ExpressionStatsSource& stats_source) const
{
    //
    const FixedPoint left_value = left.Evaluate(context, stats_source);
    const FixedPoint right_value = right.Evaluate(context, stats_source);
    return EvaluateComparison(comparison_type, left_value, right_value);
}

}  // namespace simulation
