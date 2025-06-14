#pragma once

#include <memory>

#include "data/constants.h"

namespace simulation
{
class Entity;
class World;

/* -------------------------------------------------------------------------------------------------------
 * Component
 *
 * This class forms the basis of a component in the ECS system.
 * --------------------------------------------------------------------------------------------------------
 */
class Component
{
    friend class Entity;

public:
    // Component destructor
    virtual ~Component() = default;

    // Component initialisation function
    virtual void Init() {}

    // Helper function to create a deep copy of this Component
    virtual std::shared_ptr<Component> CreateDeepCopyFromInitialState() const = 0;

    // Helper method to get the id the entity that this component is attached to
    EntityID GetOwnerEntityID() const;

    // Helper method to get the entity that this component is attached to
    std::shared_ptr<Entity> GetOwnerEntity() const;

    // Helper method to get the world this component entity is attached to
    std::shared_ptr<World> GetOwnerWorld() const;

protected:
    // Weak Reference to the entity that owns this component
    // weak_ptr needed because otherwise we would have a circular reference and
    // memory would never get deleted.
    std::weak_ptr<Entity> owner_entity_{};
};

}  // namespace simulation
