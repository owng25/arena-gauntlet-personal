#pragma once

#include <vector>

#include "effect_expression.h"
#include "enums.h"

namespace simulation
{
class World;
struct ValidationEffectExpressionComparison;

// Effect checking validation type
enum class ValidationType
{
    kNone = 0,

    // Distance checking type
    kDistanceCheck,

    // Expression checking type
    // Can me Metered and UnMetered state
    kExpression,

    // -new values can be added above this line
    kNum
};

// Type of starting point
enum class ValidationStartEntity
{
    kNone = 0,

    // Uses self as the checking point.
    kSelf,

    // Uses the current focus as the checking point
    kCurrentFocus,

    // -new values can be added above this line
    kNum
};

// Basic structure to hold common data for validation
struct ValidationBase
{
    // Type of the validation
    ValidationType validation_type = ValidationType::kNone;

    // Comparison type of the validation
    ComparisonType comparison_type = ComparisonType::kNone;
};

// Handles the data and checks the distance between entities
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/250675756/PositiveState.DistanceCheck
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/250479055/NegativeState.DistanceCheck
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/249888966/Empower.ValidationList.DistanceTo
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/250118212/Disempower.ValidationList.DistanceTo
struct ValidationDistanceCheck : ValidationBase
{
    // Distance measuring point
    ValidationStartEntity first_entity = ValidationStartEntity::kSelf;

    // Allegiance type
    AllegianceType allegiance = AllegianceType::kNone;

    // Number of entities in the distance
    int entities_count = 0;

    // Distance radius from entity
    // Note: Measure in Hex
    int distance_units = 0;

    // Check the distance from entity
    bool Check(const World& world, const EntityID entity_id) const;
};

// Handles the Metered and UnMetered state expressions
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/250447534/PositiveState.MeteredCombatStatCheck
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/250511773/PositiveState.UnmeteredCombatStatCheck
struct ValidationEffectExpressionComparison : ValidationBase
{
    EnumSet<ExpressionDataSourceType> GatherRequiredDataSourceTypes() const
    {
        return left.GatherRequiredDataSourceTypes().Union(right.GatherRequiredDataSourceTypes());
    }

    // Type of the expression
    // Note: Can contain Metered and UnMetered stats
    EffectExpression left;

    // Value of the expression
    EffectExpression right;

    // Checking if an effect needs to be activated
    bool Check(const ExpressionEvaluationContext& context, const ExpressionStatsSource& stats_source) const;
};

// Effect validation data
struct EffectValidations
{
    bool IsEmpty() const
    {
        return distance_checks.empty() && expression_comparisons.empty();
    }

    EnumSet<ExpressionDataSourceType> GatherRequiredDataSourceTypes() const
    {
        EnumSet<ExpressionDataSourceType> result;
        for (const ValidationEffectExpressionComparison& comparison_expression : expression_comparisons)
        {
            result.Add(comparison_expression.GatherRequiredDataSourceTypes());
        }

        return result;
    }

    // Data to check distance validation of the effect
    std::vector<ValidationDistanceCheck> distance_checks;

    // Data to check expression validation of the effect
    std::vector<ValidationEffectExpressionComparison> expression_comparisons;
};

}  // namespace simulation
