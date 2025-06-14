#pragma once

#include <optional>

#include "battle_data_loader.h"
#include "cli_settings.h"
#include "utility/logger.h"

namespace simulation::tool
{
struct DroneAugmentState;
}

namespace simulation::tool
{

/* -------------------------------------------------------------------------------------------------------
 * BattleSimulation
 *
 * This class take care of everything to run battle file
 * --------------------------------------------------------------------------------------------------------
 */
class BattleSimulation
{
public:
    explicit BattleSimulation(std::shared_ptr<CLISettings> settings);
    explicit BattleSimulation(
        std::shared_ptr<CLISettings> settings,
        const std::shared_ptr<Logger>& data_loading_logger,
        const std::shared_ptr<Logger>& world_logger);

    std::shared_ptr<World> OpenBattleFile(
        const fs::path& file_name,
        std::optional<uint64_t> random_seed = std::optional<uint64_t>(),
        const std::shared_ptr<Logger>& custom_world_logger = nullptr);

    void TimeStepUntilFinished(const std::shared_ptr<World>& world) const;

private:
    bool SpawnCombatUnit(World& world, const BattleCombatUnitState& combat_unit_state) const;
    bool SpawnDroneAugment(World& world, const DroneAugmentState& drone_augment) const;

    template <typename... Args>
    void LogErr(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        data_loading_logger_->LogErr(fmt, std::forward<Args>(args)...);
    }

    std::shared_ptr<CLISettings> settings_;
    std::shared_ptr<Logger> data_loading_logger_;
    std::shared_ptr<Logger> world_logger_;
    std::unique_ptr<BattleDataLoader> data_loader_;
};
}  // namespace simulation::tool
