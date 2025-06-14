#include "systems/splash_system.h"

#include "components/splash_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"

namespace simulation
{
void SplashSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kZoneCreated>(this, &Self::OnZoneCreated);
}

void SplashSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }
}

void SplashSystem::OnZoneCreated(const event_data::ZoneCreated& data)
{
    const EntityID sender_id = data.sender_id;

    // Only care about if the sender_id is a splash
    if (!EntityHelper::IsASplash(*world_, sender_id))
    {
        return;
    }

    LogDebug(sender_id, "SplashSystem::OnZoneCreated Is a splash");

    // Mark splash as destroyed
    LogDebug(sender_id, "SplashSystem::OnZoneCreated Destroying splash");
    world_->BuildAndEmitEvent<EventType::kMarkSplashAsDestroyed>(sender_id);
}
}  // namespace simulation
