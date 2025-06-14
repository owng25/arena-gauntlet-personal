#pragma once

#include <string>
#include <vector>

#include "data/stats_data.h"
#include "data/validation_data.h"
#include "nlohmann/json.hpp"
#include "utility/enum.h"
#include "utility/logger.h"

namespace simulation
{
// Helper function to work with nlohmann JSON library
class JSONHelper
{
public:
    explicit JSONHelper(std::shared_ptr<Logger> logger);

    static nlohmann::json FromString(const std::string_view value)
    {
        return nlohmann::json::parse(value);
    }

// Create Getters for all the enum values
#define ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EnumName)                                                                    \
    bool GetEnumValue(const nlohmann::json& json_object, std::string_view key, EnumName* out_enum_value) const         \
    {                                                                                                                  \
        std::string string_value;                                                                                      \
        if (GetStringValue(json_object, key, &string_value))                                                           \
        {                                                                                                              \
            *out_enum_value = Enum::StringTo##EnumName(string_value);                                                  \
        }                                                                                                              \
        if (*out_enum_value != EnumName::kNone)                                                                        \
        {                                                                                                              \
            return true;                                                                                               \
        }                                                                                                              \
                                                                                                                       \
        logger_->LogErr(                                                                                               \
            "GetEnumValue - Key = `{}` failed to extract EnumType = `" #EnumName "` from Value = `{}`",                \
            key,                                                                                                       \
            string_value);                                                                                             \
        return false;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    bool GetEnumValueWithSuffixChar(                                                                                   \
        const nlohmann::json& json_object,                                                                             \
        std::string_view key,                                                                                          \
        EnumName* out_enum_value,                                                                                      \
        const char specific_char,                                                                                      \
        bool* out_specific_char_found) const                                                                           \
    {                                                                                                                  \
        std::string string_value;                                                                                      \
        if (GetStringValue(json_object, key, &string_value))                                                           \
        {                                                                                                              \
            if (string_value.back() == specific_char)                                                                  \
            {                                                                                                          \
                string_value.pop_back();                                                                               \
                *out_specific_char_found = true;                                                                       \
            }                                                                                                          \
            *out_enum_value = Enum::StringTo##EnumName(string_value);                                                  \
        }                                                                                                              \
        if (*out_enum_value != EnumName::kNone)                                                                        \
        {                                                                                                              \
            return true;                                                                                               \
        }                                                                                                              \
                                                                                                                       \
        logger_->LogErr(                                                                                               \
            "GetEnumValue - Key = `{}` failed to extract EnumType = `" #EnumName "` from Value = `{}`",                \
            key,                                                                                                       \
            string_value);                                                                                             \
        return false;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    bool GetEnumValueOptional(const nlohmann::json& json_object, std::string_view key, EnumName* out_enum_value) const \
    {                                                                                                                  \
        if (!json_object.contains(key))                                                                                \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        std::string string_value;                                                                                      \
        if (GetStringValue(json_object, key, &string_value))                                                           \
        {                                                                                                              \
            *out_enum_value = Enum::StringTo##EnumName(string_value);                                                  \
            return true;                                                                                               \
        }                                                                                                              \
        return false;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    bool GetEnumValue(const nlohmann::json& json_enum, EnumName* out_enum_value) const                                 \
    {                                                                                                                  \
        std::string string_value;                                                                                      \
        if (GetStringValue(json_enum, &string_value))                                                                  \
        {                                                                                                              \
            *out_enum_value = Enum::StringTo##EnumName(string_value);                                                  \
        }                                                                                                              \
        if (*out_enum_value != EnumName::kNone)                                                                        \
        {                                                                                                              \
            return true;                                                                                               \
        }                                                                                                              \
                                                                                                                       \
        logger_->LogErr(                                                                                               \
            "GetEnumValue - failed to extract EnumType = `" #EnumName "` from Value = `{}`",                           \
            string_value);                                                                                             \
                                                                                                                       \
        return false;                                                                                                  \
    }

    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(AbilitySelectionType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(AbilityType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(ActivationTriggerType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(AllegianceType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(CombatClass)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(CombatUnitType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(ComparisonType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(ConditionType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(CombatAffinity)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectOperationType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectOverlapProcessType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectPositiveState)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectNegativeState)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectPlaneChange)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectDamageType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectHealType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectConditionType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectPropagationType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(EffectDisplacementType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(HexGridCardinalDirection)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(ReservedPositionType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(SkillTargetingType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(SkillDeploymentType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(GuidanceType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(StatType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(ValidationStartEntity)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(ValidationType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(ZoneEffectShape)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(StatEvaluationType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(MarkEffectType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(ExpressionDataSourceType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(AbilityTriggerContextRequirement)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(AugmentType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(SuitType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(WeaponType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(Team)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(PredefinedGridPosition)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(DroneAugmentType)
    ILLUVIUM_CREATE_GETTER_ENUM_VALUE(AbilityUpdateType)

#undef ILLUVIUM_CREATE_GETTER_ENUM_VALUE

    // Fills an out_skill with a loading_function and from the data object inside
    // json_object[key]
    bool SetSkillData(
        const nlohmann::json& json_object,
        const AbilityType ability_type,
        const std::string_view key,
        const std::function<bool(const nlohmann::json&, const AbilityType, SkillData*)>& loading_function,
        SkillData* out_skill) const;

    // Reads the condition types from the json object
    bool GetConditionTypes(
        const nlohmann::json& json_object,
        std::string_view key,
        EnumSet<ConditionType>* out_condition_types) const;

    // Read the CombatSynergy from the json_object
    bool HasCombatSynergyKey(const nlohmann::json& json_object) const;
    bool GetCombatSynergy(
        const nlohmann::json& json_object,
        std::string_view key_prefix,
        CombatSynergyBonus* out_combat_synergy) const;

    // Getters for basic types
    bool GetStringValue(const nlohmann::json& json_object, const std::string_view key, std::string* out_string) const;
    bool GetStringValue(const nlohmann::json& json_string, std::string* out_string) const;
    bool GetIntValue(const nlohmann::json& json_object, int* out_int) const;
    bool GetIntValue(const nlohmann::json& json_object, const std::string_view key, int* out_int) const;
    bool GetBoolValue(const nlohmann::json& json_object, const std::string_view key, bool* out_bool) const;

    // Walk over the array at json_object[key] with the walk_function being called for each value in
    // the array. If return_on_first_failure = true, then the first time walk_function returns false
    // this function also returns false
    bool WalkArray(
        const nlohmann::json& json_object,
        const std::string_view key,
        const bool return_on_first_failure,
        const std::function<bool(const size_t, const nlohmann::json&)>& walk_function) const;

    // Walk over the array at json_array, with the walk_function being called for each value in
    // the array. If return_on_first_failure = true, then the first time walk_function returns false
    // this function also returns false
    bool WalkArray(
        const nlohmann::json& json_array,
        const bool return_on_first_failure,
        const std::function<bool(const size_t, const nlohmann::json&)>& walk_function) const;

    // Getters for arrays of basic types
    bool GetStringArray(
        const nlohmann::json& json_object,
        const std::string_view key,
        std::vector<std::string>* out_string_array) const;
    bool GetIntArray(const nlohmann::json& json_object, const std::string_view key, std::vector<int>* out_int_array)
        const;

    std::shared_ptr<Logger> GetLogger() const
    {
        return logger_;
    }

private:
    std::shared_ptr<Logger> logger_ = nullptr;
};  // class JSONHelper

}  // namespace simulation
