#pragma once

#include <memory>

#include "draw_helper.h"

namespace simulation
{
class Event;
}

namespace simulation::tool
{
class BaseDrawFeature;

/* -------------------------------------------------------------------------------------------------------
 * BattleVisualization
 *
 * This class take listen to simulation world and visualization it
 * --------------------------------------------------------------------------------------------------------
 */
class BattleVisualization
{
public:
    BattleVisualization(std::shared_ptr<simulation::World> world, bool exit_when_finished, bool space_to_time_step);
    ~BattleVisualization();

protected:
    void OnTimeStep(const simulation::Event& event);
    void OnBattleStarted(const simulation::Event& event);

    void DrawWorld();
    void DrawGrid() const;
    void DrawMouseSelection() const;
    void DrawFeaturesWorldPass() const;
    void DrawFeaturesUIPass() const;
    void DrawInfo() const;
    void DrawSynergyInfo() const;

    const DrawHelper& GetDrawHelper() const
    {
        return draw_helper_;
    }

private:
    Vector2f camera_{0.f, 0.f};  // Camera position in world coordinates
    float zoom_ = .9f;
    std::shared_ptr<simulation::Logger> logger_;
    std::shared_ptr<simulation::World> world_;
    DrawHelper draw_helper_;

    // Defaults
    bool exit_when_finished_ = false;
    bool space_to_time_step_ = false;

    std::vector<std::unique_ptr<BaseDrawFeature>> draw_features_;

    // Key: Team
    // Value: Summary text of synergy numbers
    std::unordered_map<simulation::Team, std::string> teams_synergies{};
};

}  // namespace simulation::tool
