#include "battle_data_loader.h"

#include "ecs/world.h"

namespace simulation::tool
{

BattleDataLoader::BattleDataLoader(const std::shared_ptr<Logger>& logger) : Super(logger)
{
    file_helper_ = std::make_unique<FileHelper>(logger);
}

bool BattleDataLoader::LoadAllData(const fs::path& json_data_path)
{
    bool ok = true;
    ok &= LoadCombatUnits(json_data_path / "CombatUnitData");
    ok &= LoadWeapons(json_data_path / "WeaponData");
    ok &= LoadSuits(json_data_path / "SuitData");
    ok &= LoadSynergies(json_data_path / "SynergyData");
    ok &= LoadAugments(json_data_path / "AugmentData");
    ok &= LoadDroneAugments(json_data_path / "DroneAugmentData");
    ok &= LoadHyperData(json_data_path / "HyperData/HyperData.json");
    ok &= LoadHyperConfig(json_data_path / "HyperData/HyperConfig.json");
    ok &= LoadConsumables(json_data_path / "ConsumableData");
    ok &= LoadEncounterMods(json_data_path / "EncounterModData");
    ok &= LoadEffectsConfig(json_data_path / "WorldEffectsConfig/WorldEffectsConfig.json");
    return ok;
}

std::shared_ptr<World> BattleDataLoader::CreateWorld(
    const BattleConfig& battle_config,
    const std::shared_ptr<Logger>& world_logger) const
{
    WorldConfig config{};
    config.logger = world_logger;
    config.stats_constants_scale = 1000_fp;
    config.enable_hyper_system = true;
    config.distance_scan_frequency_time_steps = 1;
    config.battle_config = battle_config;

    return World::Create(config, game_data_container_);
}

bool BattleDataLoader::LoadCombatUnits(const fs::path& path)
{
    if (!FileHelper::DoesDirectoryExist(path))
    {
        LogErr("BattleDataLoader::LoadCombatUnits - Failed to find combat units directory = {}", path);
        return false;
    }

    GetFileHelper().WalkFilesInDirectory(
        path,
        [&](const fs::path& file_name)
        {
            const std::string file_content = GetFileHelper().ReadAllContentFromFile(file_name);
            if (!LoadCombatUnitFromString(file_content))
            {
                LogErr("BattleDataLoader::LoadCombatUnits - Failed to load combat from file_name = {}", file_name);
            }
        });

    return true;
}

bool BattleDataLoader::LoadWeapons(const fs::path& path)
{
    if (!FileHelper::DoesDirectoryExist(path))
    {
        LogErr("BattleDataLoader::LoadWeapons - Failed to find weapons directory = {}", path);
        return false;
    }

    GetFileHelper().WalkFilesInDirectory(
        path,
        [&](const fs::path& file_name)
        {
            const auto file_content = GetFileHelper().ReadAllContentFromFile(file_name);
            if (!LoadWeaponFromString(file_content))
            {
                LogErr("BattleDataLoader::LoadWeapons - Failed to load weapon from file_name = {}", file_name);
            }
        });

    return true;
}

bool BattleDataLoader::LoadSuits(const fs::path& path)
{
    if (!FileHelper::DoesDirectoryExist(path))
    {
        LogErr("BattleDataLoader::LoadSuits - Failed to find suits directory = {}", path);
        return false;
    }

    GetFileHelper().WalkFilesInDirectory(
        path,
        [&](const fs::path& file_name)
        {
            const std::string file_content = GetFileHelper().ReadAllContentFromFile(file_name);
            if (!LoadSuitFromString(file_content))
            {
                LogErr("BattleDataLoader::LoadSuits - Failed to load suit from file_name = {}", file_name);
            }
        });

    return true;
}

bool BattleDataLoader::LoadSynergies(const fs::path& path)
{
    if (!FileHelper::DoesDirectoryExist(path))
    {
        LogErr("BattleDataLoader::LoadSynergies - Failed to find synergies directory = {}", path);
        return false;
    }

    GetFileHelper().WalkFilesInDirectory(
        path,
        [&](const fs::path& file_name)
        {
            const std::string file_content = GetFileHelper().ReadAllContentFromFile(file_name);
            if (!LoadSynergyFromString(file_content))
            {
                LogErr("BattleDataLoader::LoadSynergies - Failed to load synergy from file_name = {}", file_name);
            }
        });

    return true;
}

bool BattleDataLoader::LoadAugments(const fs::path& path)
{
    if (!FileHelper::DoesDirectoryExist(path))
    {
        LogErr("BattleDataLoader::LoadAugments - Failed to find augments directory = {}", path);
        return false;
    }

    GetFileHelper().WalkFilesInDirectory(
        path,
        [&](const fs::path& file_name)
        {
            const std::string file_content = GetFileHelper().ReadAllContentFromFile(file_name);
            if (!LoadAugmentFromString(file_content))
            {
                LogErr("BattleDataLoader::LoadAugments - Failed to load augment from file_name = {}", file_name);
            }
        });

    return true;
}

bool BattleDataLoader::LoadDroneAugments(const fs::path& path)
{
    if (!FileHelper::DoesDirectoryExist(path))
    {
        LogErr("BattleDataLoader::LoadDroneAugments - Failed to find drone augments directory = {}", path);
        return false;
    }

    GetFileHelper().WalkFilesInDirectory(
        path,
        [&](const fs::path& file_name)
        {
            const std::string file_content = GetFileHelper().ReadAllContentFromFile(file_name);
            if (!LoadDroneAugmentFromString(file_content))
            {
                LogErr(
                    "BattleDataLoader::LoadDroneAugments - Failed to load drone augment from file_name = {}",
                    file_name);
            }
        });

    return true;
}

bool BattleDataLoader::LoadConsumables(const fs::path& path)
{
    static constexpr std::string_view method_name = "BattleDataLoader::LoadConsumables";

    if (!FileHelper::DoesDirectoryExist(path))
    {
        LogErr("{} - Failed to find consumables directory = {}", method_name, path);
        return false;
    }

    std::string file_content;
    GetFileHelper().WalkFilesInDirectory(
        path,
        [&](const fs::path& file_name)
        {
            const std::string file_content = GetFileHelper().ReadAllContentFromFile(file_name);
            if (!LoadConsumableFromString(file_content))
            {
                LogErr("{} - Failed to load consumable from file_name = {}", method_name, file_name);
            }
        });

    return true;
}

bool BattleDataLoader::LoadEncounterMods(const fs::path& path)
{
    if (!FileHelper::DoesDirectoryExist(path))
    {
        LogErr("BattleDataLoader::LoadEncounterMds - Directory does not exist = {}", path);
        return false;
    }

    GetFileHelper().WalkFilesInDirectory(
        path,
        [&](const fs::path& file_name)
        {
            const std::string file_content = GetFileHelper().ReadAllContentFromFile(file_name);
            if (!LoadEncounterModFromString(file_content))
            {
                LogErr("BattleDataLoader::LoadConsumables - Failed to load consumable from file_name = {}", file_name);
            }
        });

    return true;
}

bool BattleDataLoader::LoadHyperData(const fs::path& path)
{
    const std::string file_content = GetFileHelper().ReadAllContentFromFile(path);
    if (file_content.empty())
    {
        LogErr("BattleDataLoader::LoadHyperData - Failed to load hyper data from path = {}", path);
        return false;
    }

    if (!LoadHyperDataFromString(file_content))
    {
        LogErr("BattleDataLoader::LoadHyperData - Failed to load hyper data from file_name = {}", path);
        return false;
    }

    return true;
}

bool BattleDataLoader::LoadHyperConfig(const fs::path& path)
{
    const std::string file_content = GetFileHelper().ReadAllContentFromFile(path);
    if (file_content.empty())
    {
        LogErr("BattleDataLoader::LoadHyperConfig - Failed to load hyper config data from path = {}", path);
        return false;
    }

    if (!LoadHyperConfigFromString(file_content))
    {
        LogErr("BattleDataLoader::LoadHyperConfig - Failed to load hyper config from file_name = {}", path);
        return false;
    }

    return true;
}

bool BattleDataLoader::LoadEffectsConfig(const fs::path& path)
{
    const std::string file_content = GetFileHelper().ReadAllContentFromFile(path);
    if (file_content.empty())
    {
        LogErr("BattleDataLoader::LoadEffectsConfig - Failed to load world effects config data from path = {}", path);
        return false;
    }

    if (!LoadWorldEffectsConfigFromString(file_content))
    {
        LogErr("BattleDataLoader::LoadEffectsConfig - Failed to load worl effects config from file_name = {}", path);
        return false;
    }
    return true;
}

}  // namespace simulation::tool
