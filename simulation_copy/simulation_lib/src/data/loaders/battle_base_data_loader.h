#pragma once

#include "base_data_loader.h"
#include "data/world_effects_config.h"

namespace simulation
{

class GameDataContainer;

/* -------------------------------------------------------------------------------------------------------
 * BattleBaseDataLoader
 *
 * This class loads data file from JSON data folder.
 * This class exists so that it can be reused in multiple places: simulation-cli, game client, simulation backend lambda
 *
 * --------------------------------------------------------------------------------------------------------
 */
class BattleBaseDataLoader : public BaseDataLoader
{
    using Self = BattleBaseDataLoader;
    using Super = BaseDataLoader;

public:
    explicit BattleBaseDataLoader(std::shared_ptr<Logger> logger);
    ~BattleBaseDataLoader();

    const std::shared_ptr<const GameDataContainer>& GetGameDataContainer() const
    {
        return const_container_;
    }

    std::shared_ptr<const CombatUnitData> LoadCombatUnitFromString(const std::string_view json_string);
    std::shared_ptr<const CombatUnitData> LoadCombatUnitFromJSON(const nlohmann::json& json_object);

    std::shared_ptr<const CombatUnitWeaponData> LoadWeaponFromString(const std::string_view json_string);
    std::shared_ptr<const CombatUnitWeaponData> LoadWeaponFromJSON(const nlohmann::json& json_object);

    std::shared_ptr<const CombatUnitSuitData> LoadSuitFromString(const std::string_view json_string);
    std::shared_ptr<const CombatUnitSuitData> LoadSuitFromJSON(const nlohmann::json& json_object);

    std::shared_ptr<const SynergyData> LoadSynergyFromString(const std::string_view json_string);
    std::shared_ptr<const SynergyData> LoadSynergyFromJSON(const nlohmann::json& json_object);

    std::shared_ptr<const AugmentData> LoadAugmentFromString(const std::string_view json_string);
    std::shared_ptr<const AugmentData> LoadAugmentFromJSON(const nlohmann::json& json_object);

    std::shared_ptr<const DroneAugmentData> LoadDroneAugmentFromString(const std::string_view json_string);
    std::shared_ptr<const DroneAugmentData> LoadDroneAugmentFromJSON(const nlohmann::json& json_object);

    std::shared_ptr<const ConsumableData> LoadConsumableFromString(const std::string_view json_string);
    std::shared_ptr<const ConsumableData> LoadConsumableFromJSON(const nlohmann::json& json_object);

    std::shared_ptr<const EncounterModData> LoadEncounterModFromString(const std::string_view json_string);
    std::shared_ptr<const EncounterModData> LoadEncounterModFromJSON(const nlohmann::json& json_object);

    bool LoadHyperDataFromString(const std::string_view json_string);
    bool LoadHyperDataFromJSON(const nlohmann::json& json_object);

    bool LoadHyperConfigFromString(const std::string_view json_string);
    bool LoadHyperConfigFromJSON(const nlohmann::json& json_object);

    bool LoadWorldEffectsConfigFromJSON(const nlohmann::json& json_object);
    bool LoadWorldEffectsConfigFromString(const std::string_view json_string);

    bool ParseJSON(const std::string_view json_string, nlohmann::json* out_json_object) const;

    void TryLoadPetFromCombatUnitData(const CombatUnitData& data);
    void TryLoadPetFromWeaponData(const CombatUnitWeaponData& data);
    void TryLoadPetFromAbilities(const AbilitiesData& abilities);

protected:
    std::shared_ptr<GameDataContainer> game_data_container_;
    std::shared_ptr<const GameDataContainer> const_container_;
};

}  // namespace simulation
