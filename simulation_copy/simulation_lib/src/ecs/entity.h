#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <vector>

#include "data/constants.h"
#include "data/enums.h"
#include "ecs/component.h"

namespace simulation
{
class World;

/* -------------------------------------------------------------------------------------------------------
 * Entity
 *
 * This class forms the basis of an entity in the ECS system
 * --------------------------------------------------------------------------------------------------------
 */
class Entity final : public std::enable_shared_from_this<Entity>
{
public:
    // Create a new entity
    static std::shared_ptr<Entity> Create(
        const std::weak_ptr<World>& owner_world,
        const Team team,
        const EntityID id,
        const EntityID parent_id = kInvalidEntityID)
    {
        // NOTE: can't access private constructor with make_shared
        auto entity = std::shared_ptr<Entity>(new Entity);
        entity->owner_world_ = owner_world;
        entity->team_ = team;
        entity->id_ = id;
        entity->parent_id_ = parent_id;
        entity->is_active_ = true;

        return entity;
    }

    // Helper function to create a deep copy of this Entity
    std::shared_ptr<Entity> CreateDeepCopyFromInitialState(const std::weak_ptr<World>& owner_world) const;

    // Copyable via explicit constructor but not copyable via assignment or movable
    Entity& operator=(const Entity&) = delete;

    // The move operations are implicitly disabled, but you can spell that out explicitly if you
    // want:
    Entity(Entity&&) = delete;
    Entity& operator=(Entity&&) = delete;

    // Whether the entity is active in the game
    bool IsActive() const
    {
        return is_active_;
    }

    // Activate this entity
    // For combat units this means alive
    void Activate()
    {
        is_active_ = true;
    }

    // Deactivate this entity
    // For combat units this means fainted/dead
    void Deactivate();

    // Gets the name of who owns this entity
    Team GetTeam() const
    {
        return team_;
    }

    // Is this entity an ally with the other
    bool IsAlliedWith(const Team other) const
    {
        return GetTeam() == other;
    }
    bool IsAlliedWith(const Entity& other) const
    {
        return GetTeam() == other.GetTeam();
    }

    // Update the team
    // NOTE: Use with care
    void SetTeam(const Team new_team)
    {
        team_ = new_team;
    }

    // Helper method to get the world this entity is attached to
    std::shared_ptr<World> GetOwnerWorld() const;

    // Gets the ID of this entity
    EntityID GetID() const
    {
        return id_;
    }

    // Sets the parent of this entity
    void SetParentID(const EntityID parent_id)
    {
        parent_id_ = parent_id;
    }

    // Gets the ID of this entity parent
    EntityID GetParentID() const
    {
        return parent_id_;
    }

    // Whether entity has a specific component
    template <typename T>
    bool Has() const
    {
        const size_t index = GetComponentTypeId<T>();
        return component_array_[index] != nullptr;
    }

    // Whether entity has a list of components. The order does not matter.
    template <typename T, typename V, typename... Types>
    bool Has() const
    {
        return Has<T>() && Has<V, Types...>();
    }

    // Returns a reference to specified component type
    template <typename T>
    T* GetPtr() const
    {
        const size_t index = GetComponentTypeId<T>();
        auto* component = component_array_[index];
        return static_cast<T*>(component);
    }

    // Returns a reference to specified component type
    template <typename T>
    T& Get() const
    {
        auto* component = GetPtr<T>();
        assert(component != nullptr);
        return *component;
    }

    // Add a new component to the entity
    template <typename T, typename... TArgs>
    T& Add(TArgs&&... margs)
    {
        // Create the component and assign its owner
        auto component = std::make_shared<T>(std::forward<TArgs>(margs)...);
        auto* component_ptr = component.get();
        component_ptr->owner_entity_ = shared_from_this();

        // Store component
        const size_t index = GetComponentTypeId<T>();
        component_array_[index] = component_ptr;
        components_.push_back(std::move(component));

        // Initialise the component
        component_ptr->Init();

        // Return the component
        return *component_ptr;
    }

    // Remove a component from the entity
    // NOTE: be careful that your code does not still have any reference to the component
    template <typename T>
    void Remove()
    {
        const auto id = GetComponentTypeId<T>();
        auto component_ptr = component_array_[id];
        component_array_[id] = nullptr;

        auto it = components_.begin();
        while (it != components_.end())
        {
            if (component_ptr == (*it).get())
            {
                it = components_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    // Only Create() can make a new entity
    Entity() {}

    // Only private copyable
    Entity(const Entity&) = default;

    // Returns the component type ID of specific type
    template <typename T>
    static size_t GetComponentTypeId() noexcept
    {
        static const size_t type_id = GetComponentTypeId();
        assert(type_id < kMaxComponents);
        return type_id;
    }

    // Returns a new generic component ID
    static size_t GetComponentTypeId() noexcept
    {
        static size_t last_id = 0;
        const size_t id = last_id;
        last_id++;
        return id;
    }

    // Team this entity belongs to
    Team team_ = Team::kNone;

    // Entity id, default to something invalid
    EntityID id_ = kInvalidEntityID;

    // Parent Entity id, default to something invalid
    EntityID parent_id_ = kInvalidEntityID;

    // Whether the entity is active in the game
    bool is_active_ = false;

    // List of components for the entity
    // TODO(vampy): resize and shrink_to_fit this or just convert it to an std::array
    std::vector<std::shared_ptr<Component>> components_;

    // Weak Reference to the world that owns this entity.
    // weak_ptr needed because otherwise we would have a circular reference and
    // memory would never get deleted.
    std::weak_ptr<World> owner_world_{};

    // Maximum number of component types in the game
    static constexpr size_t kMaxComponents = 64;

    // Component array for the entity
    // Key: Component::GetComponentTypeId<T>()
    // Value: Pointer to the component
    std::array<Component*, kMaxComponents> component_array_{};
};

}  // namespace simulation
