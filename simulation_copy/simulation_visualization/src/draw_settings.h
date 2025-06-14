#pragma once

#include <memory>

#include "utility/file_helper.h"
#include "utility/json_helper.h"

namespace simulation
{
class Logger;
}

namespace simulation::tool
{
class BaseDrawFeature;
/* -------------------------------------------------------------------------------------------------------
 * DrawSettings
 *
 * This class handles holding, loading and saving draw settings.
 * --------------------------------------------------------------------------------------------------------
 */
class DrawSettings
{
public:
    DrawSettings();
    ~DrawSettings();

    const std::vector<simulation::StatType>& GetEnabledStats() const
    {
        return enabled_stats_;
    }

    int GetScreenWidth() const
    {
        return screen_width_;
    }

    int GetScreenHeight() const
    {
        return screen_height_;
    }

    void SetScreenWidth(int width)
    {
        screen_width_ = width;
    }

    void SetScreenHeight(int height)
    {
        screen_height_ = height;
    }

    int GetPlaySpeedMs() const
    {
        return play_speed_ms_;
    }

    void SetDrawFeaturesState(const std::vector<std::unique_ptr<BaseDrawFeature>>& features);
    bool IsFeatureEnabled(const BaseDrawFeature& feature) const;
    bool HasFeaturesSet() const;

private:
    void LoadSettings();
    void SaveSettings();

    void LoadDefaults();

    static fs::path GetSettingsFilePath();
    static const fs::path& GetExecutableDir();

private:
    std::shared_ptr<simulation::Logger> logger_;
    std::unique_ptr<simulation::FileHelper> file_helper_;
    std::unique_ptr<simulation::JSONHelper> json_helper_;

    // -------------- Settings fields ----------------
    std::vector<simulation::StatType> enabled_stats_;

    std::unordered_set<std::string> enabled_features_;

    int screen_width_ = 1400;
    int screen_height_ = 1200;
    int play_speed_ms_ = 100;
};

}  // namespace simulation::tool
