#include "utility/json_helper.h"

#include "data/loaders/json_keys.h"
#include "ecs/world.h"
#include "utility/logger.h"
#include "utility/string.h"

namespace simulation
{
JSONHelper::JSONHelper(std::shared_ptr<Logger> logger) : logger_(std::move(logger)) {}

bool JSONHelper::SetSkillData(
    const nlohmann::json& json_object,
    const AbilityType ability_type,
    const std::string_view key,
    const std::function<bool(const nlohmann::json&, const AbilityType, SkillData*)>& loading_function,
    SkillData* out_skill) const
{
    if (!json_object.contains(key))
    {
        logger_->LogErr("SetSkillData - Failed to extract key attribute '{}'", key);
        return false;
    }

    if (!loading_function(json_object[key], ability_type, out_skill))
    {
        logger_->LogErr("SetSkillData - Failed to load skill for key = '{}'", key);
        return false;
    }

    // Everything loaded okay
    return true;
}

bool JSONHelper::GetConditionTypes(
    const nlohmann::json& json_object,
    std::string_view key,
    EnumSet<ConditionType>* out_condition_types) const
{
    return WalkArray(
        json_object,
        key,
        false,
        [&](const size_t, const nlohmann::json& json_string) -> bool
        {
            if (!json_string.is_string())
            {
                logger_->LogErr(
                    "GetConditionTypes - Key = `{}`, found a value that is not a string type. "
                    "IGNORING",
                    key);
                return false;
            }

            const std::string condition_string{json_string};
            const ConditionType condition_type = Enum::StringToConditionType(condition_string);
            if (condition_type == ConditionType::kNone)
            {
                logger_->LogErr(
                    "GetConditionTypes - Key = `{}`, Condition = `{}` is INVALID. IGNORING ",
                    key,
                    condition_string);
                return false;
            }

            out_condition_types->Add(condition_type);
            return true;
        });
}

bool JSONHelper::HasCombatSynergyKey(const nlohmann::json& json_object) const
{
    return json_object.contains(JSONKeys::kCombatAffinity) || json_object.contains(JSONKeys::kCombatClass);
}

bool JSONHelper::GetCombatSynergy(
    const nlohmann::json& json_object,
    std::string_view key_prefix,
    CombatSynergyBonus* out_combat_synergy) const
{
    const std::string key_affinity = String::Concat(key_prefix, JSONKeys::kCombatAffinity);
    const std::string key_class = String::Concat(key_prefix, JSONKeys::kCombatClass);
    CombatAffinity combat_affinity = CombatAffinity::kNone;
    if (json_object.contains(key_affinity) && !GetEnumValue(json_object, key_affinity, &combat_affinity))
    {
        logger_->LogErr("GetCombatSynergy - Failed to extract Key = `{}`", key_affinity);
        return false;
    }

    CombatClass combat_class = CombatClass::kNone;
    if (json_object.contains(key_class) && !GetEnumValue(json_object, key_class, &combat_class))
    {
        logger_->LogErr("GetCombatSynergy - Failed to extract Key = `{}`", key_class);
        return false;
    }

    if (combat_affinity != CombatAffinity::kNone)
    {
        *out_combat_synergy = combat_affinity;
        return true;
    }
    if (combat_class != CombatClass::kNone)
    {
        *out_combat_synergy = combat_class;
        return true;
    }

    logger_->LogErr("GetCombatSynergy - Failed to extract combat synergies");
    return false;
}

bool JSONHelper::GetStringValue(const nlohmann::json& json_object, const std::string_view key, std::string* out_string)
    const
{
    if (!json_object.contains(key))
    {
        logger_->LogErr("GetStringValue - Key = `{}` does not exist", key);
        return false;
    }

    const auto& json_string = json_object[key];
    if (!GetStringValue(json_string, out_string))
    {
        logger_->LogErr("GetStringValue - Key = `{}` is not a string type", key);
    }

    return true;
}

bool JSONHelper::GetStringValue(const nlohmann::json& json_string, std::string* out_string) const
{
    if (!json_string.is_string())
    {
        logger_->LogErr("GetStringValue - is not a string type");
        return false;
    }

    *out_string = json_string;
    return true;
}

bool JSONHelper::GetIntValue(const nlohmann::json& json_object, int* out_int) const
{
    if (!json_object.is_number_integer())
    {
        logger_->LogErr("JSONHelper::GetIntValue: object is not integer");
        return false;
    }

    const auto int_big = json_object.get<int64_t>();
    if (Math::CastWillOverflow<int>(int_big))
    {
        logger_->LogErr("GetIntValue: \"{}\" value overflows integer bounds.", int_big);
        return false;
    }
    *out_int = static_cast<int>(int_big);

    return true;
}

bool JSONHelper::GetIntValue(const nlohmann::json& json_object, const std::string_view key, int* out_int) const
{
    if (!json_object.contains(key))
    {
        logger_->LogErr("GetIntValue - Key = `{}` does not exist", key);
        return false;
    }

    if (!GetIntValue(json_object[key], out_int))
    {
        logger_->LogErr("Failed to parse integer with key `{}`", key);
        return false;
    }

    return true;
}

bool JSONHelper::GetBoolValue(const nlohmann::json& json_object, const std::string_view key, bool* out_bool) const
{
    if (!json_object.contains(key))
    {
        logger_->LogErr("GetBoolValue - Key = `{}` does not exist", key);
        return false;
    }

    const auto& json_bool = json_object[key];
    if (!json_bool.is_boolean())
    {
        logger_->LogErr("GetBoolValue - Key = `{}` is not an bool type", key);
        return false;
    }

    *out_bool = json_bool;
    return true;
}

bool JSONHelper::WalkArray(
    const nlohmann::json& json_object,
    const std::string_view key,
    const bool return_on_first_failure,
    const std::function<bool(const size_t, const nlohmann::json&)>& walk_function) const
{
    if (!json_object.contains(key))
    {
        logger_->LogErr("WalkArray - Key =`{}` does not exist", key);
        return false;
    }

    const auto& json_array = json_object[key];
    if (!json_array.is_array())
    {
        logger_->LogErr("WalkArray - Key = `{}` is not an array type", key);
        return false;
    }

    return WalkArray(json_array, return_on_first_failure, walk_function);
}

bool JSONHelper::WalkArray(
    const nlohmann::json& json_array,
    const bool return_on_first_failure,
    const std::function<bool(const size_t, const nlohmann::json&)>& walk_function) const
{
    if (!json_array.is_array())
    {
        logger_->LogErr("WalkArray - json_array = `{}`", json_array.dump());
        return false;
    }

    // Read values from array
    bool did_fail = false;
    size_t index = 0;
    for (const auto& json_object_value : json_array)
    {
        if (!walk_function(index, json_object_value))
        {
            did_fail = true;
            if (return_on_first_failure)
            {
                return false;
            }
        }
        index++;
    }

    return !did_fail;
}

bool JSONHelper::GetStringArray(
    const nlohmann::json& json_object,
    const std::string_view key,
    std::vector<std::string>* out_string_array) const
{
    return WalkArray(
        json_object,
        key,
        false,
        [&](const size_t, const nlohmann::json& json_string) -> bool
        {
            if (!json_string.is_string())
            {
                logger_->LogErr(
                    "GetStringArray - Key = `{}`, found a value that is not a string type. "
                    "IGNORING",
                    key);
                return false;
            }

            out_string_array->push_back(json_string);
            return true;
        });
}

bool JSONHelper::GetIntArray(
    const nlohmann::json& json_object,
    const std::string_view key,
    std::vector<int>* out_int_array) const
{
    return WalkArray(
        json_object,
        key,
        false,
        [&](const size_t, const nlohmann::json& json_int) -> bool
        {
            int int_value = 0;
            if (!GetIntValue(json_int, &int_value))
            {
                logger_->LogErr("GetIntArray - Key = `{}`, found a value that is not a valid int type. IGNORING", key);
                return false;
            }

            out_int_array->push_back(int_value);
            return true;
        });
}

}  // namespace simulation
