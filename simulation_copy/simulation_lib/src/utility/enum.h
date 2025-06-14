#pragma once

#include <string>
#include <unordered_set>

#include "custom_formatter.h"
#include "data/ability_data.h"
#include "data/combat_synergy_bonus.h"
#include "data/combat_unit_type_id.h"
#include "data/effect_data.h"
#include "data/effect_enums.h"
#include "data/enums.h"
#include "data/validation_data.h"
#include "utility/ensure_enum_size.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * Enum
 *
 * This defines enum helper functions
 * --------------------------------------------------------------------------------------------------------
 */
class Enum
{
public:
    static std::string_view CombatUnitTypeToString(const CombatUnitType type);
    static CombatUnitType StringToCombatUnitType(const std::string_view name);

    static std::string_view EffectDamageTypeToString(const EffectDamageType damage_type);
    static EffectDamageType StringToEffectDamageType(const std::string_view name);

    static std::string_view EffectPositiveStateToString(const EffectPositiveState positive_state);
    static EffectPositiveState StringToEffectPositiveState(const std::string_view name);

    static std::string_view EffectNegativeStateToString(const EffectNegativeState negative_state);
    static EffectNegativeState StringToEffectNegativeState(const std::string_view name);

    static std::string_view EffectPlaneChangeToString(const EffectPlaneChange plane_change);
    static EffectPlaneChange StringToEffectPlaneChange(const std::string_view name);

    static std::string_view EffectConditionTypeToString(const EffectConditionType condition_type);
    static EffectConditionType StringToEffectConditionType(const std::string_view name);

    static std::string_view EffectPropagationTypeToString(const EffectPropagationType propagation_type);
    static EffectPropagationType StringToEffectPropagationType(const std::string_view name);

    static std::string_view EffectDisplacementTypeToString(const EffectDisplacementType displacement_type);
    static EffectDisplacementType StringToEffectDisplacementType(const std::string_view name);

    static std::string_view EffectOverlapProcessTypeToString(const EffectOverlapProcessType overlap_type);
    static EffectOverlapProcessType StringToEffectOverlapProcessType(const std::string_view name);

    static std::string_view MarkEffectTypeToString(const MarkEffectType mark_effect_type);
    static MarkEffectType StringToMarkEffectType(const std::string_view name);

    static std::string_view EffectTypeToString(const EffectType effect_type);
    static EffectType StringToEffectType(const std::string_view name);

    static std::string_view SourceContextTypeToString(const SourceContextType context_type);
    static SourceContextType StringToSourceContextType(const std::string_view name);

    static std::string_view EnergyGainTypeToString(const EnergyGainType energy_gain_type);
    static EnergyGainType StringToEnergyGainType(const std::string_view name);

    static std::string_view HealthGainTypeToString(const HealthGainType health_gain_type);
    static HealthGainType StringToHealthGainType(const std::string_view name);

    static std::string_view EffectHealTypeToString(const EffectHealType effect_heal_type);
    static EffectHealType StringToEffectHealType(const std::string_view name);

    static std::string_view ConditionTypeToString(const ConditionType condition);
    static ConditionType StringToConditionType(const std::string_view name);

    static std::string_view EffectValueTypeToString(const EffectValueType value_type);
    static EffectValueType StringToEffectValueType(const std::string_view name);

    static std::string_view EffectOperationTypeToString(const EffectOperationType operation_type);
    static EffectOperationType StringToEffectOperationType(const std::string_view name);

    static std::string_view ExpressionDataSourceTypeToString(const ExpressionDataSourceType effect_data_source_type);
    static ExpressionDataSourceType StringToExpressionDataSourceType(const std::string_view name);

    static std::string_view ComparisonTypeToString(const ComparisonType comparison_type);
    static ComparisonType StringToComparisonType(const std::string_view name);

    static std::string_view CombatClassToString(const CombatClass combat_class);
    static CombatClass StringToCombatClass(const std::string_view name);

    static std::string_view CombatAffinityToString(const CombatAffinity combat_affinity);
    static CombatAffinity StringToCombatAffinity(const std::string_view name);

    static std::string_view ZoneEffectShapeToString(const ZoneEffectShape shape);
    static ZoneEffectShape StringToZoneEffectShape(const std::string_view name);

    static std::string_view SkillTargetingTypeToString(const SkillTargetingType targeting_type);
    static SkillTargetingType StringToSkillTargetingType(const std::string_view name);

    static std::string_view SkillDeploymentTypeToString(const SkillDeploymentType deployment_type);
    static SkillDeploymentType StringToSkillDeploymentType(const std::string_view name);

    static std::string_view GuidanceTypeToString(const GuidanceType guidance_type);
    static std::string GuidanceTypesToString(const EnumSet<GuidanceType>& guidance_types);
    static GuidanceType StringToGuidanceType(const std::string_view name);

    static std::string_view AllegianceTypeToString(const AllegianceType allegiance_type);
    static AllegianceType StringToAllegianceType(const std::string_view name);

    static std::string_view HexGridCardinalDirectionToString(const HexGridCardinalDirection spawn_direction);
    static HexGridCardinalDirection StringToHexGridCardinalDirection(const std::string_view name);

    static std::string_view AbilitySelectionTypeToString(const AbilitySelectionType ability_selection);
    static AbilitySelectionType StringToAbilitySelectionType(const std::string_view name);

    static std::string_view AbilityTypeToString(const AbilityType ability_type);
    static AbilityType StringToAbilityType(const std::string_view name);

    static std::string_view AbilityUpdateTypeToString(const AbilityUpdateType update_type);
    static AbilityUpdateType StringToAbilityUpdateType(const std::string_view name);

    static std::string_view TeamToString(const Team team);
    static Team StringToTeam(const std::string_view name);

    static std::string_view StatTypeToString(const StatType stat_type);
    static StatType StringToStatType(const std::string_view name);

    static std::string_view StatEvaluationTypeToString(const StatEvaluationType stat_evaluation_type);
    static StatEvaluationType StringToStatEvaluationType(const std::string_view name);

    static std::string_view ActivationTriggerTypeToString(const ActivationTriggerType trigger_type);
    static ActivationTriggerType StringToActivationTriggerType(const std::string_view name);

    static std::string_view EventTypeToString(const EventType stat_type);
    static EventType StringToEventType(const std::string_view name);

    static std::string_view DecisionNextActionToString(const DecisionNextAction next_action);
    static DecisionNextAction StringToDecisionNextAction(const std::string_view name);

    static std::string_view LogLevelToString(const LogLevel log_level);
    static LogLevel StringToLogLevel(const std::string_view name);

    static std::string_view AttachedEffectStateTypeToString(const AttachedEffectStateType attache_effect_state_type);
    static AttachedEffectStateType StringToAttachedEffectStateType(const std::string_view name);

    static std::string_view CombatUnitStateToString(const CombatUnitState state);
    static CombatUnitState StringToCombatUnitState(const std::string_view name);

    static std::string_view ReservedPositionTypeToString(const ReservedPositionType position_type);
    static ReservedPositionType StringToReservedPositionType(const std::string_view name);

    static std::string_view AttachedEntityTypeToString(const AttachedEntityType attached_entity_type);
    static AttachedEntityType StringToAttachedEntityType(const std::string_view attached_entity_name);

    static std::string_view ValidationTypeToString(const ValidationType validation_type);
    static ValidationType StringToValidationType(const std::string_view validation_name);

    static std::string_view ValidationStartEntityToString(const ValidationStartEntity start_entity_type);
    static ValidationStartEntity StringToValidationStartEntity(const std::string_view start_entity_name);

    static std::string_view AugmentTypeToString(const AugmentType augment_type);
    static AugmentType StringToAugmentType(const std::string_view augment_type_name);

    static std::string_view SuitTypeToString(const SuitType suit_type);
    static SuitType StringToSuitType(const std::string_view suit_type_name);

    static std::string_view WeaponTypeToString(const WeaponType weapon_type);
    static WeaponType StringToWeaponType(const std::string_view weapon_type_name);

    static std::string_view DroneAugmentTypeToString(const DroneAugmentType drone_augment_type);
    static DroneAugmentType StringToDroneAugmentType(const std::string_view drone_augment_type_name);

    static std::string_view PredefinedGridPositionToString(const PredefinedGridPosition pos);
    static PredefinedGridPosition StringToPredefinedGridPosition(const std::string_view str);

    static std::string_view AbilityTriggerContextRequirementToString(
        const AbilityTriggerContextRequirement context_requirement_type);
    static AbilityTriggerContextRequirement StringToAbilityTriggerContextRequirement(
        const std::string_view context_requirement_name);

    static std::string_view DeathReasonTypeToString(const DeathReasonType death_reason);
};

}  // namespace simulation

#define ILLUVIUM_MAKE_ENUM_FORMATTER(Name)                                                      \
    template <>                                                                                 \
    struct fmt::formatter<simulation::Name>                                                     \
        : simulation::DefaultEnumFormatter<simulation::Name, &simulation::Enum::Name##ToString> \
    {                                                                                           \
    };                                                                                          \
                                                                                                \
    template <>                                                                                 \
    struct simulation::HasSimFormatter<simulation::Name>                                        \
    {                                                                                           \
        static constexpr bool kValue = true;                                                    \
    }

ILLUVIUM_MAKE_ENUM_FORMATTER(CombatUnitType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectDamageType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectPositiveState);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectNegativeState);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectPlaneChange);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectConditionType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectPropagationType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectDisplacementType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectOverlapProcessType);
ILLUVIUM_MAKE_ENUM_FORMATTER(MarkEffectType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectType);
ILLUVIUM_MAKE_ENUM_FORMATTER(SourceContextType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EnergyGainType);
ILLUVIUM_MAKE_ENUM_FORMATTER(HealthGainType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectHealType);
ILLUVIUM_MAKE_ENUM_FORMATTER(ConditionType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectValueType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EffectOperationType);
ILLUVIUM_MAKE_ENUM_FORMATTER(ExpressionDataSourceType);
ILLUVIUM_MAKE_ENUM_FORMATTER(ComparisonType);
ILLUVIUM_MAKE_ENUM_FORMATTER(CombatClass);
ILLUVIUM_MAKE_ENUM_FORMATTER(CombatAffinity);
ILLUVIUM_MAKE_ENUM_FORMATTER(ZoneEffectShape);
ILLUVIUM_MAKE_ENUM_FORMATTER(SkillTargetingType);
ILLUVIUM_MAKE_ENUM_FORMATTER(SkillDeploymentType);
ILLUVIUM_MAKE_ENUM_FORMATTER(GuidanceType);
ILLUVIUM_MAKE_ENUM_FORMATTER(AllegianceType);
ILLUVIUM_MAKE_ENUM_FORMATTER(HexGridCardinalDirection);
ILLUVIUM_MAKE_ENUM_FORMATTER(AbilitySelectionType);
ILLUVIUM_MAKE_ENUM_FORMATTER(AbilityType);
ILLUVIUM_MAKE_ENUM_FORMATTER(Team);
ILLUVIUM_MAKE_ENUM_FORMATTER(StatType);
ILLUVIUM_MAKE_ENUM_FORMATTER(StatEvaluationType);
ILLUVIUM_MAKE_ENUM_FORMATTER(ActivationTriggerType);
ILLUVIUM_MAKE_ENUM_FORMATTER(EventType);
ILLUVIUM_MAKE_ENUM_FORMATTER(DecisionNextAction);
ILLUVIUM_MAKE_ENUM_FORMATTER(LogLevel);
ILLUVIUM_MAKE_ENUM_FORMATTER(AttachedEffectStateType);
ILLUVIUM_MAKE_ENUM_FORMATTER(CombatUnitState);
ILLUVIUM_MAKE_ENUM_FORMATTER(ReservedPositionType);
ILLUVIUM_MAKE_ENUM_FORMATTER(AttachedEntityType);
ILLUVIUM_MAKE_ENUM_FORMATTER(ValidationType);
ILLUVIUM_MAKE_ENUM_FORMATTER(ValidationStartEntity);
ILLUVIUM_MAKE_ENUM_FORMATTER(AbilityTriggerContextRequirement);
ILLUVIUM_MAKE_ENUM_FORMATTER(AugmentType);
ILLUVIUM_MAKE_ENUM_FORMATTER(SuitType);
ILLUVIUM_MAKE_ENUM_FORMATTER(WeaponType);
ILLUVIUM_MAKE_ENUM_FORMATTER(DeathReasonType);
ILLUVIUM_MAKE_ENUM_FORMATTER(DroneAugmentType);
ILLUVIUM_MAKE_ENUM_FORMATTER(AbilityUpdateType);

#undef ILLUVIUM_MAKE_ENUM_FORMATTER
