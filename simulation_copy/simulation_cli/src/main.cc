#include <filesystem>
#include <iostream>
#include <lyra/lyra.hpp>

#include "cli_run_batch_command.h"
#include "cli_run_battle_command.h"
#include "cli_run_test_command.h"
#include "cli_set_settings_command.h"
#include "utility/file_helper.h"

int main(int argc, const char** argv)
{
    // Init executable directory
    simulation::FileHelper::SetExecutableDirectory(std::filesystem::path(argv[0]).parent_path());  // NOLINT
    // std::cout << "ExePath: " << simulation::FileHelper::GetExecutableDirectory() << "\n";

    // Init Lyra cli
    auto cli = lyra::cli();

    // Help flag
    bool show_help = false;
    cli.add_argument(lyra::help(show_help));

    // Setup cli commands
    simulation::tool::CLISetSettingCommand set_command{cli};
    simulation::tool::CLIRunBattleCommand run_command{cli};
    simulation::tool::CLIRunBatchCommand run_batch_command{cli};
    simulation::tool::CLIRunTestCommand run_test_command{cli};

    // Parse commands line
    const auto result = cli.parse({argc, argv});
    if (show_help || argc == 1)
    {
        std::cout << cli;
        return 0;
    }
    if (!result)
    {
        std::cerr << result.message() << "\n";
    }
    return result ? 0 : 1;
}
