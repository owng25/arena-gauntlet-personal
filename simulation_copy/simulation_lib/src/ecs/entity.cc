#include "ecs/entity.h"

#include "ecs/event_types_data.h"
#include "ecs/world.h"

namespace simulation
{
std::shared_ptr<Entity> Entity::CreateDeepCopyFromInitialState(const std::weak_ptr<World>& owner_world) const
{
    auto new_entity = std::shared_ptr<Entity>(new Entity{*this});
    new_entity->owner_world_ = owner_world;

    // Deep copy the components
    new_entity->components_.clear();
    new_entity->component_array_.fill(nullptr);

    for (size_t i = 0; i < component_array_.size(); i++)
    {
        Component* component = component_array_[i];
        if (component == nullptr)
        {
            continue;
        }

        // Create the component and assign its owner
        auto new_component = component->CreateDeepCopyFromInitialState();
        auto* new_component_ptr = new_component.get();
        new_component_ptr->owner_entity_ = new_entity->shared_from_this();

        // Store component
        new_entity->components_.push_back(std::move(new_component));
        new_entity->component_array_[i] = new_component_ptr;

        // NOTE: Do not initialize the component
    }

    return new_entity;
}

std::shared_ptr<World> Entity::GetOwnerWorld() const
{
    return owner_world_.lock();
}

void Entity::Deactivate()
{
    is_active_ = false;

    const auto world = GetOwnerWorld();
    if (!world)
    {
        return;
    }
    world->BuildAndEmitEvent<EventType::kDeactivated>(GetID());
    world->LogDebug(GetID(), "Deactivating");
}

}  // namespace simulation
