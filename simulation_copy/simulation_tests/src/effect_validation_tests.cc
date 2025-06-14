#include "base_test_fixtures.h"
#include "components/stats_component.h"
#include "utility/attached_effects_helper.h"
#include "utility/stats_helper.h"

namespace simulation
{
class EffectValidationTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);

        blue_entity_id = blue_entity->GetID();
        red_entity_id = red_entity->GetID();
    }

    void SetUp() override
    {
        Super::SetUp();

        InitCombatUnitData();
        SpawnCombatUnits();
    }

    ValidationEffectExpressionComparison
    CreateExpressionCheck(const StatType stat_type, const FixedPoint value, const ComparisonType new_comparison)
    {
        ValidationEffectExpressionComparison validation_effect_expression_comparison;
        validation_effect_expression_comparison.validation_type = ValidationType::kExpression;
        validation_effect_expression_comparison.comparison_type = new_comparison;

        if (StatsHelper::IsMeteredStatType(stat_type))
        {
            validation_effect_expression_comparison.left = EffectExpression::FromMeteredStatPercentage(
                stat_type,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender);
        }
        else
        {
            validation_effect_expression_comparison.left =
                EffectExpression::FromStat(stat_type, StatEvaluationType::kLive, ExpressionDataSourceType::kSender);
        }
        validation_effect_expression_comparison.right = EffectExpression::FromValue(value);

        return validation_effect_expression_comparison;
    }

    void UpdateExpressionCheck(
        const StatType stat_type,
        const FixedPoint value,
        const ComparisonType new_comparison,
        ValidationEffectExpressionComparison& out_validation_effect_expression_comparison)
    {
        out_validation_effect_expression_comparison.validation_type = ValidationType::kExpression;
        out_validation_effect_expression_comparison.comparison_type = new_comparison;

        if (StatsHelper::IsMeteredStatType(stat_type))
        {
            out_validation_effect_expression_comparison.left = EffectExpression::FromMeteredStatPercentage(
                stat_type,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender);
        }
        else
        {
            out_validation_effect_expression_comparison.left =
                EffectExpression::FromStat(stat_type, StatEvaluationType::kLive, ExpressionDataSourceType::kSender);
        }
        out_validation_effect_expression_comparison.right = EffectExpression::FromValue(value);
    }

    ValidationDistanceCheck CreateDistanceCheck(
        const int new_distance,
        const int entities_count,
        const AllegianceType allegiance_type,
        const ComparisonType new_comparison)
    {
        ValidationDistanceCheck validation_distance_check;
        validation_distance_check.validation_type = ValidationType::kDistanceCheck;
        validation_distance_check.distance_units = new_distance;
        validation_distance_check.entities_count = entities_count;
        validation_distance_check.comparison_type = new_comparison;
        validation_distance_check.allegiance = allegiance_type;
        return validation_distance_check;
    }

    void UpdateDistanceCheck(
        const int new_distance,
        const int entities_count,
        const AllegianceType allegiance_type,
        const ComparisonType new_comparison,
        ValidationDistanceCheck& out_validation_distance_check)
    {
        out_validation_distance_check.validation_type = ValidationType::kDistanceCheck;
        out_validation_distance_check.distance_units = new_distance;
        out_validation_distance_check.entities_count = entities_count;
        out_validation_distance_check.comparison_type = new_comparison;
        out_validation_distance_check.allegiance = allegiance_type;
    }

    bool CheckExpression(
        const ValidationEffectExpressionComparison& expression,
        const EntityID sender_id,
        const EntityID receiver_id)
    {
        const ExpressionEvaluationContext context(world.get(), sender_id, receiver_id);
        const ExpressionStatsSource stats_source = context.MakeStatsSource(expression.GatherRequiredDataSourceTypes());
        return expression.Check(context, stats_source);
    }

    EntityID blue_entity_id = kInvalidEntityID;
    EntityID red_entity_id = kInvalidEntityID;
    Entity* blue_entity = nullptr;
    Entity* blue_entity2 = nullptr;
    Entity* red_entity = nullptr;
    CombatUnitData data;
};

TEST_F(EffectValidationTest, ValidationExpressionCheck)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    ValidationEffectExpressionComparison validation_expression_check;

    // True cases
    {
        blue_stats_component.SetCurrentHealth(800_fp);

        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kGreater, validation_expression_check);
        ASSERT_TRUE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }
    {
        blue_stats_component.SetCurrentHealth(400_fp);
        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kLess, validation_expression_check);
        ASSERT_TRUE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }
    {
        blue_stats_component.SetCurrentHealth(500_fp);
        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kEqual, validation_expression_check);
        ASSERT_TRUE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }

    // False cases
    // kEqual
    {
        blue_stats_component.SetCurrentHealth(800_fp);
        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kEqual, validation_expression_check);
        ASSERT_FALSE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }
    {
        blue_stats_component.SetCurrentHealth(400_fp);
        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kEqual, validation_expression_check);
        ASSERT_FALSE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }
    // kGreater
    {
        blue_stats_component.SetCurrentHealth(500_fp);
        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kGreater, validation_expression_check);
        ASSERT_FALSE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }
    {
        blue_stats_component.SetCurrentHealth(200_fp);
        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kGreater, validation_expression_check);
        ASSERT_FALSE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }
    // kLess
    {
        blue_stats_component.SetCurrentHealth(500_fp);
        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kLess, validation_expression_check);
        ASSERT_FALSE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }
    {
        blue_stats_component.SetCurrentHealth(800_fp);
        UpdateExpressionCheck(StatType::kCurrentHealth, 50_fp, ComparisonType::kLess, validation_expression_check);
        ASSERT_FALSE(CheckExpression(validation_expression_check, blue_entity_id, red_entity_id));
    }
}

TEST_F(EffectValidationTest, ValidationDistanceCheck)
{
    ValidationDistanceCheck validation_distance_check;

    // Entity count
    {
        // Wrong entity count
        UpdateDistanceCheck(20, 2, AllegianceType::kEnemy, ComparisonType::kEqual, validation_distance_check);
        const bool check = validation_distance_check.Check(*world, blue_entity_id);
        ASSERT_FALSE(check);
    }
    {
        // Wrong entity count
        UpdateDistanceCheck(20, 2, AllegianceType::kEnemy, ComparisonType::kGreater, validation_distance_check);
        const bool check = validation_distance_check.Check(*world, blue_entity_id);
        ASSERT_FALSE(check);
    }
    {
        // Right entity count
        UpdateDistanceCheck(20, 2, AllegianceType::kEnemy, ComparisonType::kLess, validation_distance_check);
        const bool check = validation_distance_check.Check(*world, blue_entity_id);
        ASSERT_TRUE(check);
    }

    // Range
    {
        // In range
        UpdateDistanceCheck(20, 1, AllegianceType::kEnemy, ComparisonType::kEqual, validation_distance_check);
        const bool check = validation_distance_check.Check(*world, blue_entity_id);
        ASSERT_TRUE(check);
    }
    {
        // Not in range
        UpdateDistanceCheck(10, 1, AllegianceType::kEnemy, ComparisonType::kEqual, validation_distance_check);
        const bool check = validation_distance_check.Check(*world, blue_entity_id);
        ASSERT_FALSE(check);
    }
}

TEST_F(EffectValidationTest, RemoveEffect_ValidationListNotValid)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();

    red_stats_component.GetMutableTemplateStats()
        .Set(StatType::kMaxHealth, 10000_fp)
        .Set(StatType::kCurrentHealth, 10000_fp);

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 10000_fp);

    // Add Validation expression
    ValidationEffectExpressionComparison validation_effect_expression_comparison;
    validation_effect_expression_comparison.validation_type = ValidationType::kExpression;
    validation_effect_expression_comparison.comparison_type = ComparisonType::kLess;
    validation_effect_expression_comparison.left = EffectExpression::FromStat(
        StatType::kCurrentHealth,
        StatEvaluationType::kLive,
        ExpressionDataSourceType::kSender);
    validation_effect_expression_comparison.right = EffectExpression::FromValue(9000_fp);

    // Add invulnerability
    auto invulnerable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 8000);
    EffectState invulnerable_effect_state{};
    invulnerable_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());

    invulnerable_effect_data.validations.expression_comparisons.push_back(validation_effect_expression_comparison);
    invulnerable_effect_data.lifetime.deactivate_if_validation_list_not_valid = true;
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), invulnerable_effect_data, invulnerable_effect_state);

    const auto& attached_effect_states = red_attached_effects_component.GetAttachedEffects();

    TimeStepForNTimeSteps(kTimeStepsPerSecond);

    // Invulnerability is attached but destroyed
    red_attached_effects_component.EraseDestroyedEffects(true);
    ASSERT_EQ(attached_effect_states.size(), 0);
}

TEST_F(EffectValidationTest, DeactivateActiveEffect)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();

    red_stats_component.GetMutableTemplateStats()
        .Set(StatType::kMaxHealth, 10000_fp)
        .Set(StatType::kCurrentHealth, 10000_fp);

    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 10000_fp);

    // Add Validation expression
    ValidationEffectExpressionComparison validation_effect_expression_comparison;
    validation_effect_expression_comparison.validation_type = ValidationType::kExpression;
    validation_effect_expression_comparison.comparison_type = ComparisonType::kGreater;
    validation_effect_expression_comparison.left = EffectExpression::FromStat(
        StatType::kCurrentHealth,
        StatEvaluationType::kLive,
        ExpressionDataSourceType::kSender);
    validation_effect_expression_comparison.right = EffectExpression::FromValue(9000_fp);

    // Add invulnerability
    auto invulnerable_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 8000);
    EffectState invulnerable_effect_state{};
    invulnerable_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    invulnerable_effect_data.validations.expression_comparisons.push_back(validation_effect_expression_comparison);
    invulnerable_effect_data.lifetime.deactivate_if_validation_list_not_valid = true;
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), invulnerable_effect_data, invulnerable_effect_state);

    const auto& attached_effect_states = red_attached_effects_component.GetAttachedEffects();

    // Activates ability
    TimeStepForMs(100);

    // Invulnerability is attached but destroyed
    ASSERT_EQ(attached_effect_states.size(), 1);
    ASSERT_EQ(attached_effect_states.at(0)->state, AttachedEffectStateType::kActive);

    // Set health lower of validation size to deactivate effect
    red_stats_component.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 8000_fp);

    // Deactivate ability
    TimeStepForMs(100);

    red_attached_effects_component.EraseDestroyedEffects(true);
    ASSERT_EQ(attached_effect_states.size(), 0);
}

}  // namespace simulation
