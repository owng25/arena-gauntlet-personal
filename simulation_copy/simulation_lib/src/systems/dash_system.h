#pragma once

#include "ecs/event_types_data.h"
#include "ecs/system.h"

namespace simulation
{
namespace event_data
{
struct Moved;
}

struct HexGridPosition;

/* -------------------------------------------------------------------------------------------------------
 * DashSystem
 *
 * Handles dashing combat units inside the world
 * --------------------------------------------------------------------------------------------------------
 */
class DashSystem : public System
{
    typedef System Super;
    typedef DashSystem Self;

public:
    void TimeStep(const Entity&) override;
    void Init(World* world) override;

    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSpawnable;
    }

private:
    // Process dash movement
    void ProcessDashMove(const Entity& dash_entity);
    bool TryToDestroyIfInterrupted(const Entity& dash_entity);

    // Activate dash ability on given receiver once
    void ActivateDashAbilityOnEntity(const Entity& sender, EntityID receiver_id);

    // Try to find new collision that happen since last timestep
    bool FindNewCollisions(const Entity& dash_entity, std::vector<EntityID>& out_new_collision);

    // Check if entity is collided with path between two points with given radius
    bool CheckCapsuleCollision(
        const HexGridPosition& first_position,
        const HexGridPosition& second_position,
        int radius,
        EntityID other_entity_id) const;

    void OnInterrupt(const event_data::AbilityInterrupted& event);

private:
    std::unordered_set<EntityID> interrupted_dashes_{};
};

}  // namespace simulation
