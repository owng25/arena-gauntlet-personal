#pragma once

#include "data/loaders/base_data_loader.h"
#include "data/loaders/battle_base_data_loader.h"
#include "utility/file_helper.h"

namespace simulation::tool
{
struct BattleCombatUnitState;
struct BattleBoardState;

/* -------------------------------------------------------------------------------------------------------
 * BattleDataLoader
 *
 * This class loads data file from JSON data folder
 * and can use it for create simulation world and filling world from board state.
 * --------------------------------------------------------------------------------------------------------
 */
class BattleDataLoader : public BattleBaseDataLoader
{
    using Self = BattleDataLoader;
    using Super = BattleBaseDataLoader;

public:
    explicit BattleDataLoader(const std::shared_ptr<Logger>& logger);

    // Loads all json data from provided json folder
    bool LoadAllData(const fs::path& json_data_path);

    // Create simulation world using all loaded data
    std::shared_ptr<World> CreateWorld(const BattleConfig& battle_config, const std::shared_ptr<Logger>& world_logger)
        const;

private:
    // Loading functions
    bool LoadCombatUnits(const fs::path& path);
    bool LoadWeapons(const fs::path& path);
    bool LoadSuits(const fs::path& path);
    bool LoadSynergies(const fs::path& path);
    bool LoadAugments(const fs::path& path);
    bool LoadDroneAugments(const fs::path& path);
    bool LoadConsumables(const fs::path& path);
    bool LoadEncounterMods(const fs::path& path);
    bool LoadHyperData(const fs::path& path);
    bool LoadHyperConfig(const fs::path& path);
    bool LoadEffectsConfig(const fs::path& path);

    FileHelper& GetFileHelper() const
    {
        return *file_helper_;
    }

    // Helper functions for file operations
    std::unique_ptr<FileHelper> file_helper_;

    // TODO(shchavinskyi) optimize by creating a cache with maps [id, filename] and load only what is needed
    // [id, filename] cache needs to be updated only when something can't be found/loaded or when path to JSONs is
    // changed
};
}  // namespace simulation::tool
