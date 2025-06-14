#include "cli_set_settings_command.h"

#include <lyra/lyra.hpp>

#include "cli_settings.h"

namespace simulation::tool
{
CLISetSettingCommand::CLISetSettingCommand(lyra::cli& cli)
{
    auto set_command = lyra::command(
        "set",
        [this](const lyra::group&)
        {
            this->settings_.SaveSettings();
        });

    set_command.help("Set battle settings.");

    const auto log_option = lyra::opt(settings_.enable_debug_logs_, "true|false")
                                .name("--enable_debug_logs")
                                .choices("true", "false")
                                .optional()
                                .help("Should we should debug simulation logs?");
    set_command.add_argument(log_option);

    const auto json_data_path_option = lyra::opt(settings_.json_data_path_, "path")
                                           .name("--json_data_path")
                                           .optional()
                                           .help("Path to folder with json data");
    set_command.add_argument(json_data_path_option);

    const auto battle_files_path_option = lyra::opt(settings_.battle_files_path_, "path")
                                              .name("--battle_files_path")
                                              .optional()
                                              .help("Path to folder with battle files");
    set_command.add_argument(battle_files_path_option);

    const auto log_pattern_option =
        lyra::opt(settings_.log_pattern_, "spdlog pattern")
            .name("--log_pattern")
            .optional()
            .help(
                "Pattern for log messages (format can be found here: "
                "https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#customizing-format-using-set_pattern)");

    set_command.add_argument(log_pattern_option);

    const auto profile_file_path_option = lyra::opt(settings_.profile_file_path_, "path")
                                              .name("--profile_file_path")
                                              .optional()
                                              .help("Path where profiling file will be stored.");

    set_command.add_argument(profile_file_path_option);

    cli.add_argument(set_command);
}
}  // namespace simulation::tool
