#include "battle_base_data_loader.h"

#include "base_data_loader.h"
#include "data/combat_unit_data.h"
#include "data/containers/game_data_container.h"
#include "data/drone_augment/drone_augment_data.h"
#include "data/encounter_mod_data.h"
#include "data/loaders/base_data_loader.h"

namespace simulation
{
BattleBaseDataLoader::BattleBaseDataLoader(std::shared_ptr<Logger> logger) : Super(std::move(logger))
{
    game_data_container_ = std::make_shared<GameDataContainer>(GetLogger());
    const_container_ = game_data_container_;
}

BattleBaseDataLoader::~BattleBaseDataLoader() = default;

void BattleBaseDataLoader::TryLoadPetFromAbilities(const AbilitiesData& abilities)
{
    for (const auto& AbilityData : abilities.abilities)
    {
        for (const auto& skill : AbilityData->skills)
        {
            if (skill->deployment.type == SkillDeploymentType::kSpawnedCombatUnit)
            {
                game_data_container_->AddCombatUnitData(skill->spawn.combat_unit->type_id, skill->spawn.combat_unit);
            }
        }
    }
}

std::shared_ptr<const CombatUnitData> BattleBaseDataLoader::LoadCombatUnitFromString(const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadCombatUnitFromString - Failed to Parse JSON");
        return nullptr;
    }

    return LoadCombatUnitFromJSON(json_object);
}

std::shared_ptr<const CombatUnitData> BattleBaseDataLoader::LoadCombatUnitFromJSON(const nlohmann::json& json_object)
{
    auto combat_unit_data = std::make_shared<CombatUnitData>();
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadCombatUnitData(json_object, combat_unit_data.get()))
    {
        LogErr("BattleBaseDataLoader::LoadCombatUnitFromJSON - Failed to load combat unit data from JSON.");
        return nullptr;
    }

    if (game_data_container_->HasCombatUnitData(combat_unit_data->type_id))
    {
        LogErr(
            "BattleBaseDataLoader::LoadCombatUnitFromJSON - Combat Unit type_id = {} already exists",
            combat_unit_data->type_id);
    }

    game_data_container_->AddCombatUnitData(combat_unit_data->type_id, combat_unit_data);
    TryLoadPetFromCombatUnitData(*combat_unit_data);

    return combat_unit_data;
}

std::shared_ptr<const CombatUnitWeaponData> BattleBaseDataLoader::LoadWeaponFromString(
    const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadWeaponFromString - Failed to Parse JSON");
        return nullptr;
    }

    return LoadWeaponFromJSON(json_object);
}

std::shared_ptr<const CombatUnitWeaponData> BattleBaseDataLoader::LoadWeaponFromJSON(const nlohmann::json& json_object)
{
    auto weapon_data = std::make_shared<CombatUnitWeaponData>();
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadCombatWeaponData(json_object, weapon_data.get()))
    {
        LogErr("BattleBaseDataLoader::LoadWeaponFromJSON - Failed to load weapon data from JSON.");
        return nullptr;
    }

    if (game_data_container_->HasWeaponData(weapon_data->type_id))
    {
        LogErr(
            "BattleBaseDataLoader::LoadWeaponFromJSON - Weapon type_id = {{{}}} already exists",
            weapon_data->type_id);
    }

    game_data_container_->AddWeaponData(weapon_data->type_id, weapon_data);
    TryLoadPetFromWeaponData(*weapon_data);

    return weapon_data;
}

std::shared_ptr<const CombatUnitSuitData> BattleBaseDataLoader::LoadSuitFromString(const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadSuitFromString - Failed to Parse JSON");
        return nullptr;
    }

    return LoadSuitFromJSON(json_object);
}

std::shared_ptr<const CombatUnitSuitData> BattleBaseDataLoader::LoadSuitFromJSON(const nlohmann::json& json_object)
{
    auto suit_data = std::make_shared<CombatUnitSuitData>();
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadCombatSuitData(json_object, suit_data.get()))
    {
        LogErr("BattleBaseDataLoader::LoadSuitFromJSON - Failed to load suit data from JSON.");
        return nullptr;
    }

    if (game_data_container_->HasSuitData(suit_data->type_id))
    {
        LogErr("BattleBaseDataLoader::LoadSuitFromJSON - Suit type_id = {{{}}} already exists", suit_data->type_id);
    }

    game_data_container_->AddSuitData(suit_data->type_id, suit_data);
    return suit_data;
}

std::shared_ptr<const SynergyData> BattleBaseDataLoader::LoadSynergyFromString(const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadSynergyFromString - Failed to Parse JSON");
        return nullptr;
    }

    return LoadSynergyFromJSON(json_object);
}

std::shared_ptr<const SynergyData> BattleBaseDataLoader::LoadSynergyFromJSON(const nlohmann::json& json_object)
{
    static constexpr std::string_view method_name = "BattleBaseDataLoader::LoadSynergyFromJSON";

    auto synergy_data = std::make_shared<SynergyData>();
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadSynergyData(json_object, synergy_data.get()))
    {
        LogErr("{} - Failed to load synergy data from JSON.", method_name);
        return nullptr;
    }

    if (synergy_data->IsCombatClass())
    {
        game_data_container_->AddCombatClassSynergyData(synergy_data->GetCombatClass(), synergy_data);
    }
    else if (synergy_data->IsCombatAffinity())
    {
        game_data_container_->AddCombatAffinitySynergyData(synergy_data->GetCombatAffinity(), synergy_data);
    }
    else
    {
        LogErr("{} - Loaded synergy is not combat class and not combat affinity", method_name);
        return nullptr;
    }

    return synergy_data;
}

std::shared_ptr<const AugmentData> BattleBaseDataLoader::LoadAugmentFromString(const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadAugmentFromString - Failed to Parse JSON");
        return nullptr;
    }

    return LoadAugmentFromJSON(json_object);
}

std::shared_ptr<const AugmentData> BattleBaseDataLoader::LoadAugmentFromJSON(const nlohmann::json& json_object)
{
    auto augment_data = std::make_shared<AugmentData>();

    BaseDataLoader loader(GetLogger());

    if (!loader.LoadAugmentData(json_object, augment_data.get()))
    {
        LogErr("BattleBaseDataLoader::LoadAugmentFromJSON - Failed to load augment data from JSON.");
        return nullptr;
    }

    if (game_data_container_->HasAugmentData(augment_data->type_id))
    {
        LogErr(
            "BattleBaseDataLoader::LoadAugmentFromJSON - Augment type_id = {{{}}} already exists",
            augment_data->type_id);
    }

    game_data_container_->AddAugmentData(augment_data->type_id, augment_data);
    return augment_data;
}

std::shared_ptr<const DroneAugmentData> BattleBaseDataLoader::LoadDroneAugmentFromString(
    const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadDroneAugmentFromString - Failed to Parse JSON");
        return nullptr;
    }

    return LoadDroneAugmentFromJSON(json_object);
}

std::shared_ptr<const DroneAugmentData> BattleBaseDataLoader::LoadDroneAugmentFromJSON(
    const nlohmann::json& json_object)
{
    auto drone_augment_data = std::make_shared<DroneAugmentData>();

    BaseDataLoader loader(GetLogger());

    if (!loader.LoadDroneAugmentData(json_object, drone_augment_data.get()))
    {
        LogErr("BattleBaseDataLoader::LoadDroneAugmentFromJSON - Failed to load drone augment data from JSON.");
        return nullptr;
    }

    if (game_data_container_->HasDroneAugmentData(drone_augment_data->type_id))
    {
        LogErr(
            "BattleBaseDataLoader::LoadDroneAugmentFromJSON - Drone augment type_id = {} already exists",
            drone_augment_data->type_id);
    }

    game_data_container_->AddDroneAugmentData(drone_augment_data->type_id, drone_augment_data);
    return drone_augment_data;
}

std::shared_ptr<const ConsumableData> BattleBaseDataLoader::LoadConsumableFromString(const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadConsumableFromString - Failed to Parse JSON");
        return nullptr;
    }

    return LoadConsumableFromJSON(json_object);
}

std::shared_ptr<const ConsumableData> BattleBaseDataLoader::LoadConsumableFromJSON(const nlohmann::json& json_object)
{
    auto consumable_data = std::make_shared<ConsumableData>();
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadConsumableData(json_object, consumable_data.get()))
    {
        LogErr("BattleBaseDataLoader::LoadConsumableFromString - Failed to load consumable data from JSON.");
        return nullptr;
    }

    if (game_data_container_->HasConsumableData(consumable_data->type_id))
    {
        LogErr(
            "BattleBaseDataLoader::LoadConsumableFromJSON - Consumable type_id = {{{}}} already exists",
            consumable_data->type_id);
    }

    game_data_container_->AddConsumableData(consumable_data->type_id, consumable_data);
    return consumable_data;
}

std::shared_ptr<const EncounterModData> BattleBaseDataLoader::LoadEncounterModFromString(
    const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadEncounterModFromString - Failed to Parse JSON");
        return nullptr;
    }

    return LoadEncounterModFromJSON(json_object);
}

std::shared_ptr<const EncounterModData> BattleBaseDataLoader::LoadEncounterModFromJSON(
    const nlohmann::json& json_object)
{
    static constexpr std::string_view method_name = "BattleBaseDataLoader::LoadEncounterModFromJSON";

    auto encounter_mod_data = std::make_shared<EncounterModData>();
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadEncounterModData(json_object, encounter_mod_data.get()))
    {
        LogErr("{} - Failed to load encounter mod data from JSON.", method_name);
        return nullptr;
    }

    if (game_data_container_->HasEncounterModData(encounter_mod_data->type_id))
    {
        LogErr("{} - Encounter mod type_id = {{{}}} already exists", method_name, encounter_mod_data->type_id);
        return nullptr;
    }

    game_data_container_->AddEncounterModData(encounter_mod_data->type_id, encounter_mod_data);

    return encounter_mod_data;
}

bool BattleBaseDataLoader::LoadHyperDataFromString(const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadHyperDataFromString - Failed to Parse JSON");
        return false;
    }

    return LoadHyperDataFromJSON(json_object);
}

bool BattleBaseDataLoader::LoadHyperDataFromJSON(const nlohmann::json& json_object)
{
    auto hyper_data = std::make_unique<HyperData>();
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadHyperData(json_object, hyper_data.get()))
    {
        LogErr("BattleBaseDataLoader::LoadHyperDataFromJSON - Failed to load hyper data from JSON.");
        return false;
    }

    game_data_container_->SetHyperData(std::move(hyper_data));

    return true;
}

bool BattleBaseDataLoader::LoadHyperConfigFromString(const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadHyperConfigFromString - Failed to Parse JSON");
        return false;
    }

    return LoadHyperConfigFromJSON(json_object);
}

bool BattleBaseDataLoader::LoadWorldEffectsConfigFromString(const std::string_view json_string)
{
    nlohmann::json json_object;
    if (!ParseJSON(json_string, &json_object))
    {
        LogErr("BattleBaseDataLoader::LoadWorldEffectsConfigFromString - Failed to Parse JSON");
        return false;
    }

    return LoadWorldEffectsConfigFromJSON(json_object);
}

bool BattleBaseDataLoader::LoadWorldEffectsConfigFromJSON(const nlohmann::json& json_object)
{
    auto effects_config_ = std::make_unique<WorldEffectsConfig>();
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadWorldEffectsConfig(json_object, effects_config_.get()))
    {
        LogErr("BattleBaseDataLoader::LoadWorldEffectsConfigFromJSON - Failed to load hyper config from JSON.");
        return false;
    }

    game_data_container_->SetWorldEffectsConfig(std::move(effects_config_));

    return true;
}

bool BattleBaseDataLoader::LoadHyperConfigFromJSON(const nlohmann::json& json_object)
{
    HyperConfig hyper_config;
    BaseDataLoader loader(GetLogger());
    if (!loader.LoadHyperConfig(json_object, &hyper_config))
    {
        LogErr("BattleBaseDataLoader::LoadHyperConfigFromJSON - Failed to load hyper config from JSON.");
        return false;
    }

    game_data_container_->SetHyperConfig(hyper_config);

    return true;
}

bool BattleBaseDataLoader::ParseJSON(const std::string_view json_string, nlohmann::json* out_json_object) const
{
    assert(out_json_object);
    constexpr bool allow_exceptions = false;
    constexpr bool ignore_comments = false;
    *out_json_object = nlohmann::json::parse(json_string, nullptr, allow_exceptions, ignore_comments);

    // We don't allow exceptions so just check if discarded
    if (out_json_object->is_discarded())
    {
        LogErr("BattleBaseDataLoader::ParseJSON - JSON input is not valid");
        return false;
    }

    return true;
}

void BattleBaseDataLoader::TryLoadPetFromCombatUnitData(const CombatUnitData& data)
{
    TryLoadPetFromAbilities(data.type_data.attack_abilities);
    TryLoadPetFromAbilities(data.type_data.omega_abilities);
    TryLoadPetFromAbilities(data.type_data.innate_abilities);
}

void BattleBaseDataLoader::TryLoadPetFromWeaponData(const CombatUnitWeaponData& data)
{
    TryLoadPetFromAbilities(data.attack_abilities);
    TryLoadPetFromAbilities(data.omega_abilities);
    TryLoadPetFromAbilities(data.innate_abilities);
}

}  // namespace simulation
