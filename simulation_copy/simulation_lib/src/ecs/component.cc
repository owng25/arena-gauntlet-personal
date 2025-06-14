#include "ecs/component.h"

#include <cassert>
#include <memory>

#include "data/constants.h"
#include "ecs/entity.h"
#include "ecs/world.h"

namespace simulation
{
EntityID Component::GetOwnerEntityID() const
{
    const auto entity = GetOwnerEntity();
    return entity ? entity->GetID() : kInvalidEntityID;
}

std::shared_ptr<Entity> Component::GetOwnerEntity() const
{
    // assert(!owner_entity_.expired());
    return owner_entity_.lock();
}

std::shared_ptr<World> Component::GetOwnerWorld() const
{
    const auto entity = GetOwnerEntity();
    return entity ? entity->GetOwnerWorld() : nullptr;
}

}  // namespace simulation
