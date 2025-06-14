#include "cli_run_test_command.h"

#include <chrono>
#include <filesystem>
#include <lyra/lyra.hpp>
#include <memory>
#include <string>

#include "battle_simulation.h"
#include "battle_test.h"
#include "cli_settings.h"
#include "ecs/world.h"
#include "utility/file_helper.h"
#include "utility/logger.h"
#include "utility/string.h"

namespace simulation::tool
{
using namespace simulation;

CLIRunTestCommand::CLIRunTestCommand(lyra::cli& cli)
{
    auto run_command = lyra::command(
        "run_test",
        [this](const lyra::group& g)
        {
            this->DoCommand(g);
        });
    run_command.help("Run all tests in specified directory.");
    run_command.add_argument(
        lyra::arg(test_files_dir_, "test_files_dir").required().help("Path to a directory with test files."));

    cli.add_argument(run_command);
}

void CLIRunTestCommand::DoCommand(const lyra::group&) const
{
    // Create logger for test results
    const auto test_logger = Logger::Create();
    test_logger->SinkAddFile("test_result.txt");
    test_logger->SinkAddStdout();

    // Shorted log pattern for test results
    test_logger->SetLogsPattern("[%T.%e] [%l] %v");

    test_logger->LogInfo("Starting running tests");

    const auto settings = std::make_shared<CLISettings>();
    std::vector<std::string> passed_tests;
    std::vector<std::string> failed_tests;

    const BattleTestRunner test_runner(settings, test_logger);

    const FileHelper& file_helper = settings->GetFileHelper();
    file_helper.WalkFilesInDirectory(
        test_files_dir_,
        [&](const fs::path& path)
        {
            if (fs::is_directory(path))
            {
                return;
            }

            const std::string test_file_name = path.stem().string();
            if (!String::StartsWith(test_file_name, "test_"))
            {
                // Skip non test files
                return;
            }

            BattleTestData test_data;
            if (!test_runner.Load(path, &test_data))
            {
                test_logger->LogErr("Failed to load test file {}", path);
                failed_tests.push_back(test_file_name);
                return;
            }

            if (test_runner.Run(test_data))
            {
                passed_tests.push_back(test_file_name);
            }
            else
            {
                failed_tests.push_back(test_file_name);
            }
        });

    test_logger->LogInfo("Finished running tests");
    test_logger->LogInfo(
        "{}: passed {}, failed {}",
        failed_tests.empty() ? "Success" : "Fail",
        passed_tests.size(),
        failed_tests.size());

    if (!failed_tests.empty())
    {
        test_logger->LogErr("Failed tests:");
        for (const auto& failed_test : failed_tests)
        {
            test_logger->LogErr("\t{}", failed_test);
        }
    }
}
}  // namespace simulation::tool
