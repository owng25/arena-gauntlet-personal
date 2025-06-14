#pragma once

#include <optional>

#include "data/ability_data.h"
#include "data/augment/augment_data.h"
#include "data/consumable/consumable_data.h"
#include "data/drone_augment/drone_augment_data.h"
#include "data/encounter_mod_data.h"
#include "data/loaders/base_data_loader.h"
#include "data/suit/suit_data.h"
#include "data/weapon/weapon_data.h"
#include "ecs/world.h"

namespace simulation
{

// Parse and load utilities and publicly exposed methods
class TestingDataLoader : public BaseDataLoader
{
public:
    using Self = TestingDataLoader;
    using Super = BaseDataLoader;
    using Super::Super;  // Use parent class constructor

    template <typename T>
    using LoadFunctionType = bool (BaseDataLoader::*)(const nlohmann::json&, T*) const;

    static std::optional<nlohmann::json> ParseJSON(const std::string_view json_text)
    {
        auto json_object = nlohmann::json::parse(json_text, nullptr, false);
        if (!json_object.is_discarded())
        {
            return json_object;
        }

        return std::nullopt;
    }

    template <typename T>
    std::optional<T> ParseAndLoad(const std::string_view json_text, LoadFunctionType<T> load_function) const
    {
        auto json_object = ParseJSON(json_text);

        T value;
        if (!json_object.has_value() || !(*this.*load_function)(*json_object, &value))
        {
            LogErr("Failed to parse json");
            return std::nullopt;
        }

        return value;
    }

    std::optional<AbilityActivationTriggerData> ParseAndLoadTrigger(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadAbilityActivationTriggerData);
    }

    std::optional<EffectTypeID> ParseAndLoadEffectTypeID(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadEffectTypeID);
    }

    std::optional<EffectData> ParseAndLoadEffect(const std::string_view json_text)
    {
        auto json_object = ParseJSON(json_text);

        EffectData value;
        if (!json_object.has_value() || !LoadEffect(json_object.value(), {}, &value))
        {
            return std::nullopt;
        }

        return value;
    }

    std::optional<EffectData> ParseAndLoadEffectForAura(const std::string_view json_text)
    {
        auto json_object = ParseJSON(json_text);

        EffectData value;
        if (!json_object.has_value() || !LoadEffect(json_object.value(), {.for_aura = true}, &value))
        {
            return std::nullopt;
        }

        return value;
    }

    bool ParseAndLoadAbility(const std::string_view json_text, const AbilityType type, AbilityData* out_ability)
    {
        const auto json_object = ParseJSON(json_text);
        if (!json_object.has_value())
        {
            LogErr("Failed to parse json");
            return false;
        }
        return LoadAbility(json_object.value(), type, out_ability);
    }

    std::optional<EffectExpression> ParseAndLoadExpression(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadEffectExpression);
    }

    bool IsValidTrigger(const std::string_view json_text)
    {
        return ParseAndLoadTrigger(json_text).has_value();
    }

    bool IsValidEffect(const std::string_view json_text)
    {
        return ParseAndLoadEffect(json_text).has_value();
    }

    bool IsValidExpression(const std::string_view json_text)
    {
        return ParseAndLoadExpression(json_text).has_value();
    }

    static TestingDataLoader MakeSilent()
    {
        auto logger = Logger::Create();
        logger->Disable();
        return TestingDataLoader(std::move(logger));
    }

    static TestingDataLoader MakeFrom(const std::shared_ptr<World>& world)
    {
        return TestingDataLoader(world->GetLogger());
    }

    static StatParameters ParseStatParameters(const std::string_view string)
    {
        return Super::ParseStatParameters(string);
    }

    std::optional<AugmentData> ParseAndLoadAugment(const std::string_view json_text)
    {
        return ParseAndLoad<AugmentData>(json_text, &Self::LoadAugmentData);
    }

    std::optional<AugmentTypeID> ParseAndLoadAugmentTypeID(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadAugmentTypeID);
    }

    std::optional<DroneAugmentData> ParseAndLoadDroneAugment(const std::string_view json_text)
    {
        return ParseAndLoad<DroneAugmentData>(json_text, &Self::LoadDroneAugmentData);
    }

    std::optional<DroneAugmentTypeID> ParseAndLoadDroneAugmentTypeID(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadDroneAugmentTypeID);
    }

    std::optional<AugmentInstanceData> ParseAndLoadAugmentInstance(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadAugmentInstanceData);
    }

    std::optional<CombatUnitWeaponData> ParseAndLoadWeapon(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadCombatWeaponData);
    }

    std::optional<CombatWeaponTypeID> ParseAndLoadWeaponTypeID(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadCombatWeaponTypeID);
    }

    std::optional<CombatWeaponInstanceData> ParseAndLoadWeaponInstanceData(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadCombatWeaponInstanceData);
    }

    std::optional<CombatUnitSuitData> ParseAndLoadSuit(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadCombatSuitData);
    }

    std::optional<SkillTargetingData> ParseAndLoadSkillTargetingData(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadSkillTargetingData);
    }

    std::optional<CombatSuitTypeID> ParseAndLoadSuitTypeID(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadCombatSuitTypeID);
    }

    std::optional<ConsumableData> ParseAndLoadConsumable(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadConsumableData);
    }

    std::optional<ConsumableTypeID> ParseAndLoadConsumableTypeID(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadConsumableTypeID);
    }

    std::optional<BattleConfig> ParseAndLoadBattleConfig(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadBattleConfig);
    }

    std::optional<WorldEffectsConfig> ParseAndLoadWorldEffectsConfig(std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadWorldEffectsConfig);
    }

    std::optional<WorldEffectConditionConfig> ParseAndLoadWorldEffectConditionConfig(std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadWorldEffectConditionConfig);
    }

    std::optional<AugmentsConfig> ParseAndLoadAugmentsConfig(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadAugmentsConfig);
    }

    std::optional<ConsumablesConfig> ParseAndLoadConsumablesConfig(const std::string_view json_text)
    {
        return ParseAndLoad(json_text, &Self::LoadConsumablesConfig);
    }

    auto ParseAndLoadEncounterModTypeID(const std::string_view json_text)
    {
        return ParseAndLoad<EncounterModTypeID>(json_text, &Self::LoadEncounterModTypeID);
    }

    auto ParseAndLoadEncounterModData(const std::string_view json_text)
    {
        return ParseAndLoad<EncounterModData>(json_text, &Self::LoadEncounterModData);
    }

    auto ParseAndLoadTeamsEncounterMods(const std::string_view json_text)
    {
        return ParseAndLoad<std::unordered_map<Team, std::vector<EncounterModInstanceData>>>(
            json_text,
            &Self::LoadTeamsEncounterMods);
    }

    std::optional<CombatUnitSuitData> ParseAndLoadSuitData(const std::string_view json_text)
    {
        return ParseAndLoad<CombatUnitSuitData>(json_text, &Self::LoadCombatSuitData);
    }

    std::optional<OverloadConfig> ParseAndLoadOverloadConfig(const std::string_view json_text)
    {
        return ParseAndLoad<OverloadConfig>(json_text, &Self::LoadOverloadConfig);
    }

    std::optional<LevelingConfig> ParseAndLoadLevelingConfig(const std::string_view json_text)
    {
        return ParseAndLoad<LevelingConfig>(json_text, &Self::LoadLevelingConfig);
    }

    std::optional<std::unordered_map<StatType, FixedPoint>> ParseAndLoadStatsMap(const std::string_view json_text)
    {
        if (const auto opt_json = ParseJSON(json_text))
        {
            const bool parse_strings_as_fixed_point = false;
            std::unordered_map<StatType, FixedPoint> out_map;
            if (LoadCombatUnitStats(*opt_json, parse_strings_as_fixed_point, &out_map))
            {
                return out_map;
            }
        }

        return std::nullopt;
    }

    std::optional<std::unordered_map<StatType, FixedPoint>> ParseAndLoadStatsMap_AllowStrings(
        const std::string_view json_text)
    {
        if (const auto opt_json = ParseJSON(json_text))
        {
            const bool parse_strings_as_fixed_point = true;
            std::unordered_map<StatType, FixedPoint> out_map;
            if (LoadCombatUnitStats(*opt_json, parse_strings_as_fixed_point, &out_map))
            {
                return out_map;
            }
        }

        return std::nullopt;
    }
};
}  // namespace simulation
