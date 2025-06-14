#include "cli_run_batch_command.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <lyra/lyra.hpp>
#include <memory>

#include "battle_simulation.h"
#include "cli_settings.h"
#include "profiling/illuvium_profiling.h"
#include "utility/file_helper.h"
#include "utility/logger.h"

namespace simulation::tool
{

CLIRunBatchCommand::CLIRunBatchCommand(lyra::cli& cli)
{
    auto run_command = lyra::command(
        "run_batch",
        [this](const lyra::group& g)
        {
            this->DoCommand(g);
        });
    run_command.help("Run all battles in specified directory.");
    run_command.add_argument(
        lyra::arg(battle_files_dir_, "battle_files_dir").required().help("Path to a directory with battle files."));
    run_command.add_argument(lyra::arg(battles_results_dir_, "battles_results_dir")
                                 .required()
                                 .help("Path to a directory where to save battle results."));

    cli.add_argument(run_command);
}

void CLIRunBatchCommand::DoCommand(const lyra::group&) const
{
    const auto settings = std::make_shared<CLISettings>();
    BattleSimulation simulation(settings);

    const FileHelper& file_helper = settings->GetFileHelper();
    {
        const bool results_dir_exists = file_helper.DoesDirectoryExist(battles_results_dir_);
        if (!results_dir_exists)
        {
            fs::create_directory(fs::path(battles_results_dir_));
        }
    }

    IlluviumStartProfiling();

    file_helper.WalkFilesInDirectory(
        battle_files_dir_,
        [&](const fs::path& path)
        {
            const std::string battle_name = path.stem().string();

            fs::path battle_results_dir(battles_results_dir_);
            battle_results_dir.append(battle_name);
            fs::create_directory(battle_results_dir);

            fs::path log_file_path(battle_results_dir);
            log_file_path.append("stdout.txt");

            // Create logger for this specific battle which writes to separate file
            const auto world_logger = Logger::Create(settings->IsDebugLogsEnabled());
            world_logger->SinkAddFile(log_file_path.string());
            world_logger->SetLogsPattern(settings->GetLogPattern());

            const auto start_time = std::chrono::high_resolution_clock::now();
            const auto world = simulation.OpenBattleFile(path, 0, world_logger);

            if (world)
            {
                // Simulation starts here
                simulation.TimeStepUntilFinished(world);
                const auto end_time = std::chrono::high_resolution_clock::now();

                fs::path duration_file_path(battle_results_dir);
                duration_file_path.append("duration.json");

                std::ofstream duration_file(duration_file_path);
                fmt::format_to(
                    std::ostream_iterator<char>(duration_file),
                    "{{\"duration\": {} }}",
                    std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count());
            }
        });

    IlluviumStopProfiling(settings->GetProfileFilePath().string());
}
}  // namespace simulation::tool
