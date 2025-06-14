#include "cli_run_battle_command.h"

#include <lyra/lyra.hpp>
#include <memory>

#include "battle_simulation.h"
#include "cli_settings.h"
#include "profiling/illuvium_profiling.h"

#ifdef ENABLE_VISUALIZATION
#include "battle_visualization.h"
#endif

namespace simulation::tool
{

CLIRunBattleCommand::CLIRunBattleCommand(lyra::cli& cli)
{
    auto run_command =
        lyra::command(
            "run",
            [this](const lyra::group& g)
            {
                this->DoCommand(g);
            })
            .help("Run the given battle file.")
            .add_argument(lyra::arg(battle_file_, "battle_file").required().help("Path to battle file json to run."));

#ifdef ENABLE_VISUALIZATION
    const auto visualize_option =
        lyra::opt(visualize).name("-v").name("--visualize").optional().help("Should we visualize battle");

    run_command.add_argument(visualize_option);
#endif

    cli.add_argument(run_command);
}

void CLIRunBattleCommand::DoCommand(const lyra::group&) const
{
    const auto settings = std::make_shared<CLISettings>();
    BattleSimulation simulation(settings);

    auto world = simulation.OpenBattleFile(battle_file_);

#ifdef ENABLE_VISUALIZATION
    std::unique_ptr<BattleVisualization> visualization = nullptr;
    if (visualize)
    {
        constexpr bool exit_when_finished = true;
        constexpr bool space_to_time_step = true;
        visualization = std::make_unique<BattleVisualization>(world, exit_when_finished, space_to_time_step);
    }
#endif

    if (!world)
    {
        return;
    }

    IlluviumStartProfiling();
    simulation.TimeStepUntilFinished(world);
    IlluviumStopProfiling(settings->GetProfileFilePath().string());
}
}  // namespace simulation::tool
