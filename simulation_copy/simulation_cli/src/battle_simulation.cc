#include "battle_simulation.h"

#include "battle_data_loader.h"
#include "battle_file_data.h"
#include "battle_file_loader.h"
#include "cli_settings.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"

namespace simulation::tool
{

BattleSimulation::BattleSimulation(std::shared_ptr<CLISettings> settings) : settings_(std::move(settings))
{
    data_loading_logger_ = Logger::Create(settings_->IsDebugLogsEnabled());
    data_loading_logger_->SinkAddStdout();
    data_loading_logger_->SetLogsPattern(settings_->GetLogPattern());

    // Use the same in the default case
    world_logger_ = data_loading_logger_;

    data_loader_ = std::make_unique<BattleDataLoader>(data_loading_logger_);

    const fs::path json_data_path = settings_->GetJSONDataPath();

    data_loading_logger_->LogDebug(
        "json_data_path = {}, battle_files_path = {}",
        json_data_path,
        settings_->GetBattleFilesPath());
    if (!data_loader_->LoadAllData(json_data_path))
    {
        LogErr("Failed to load json data from folder {}.", json_data_path);
        return;
    }
}

BattleSimulation::BattleSimulation(
    std::shared_ptr<CLISettings> settings,
    const std::shared_ptr<Logger>& data_loading_logger,
    const std::shared_ptr<Logger>& world_logger)
    : settings_(std::move(settings)),
      data_loading_logger_(data_loading_logger),
      world_logger_(world_logger)
{
    data_loader_ = std::make_unique<BattleDataLoader>(data_loading_logger_);

    const fs::path json_data_path = settings_->GetJSONDataPath();
    if (!data_loader_->LoadAllData(json_data_path))
    {
        LogErr("Failed to load json data from folder {}.", json_data_path);
        return;
    }
}

std::shared_ptr<World> BattleSimulation::OpenBattleFile(
    const fs::path& file_name,
    std::optional<uint64_t> random_seed,
    const std::shared_ptr<Logger>& custom_world_logger)
{
    // Load the board state

    // Set some optional defaults
    BattleBoardState board_state;
    board_state.battle_config.grid_height = 111;
    board_state.battle_config.grid_width = 105;
    board_state.battle_config.grid_scale = 10;
    board_state.battle_config.middle_line_width = 0;
    board_state.battle_config.overload_config.enable_overload_system = true;
    board_state.battle_config.overload_config.increase_overload_damage_percentage = 5_fp;
    board_state.battle_config.overload_config.start_seconds_apply_overload_damage = 45;

    const auto battle_files_path = settings_->GetBattleFilesPath();
    const BattleFileLoader battle_loader(data_loading_logger_, battle_files_path);

    if (!battle_loader.LoadBattleBoardState(file_name, &board_state))
    {
        LogErr("RunBattle - Failed to load board state from battle_files_path = {}", battle_files_path);
        return nullptr;
    }

    // Override random seed if specified
    if (random_seed.has_value())
    {
        board_state.battle_config.random_seed = random_seed.value();
    }

    auto world_ =
        data_loader_->CreateWorld(board_state.battle_config, custom_world_logger ? custom_world_logger : world_logger_);

    for (const DroneAugmentState& drone_augment : board_state.drone_augments)
    {
        if (!SpawnDroneAugment(*world_, drone_augment))
        {
            LogErr("RunBattle - Failed to spawn drone augment = {}", drone_augment.type_id);
            return nullptr;
        }
    }

    for (const BattleCombatUnitState& state : board_state.combat_units)
    {
        if (!SpawnCombatUnit(*world_, state))
        {
            LogErr("RunBattle - Failed to spawn unit = {}", state.type_id);
            return nullptr;
        }
    }

    if (!world_)
    {
        LogErr("RunBattle - Failed to create world, see above errors");
    }

    return world_;
}

void BattleSimulation::TimeStepUntilFinished(const std::shared_ptr<World>& world) const
{
    std::shared_ptr<Logger> world_logger = world->GetLogger();

    while (true)
    {
        world->TimeStep();

        // Event happened, quit
        if (world->IsBattleFinished())
        {
            const auto& battle_result = world->GetBattleResult();

            world_logger->LogDebug(
                "Simulation finished, winner is \"{}\", Took {} time steps.",
                battle_result.winning_team,
                battle_result.duration_time_steps);

            world_logger->LogInfo("BattleResultJSON: \"{}\"", world->GetBattleResult().ToJSONObject().dump(4));
            break;
        }

        constexpr int max_loop_limit = 1000;

        // Something is wrong, infinite loop check
        if (world->GetTimeStepCount() >= max_loop_limit)
        {
            world_logger->LogErr("Simulation took more than {} steps, aborting...", max_loop_limit);
            break;
        }
    }
}

bool BattleSimulation::SpawnCombatUnit(World& world, const BattleCombatUnitState& combat_unit_state) const
{
    FullCombatUnitData full_data;
    full_data.instance = combat_unit_state.instance;
    full_data.instance.team = GridHelper::IsInBlueGridSpace(combat_unit_state.position) ? Team::kBlue : Team::kRed;
    full_data.instance.position = combat_unit_state.position;

    const auto combat_unit_data = data_loader_->GetGameDataContainer()->GetCombatUnitData(combat_unit_state.type_id);
    if (!combat_unit_data)
    {
        LogErr("SpawnUnit - Can't find unit_data for type_id = {}", combat_unit_state.type_id);
        return false;
    }
    full_data.data = *combat_unit_data;

    std::string out_error_message;
    const Entity* spawned = EntityFactory::SpawnCombatUnit(world, full_data, kInvalidEntityID, &out_error_message);
    if (!spawned)
    {
        LogErr("SpawnUnit - Failed with message: {}", out_error_message);
        return false;
    }

    return true;
}

bool BattleSimulation::SpawnDroneAugment(World& world, const DroneAugmentState& drone_augment_state) const
{
    const auto type_id = drone_augment_state.type_id;
    const auto team = drone_augment_state.team;
    if (!data_loader_->GetGameDataContainer()->HasDroneAugmentData(type_id))
    {
        LogErr("SpawnDroneAugment - Can't find drone augment data for type_id = {{{}}}", type_id);
        return false;
    }

    if (!world.AddDroneAugmentBeforeBattleStarted(team, type_id))
    {
        LogErr("SpawnDroneAugment - Failed to spawn drone augment = {{{}}}", type_id);
        return false;
    }

    return true;
}

}  // namespace simulation::tool
