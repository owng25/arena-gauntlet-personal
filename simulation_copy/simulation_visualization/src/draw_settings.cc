#include "draw_settings.h"

#include "draw_features.h"

static constexpr std::string_view settings_file_name = ".draw_settings.json";
static constexpr std::string_view enabled_stats_field = "EnabledStats";
static constexpr std::string_view enabled_features_field = "EnabledFeatures";
static constexpr std::string_view screen_width_field = "ScreenWidth";
static constexpr std::string_view screen_height_field = "ScreenHeight";
static constexpr std::string_view play_speed_ms_field = "PlaySpeedMs";

namespace simulation::tool
{

DrawSettings::DrawSettings()
{
    // settings have they own logger
    logger_ = Logger::Create();
    logger_->SinkAddStdout();

    const std::string custom_log_pattern = "[%=15!n] [%l] %v";
    logger_->SetLogsPattern(custom_log_pattern);

    file_helper_ = std::make_unique<FileHelper>(logger_);
    json_helper_ = std::make_unique<JSONHelper>(logger_);

    LoadSettings();
}

DrawSettings::~DrawSettings()
{
    SaveSettings();
}

void DrawSettings::SetDrawFeaturesState(const std::vector<std::unique_ptr<BaseDrawFeature>>& features)
{
    enabled_features_.clear();

    for (const auto& feature : features)
    {
        if (feature->IsEnabled())
        {
            enabled_features_.emplace(feature->GetName());
        }
    }
}

bool DrawSettings::IsFeatureEnabled(const BaseDrawFeature& feature) const
{
    return enabled_features_.count(feature.GetName()) > 0;
}

bool DrawSettings::HasFeaturesSet() const
{
    return !enabled_features_.empty();
}

void DrawSettings::LoadSettings()
{
    LoadDefaults();

    if (!FileHelper::DoesFileExist(GetSettingsFilePath()))
    {
        // Cli setting file doesn't exists, using defaults.
        return;
    }

    std::string settings_content = file_helper_->ReadAllContentFromFile(GetSettingsFilePath());
    const auto settings_json = nlohmann::json::parse(settings_content);

    enabled_stats_.clear();
    json_helper_->WalkArray(
        settings_json,
        enabled_stats_field,
        true,
        [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            StatType stat_type = StatType::kNone;
            if (!json_helper_->GetEnumValue(json_array_element, &stat_type))
            {
                logger_->LogErr("DrawSettings::LoadSettings - Failed to load enabled stats with index = {}", index);
                return false;
            }

            enabled_stats_.push_back(stat_type);
            return true;
        });

    json_helper_->GetIntValue(settings_json, screen_width_field, &screen_width_);
    json_helper_->GetIntValue(settings_json, screen_height_field, &screen_height_);
    json_helper_->GetIntValue(settings_json, play_speed_ms_field, &play_speed_ms_);

    std::vector<std::string> enabled_features_vector;
    if (!json_helper_->GetStringArray(settings_json, enabled_features_field, &enabled_features_vector))
    {
        logger_->LogErr("DrawSettings::LoadSettings - Failed to load enabled features from draw settings file");
    }

    std::copy(
        enabled_features_vector.begin(),
        enabled_features_vector.end(),
        std::inserter(enabled_features_, enabled_features_.end()));
}

void DrawSettings::LoadDefaults()
{
    enabled_stats_ = {
        StatType::kMaxHealth,
        StatType::kCurrentHealth,
        StatType::kEnergyCost,
        StatType::kCurrentEnergy,
        StatType::kOmegaPowerPercentage,
        StatType::kAttackSpeed,
        StatType::kAttackPhysicalDamage,
        StatType::kAttackEnergyDamage,
        StatType::kPhysicalResist,
        StatType::kEnergyResist,
        StatType::kCritChancePercentage,
        StatType::kCritAmplificationPercentage};
}

fs::path DrawSettings::GetSettingsFilePath()
{
    return GetExecutableDir() / settings_file_name;
}

const fs::path& DrawSettings::GetExecutableDir()
{
    return FileHelper::GetExecutableDirectory();
}

void DrawSettings::SaveSettings()
{
    nlohmann::json settings_json;

    // Stats
    nlohmann::json enabled_stats_json_array;
    for (const StatType stat : enabled_stats_)
    {
        enabled_stats_json_array.push_back(Enum::StatTypeToString(stat));
    }
    settings_json[enabled_stats_field] = enabled_stats_json_array;

    settings_json[enabled_features_field] = enabled_features_;

    // Int values
    settings_json[play_speed_ms_field] = play_speed_ms_;
    settings_json[screen_width_field] = screen_width_;
    settings_json[screen_height_field] = screen_height_;

    file_helper_->WriteContentToFile(GetSettingsFilePath(), settings_json.dump(4));
}

}  // namespace simulation::tool
