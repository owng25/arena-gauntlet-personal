#pragma once

#include <memory>
#include <string>

#include "utility/file_helper.h"
#include "utility/json_helper.h"

namespace simulation
{
class Logger;
}

namespace simulation::tool
{
/* -------------------------------------------------------------------------------------------------------
 * CLISettings
 *
 * This class handles holding, loading and saving cli settings.
 * --------------------------------------------------------------------------------------------------------
 */
class CLISettings
{
public:
    CLISettings();
    ~CLISettings() = default;

    bool IsDebugLogsEnabled() const
    {
        return enable_debug_logs_;
    }

    std::string_view GetLogPattern() const
    {
        return log_pattern_;
    }

    fs::path GetJSONDataPath() const;
    fs::path GetBattleFilesPath() const;
    static const fs::path& GetExecutableDir();
    fs::path GetProfileFilePath() const;
    simulation::FileHelper& GetFileHelper() const
    {
        return *file_helper_;
    }

    void SaveSettings();

private:
    void LoadSettings();

    static fs::path GetSettingsFilePath();

private:
    std::shared_ptr<simulation::Logger> logger_;
    std::unique_ptr<simulation::FileHelper> file_helper_;
    std::unique_ptr<simulation::JSONHelper> json_helper_;

    // -------------- Settings fields ----------------

    // Are simulation debug logs enabled?
    bool enable_debug_logs_ = false;

    // Path to json data files folder
    std::string json_data_path_ = "../../../IlluviumGame/Content/LocalTestData";

    // Path to battle files folder
    std::string battle_files_path_ = "../../../IlluviumGame/Saved/Simulation/Battles";

    // Path to file which will contain profiling data
    std::string profile_file_path_ = "simulation-cli.prof";

    // Log message pattern
    std::string log_pattern_;

    // CLISetSettingCommand should have write access to settings fields
    friend class CLISetSettingCommand;
};
}  // namespace simulation::tool
