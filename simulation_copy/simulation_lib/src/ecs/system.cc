#include "ecs/system.h"

#include "ecs/world.h"

namespace simulation
{
void System::Init(World* world)
{
    world_ = world;
}

std::shared_ptr<Logger> System::GetLogger() const
{
    return world_->GetLogger();
}

void System::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int System::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

}  // namespace simulation
