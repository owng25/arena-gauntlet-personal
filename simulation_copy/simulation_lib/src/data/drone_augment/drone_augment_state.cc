#include "drone_augment_state.h"

#include "ecs/world.h"

namespace simulation
{
DroneAugmentsState::DroneAugmentsState(World* world) : world_(world) {}

std::shared_ptr<Logger> DroneAugmentsState::GetLogger() const
{
    return world_->GetLogger();
}

void DroneAugmentsState::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int DroneAugmentsState::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

bool DroneAugmentsState::AddDroneAugment(const Team team, const DroneAugmentData& drone_augment_data)
{
    if (team == Team::kNone)
    {
        return false;
    }

    std::vector<DroneAugmentData>& team_drone_augment_data = drone_augments_per_team_[EnumToIndex(team)];
    if (!CanAddMoreAugments(team_drone_augment_data.size()))
    {
        return false;
    }

    team_drone_augment_data.push_back(drone_augment_data);
    return true;
}

bool DroneAugmentsState::CanAddMoreAugments(const size_t equipped_augments_size) const
{
    return equipped_augments_size < world_->GetAugmentsConfig().max_drone_augments;
}

}  // namespace simulation
