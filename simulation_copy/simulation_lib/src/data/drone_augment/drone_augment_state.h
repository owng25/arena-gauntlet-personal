#pragma once

#include "drone_augment_data.h"
#include "utility/logger_consumer.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * DroneAugmentState
 *
 * This class holds world drone augment state
 * --------------------------------------------------------------------------------------------------------
 */
class DroneAugmentsState final : public LoggerConsumer
{
public:
    DroneAugmentsState() = default;
    explicit DroneAugmentsState(World* world);
    void ChangeWorld(World* new_world)
    {
        world_ = new_world;
    }

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;
    int GetTimeStepCount() const override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kAugment;
    }

    //
    // Own functions
    //

    bool AddDroneAugment(const Team team, const DroneAugmentData& drone_augment_data);

    bool CanAddMoreAugments(const size_t equipped_augments_size) const;

    const std::vector<DroneAugmentData>& GetDroneAugments(const Team team) const
    {
        return drone_augments_per_team_[EnumToIndex(team)];
    }

    size_t CalculateDroneAugmentCount() const
    {
        size_t count = 0;
        for (const auto& team_drone_augments : drone_augments_per_team_)
        {
            count += team_drone_augments.size();
        }
        return count;
    }

    void ResetState()
    {
        for (auto& team_drone_augments : drone_augments_per_team_)
        {
            team_drone_augments.clear();
        }
    }

private:
    static constexpr size_t kTeamCount = GetEnumEntriesCount<Team>();

    // Index: team enum
    // Value: All the drones for that team
    std::array<std::vector<DroneAugmentData>, kTeamCount> drone_augments_per_team_;

    // Owner world of this helper
    World* world_ = nullptr;
};

}  // namespace simulation
