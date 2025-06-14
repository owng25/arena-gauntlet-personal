#include "cli_settings.h"

#include "utility/file_helper.h"

static constexpr std::string_view cli_settings_file_name = ".cli_settings.json";
static constexpr std::string_view enable_debug_logs_field = "EnableDebugLogs";
static constexpr std::string_view log_pattern_field = "LogPattern";
static constexpr std::string_view json_data_path_field = "JsonDataPath";
static constexpr std::string_view battle_files_path_field = "BattleFilesPath";
static constexpr std::string_view profile_file_path_filed = "ProfileFilePath";

namespace simulation::tool
{
CLISettings::CLISettings()
{
    // The custom log pattern used
    // https://spdlog.docsforge.com/v1.x/3.custom-formatting/
    log_pattern_ = "[%=15!n] [%l] %v";

    // settings have they own logger
    logger_ = simulation::Logger::Create();
    logger_->SinkAddStdout();

    logger_->SetLogsPattern(log_pattern_);

    file_helper_ = std::make_unique<simulation::FileHelper>(logger_);
    json_helper_ = std::make_unique<simulation::JSONHelper>(logger_);

    LoadSettings();
}

void CLISettings::LoadSettings()
{
    if (!simulation::FileHelper::DoesFileExist(GetSettingsFilePath()))
    {
        // Cli setting file doesn't exists, using defaults.
        return;
    }

    std::string settings_content = file_helper_->ReadAllContentFromFile(GetSettingsFilePath());
    const auto settings_json = nlohmann::json::parse(settings_content);

    json_helper_->GetBoolValue(settings_json, enable_debug_logs_field, &enable_debug_logs_);
    json_helper_->GetStringValue(settings_json, log_pattern_field, &log_pattern_);
    json_helper_->GetStringValue(settings_json, json_data_path_field, &json_data_path_);
    json_helper_->GetStringValue(settings_json, battle_files_path_field, &battle_files_path_);
    json_helper_->GetStringValue(settings_json, profile_file_path_filed, &profile_file_path_);
}

fs::path CLISettings::GetSettingsFilePath()
{
    return GetExecutableDir() / cli_settings_file_name;
}

fs::path CLISettings::GetJSONDataPath() const
{
    fs::path json_data_path(json_data_path_);
    json_data_path.make_preferred();
    if (json_data_path.is_relative())
    {
        json_data_path = GetExecutableDir() / json_data_path;
    }

    return json_data_path;
}

fs::path CLISettings::GetBattleFilesPath() const
{
    fs::path battle_files_path(battle_files_path_);
    battle_files_path.make_preferred();
    if (battle_files_path.is_relative())
    {
        battle_files_path = GetExecutableDir() / battle_files_path;
    }
    return battle_files_path;
}

const fs::path& CLISettings::GetExecutableDir()
{
    return FileHelper::GetExecutableDirectory();
}

fs::path CLISettings::GetProfileFilePath() const
{
    fs::path path = profile_file_path_;
    if (path.is_relative())
    {
        path = GetExecutableDir() / profile_file_path_;
    }

    return path;
}

void CLISettings::SaveSettings()
{
    nlohmann::json settings_json;
    settings_json[enable_debug_logs_field] = enable_debug_logs_;
    settings_json[log_pattern_field] = log_pattern_;
    settings_json[json_data_path_field] = json_data_path_;
    settings_json[battle_files_path_field] = battle_files_path_;
    settings_json[profile_file_path_filed] = profile_file_path_;

    file_helper_->WriteContentToFile(GetSettingsFilePath(), settings_json.dump(4));
}
}  // namespace simulation::tool
