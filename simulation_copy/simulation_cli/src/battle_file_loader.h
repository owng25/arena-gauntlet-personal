#pragma once

#include "data/loaders/base_data_loader.h"
#include "utility/file_helper.h"

namespace simulation
{
struct CombatUnitInstanceData;
class DroneAugmentsState;
class AugmentInstanceData;
}  // namespace simulation

namespace simulation::tool
{
struct BattleBoardState;
struct BattleCombatUnitState;
struct DroneAugmentState;

/* -------------------------------------------------------------------------------------------------------
 * BattleDataLoader
 *
 * This class loads data for a Battle JSON input
 * --------------------------------------------------------------------------------------------------------
 */
class BattleFileLoader : public simulation::BaseDataLoader
{
    using Self = BattleFileLoader;
    using Super = BaseDataLoader;

public:
    BattleFileLoader(std::shared_ptr<Logger> logger, fs::path battle_files_path);

    // Trying to load battle json file into BattleBoardState
    bool LoadBattleBoardState(const fs::path& file_name, BattleBoardState* out_battle_board_state) const;

private:
    // Find battle file using next priorities:
    // 1) file name
    // 2) file name + .json
    // 3) battle_files_path + file name
    // 4) battle_files_path + file name + .json
    std::string FindBattleFile(const fs::path& file_name) const;

    // Load data of the battle board state from json_object
    bool LoadBattleBoardStateFromJSON(const nlohmann::json& json_object, BattleBoardState* out_battle_board_state)
        const;

    // Load data of the battle combat unit state from json_object
    bool LoadBattleCombatUnitStateFromJSON(
        const nlohmann::json& json_object,
        BattleCombatUnitState* out_combat_unit_state) const;

    // Load data of the battle combat unit state from json_object
    bool LoadBattleCombatUnitInstanceDataFromJSON(
        const nlohmann::json& json_object,
        const CombatUnitTypeID& type_id,
        CombatUnitInstanceData* out_combat_unit_instance_data) const;

    // Load data of the drone augments state from json_object
    bool LoadBattleDroneAugmentStateFromJSON(
        const nlohmann::json& json_object,
        std::vector<DroneAugmentState>* out_drone_augments_state) const;

    std::unique_ptr<FileHelper> file_helper_;
    fs::path battle_files_path_;
};
}  // namespace simulation::tool
