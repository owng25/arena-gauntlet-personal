#pragma once

#include "components/zone_component.h"
#include "ecs/system.h"

namespace simulation
{
namespace event_data
{
struct ZoneActivated;
}

class FilteringComponent;

/* -------------------------------------------------------------------------------------------------------
 * ZoneSystem
 *
 * Handles the zones inside the world
 * --------------------------------------------------------------------------------------------------------
 */
class ZoneSystem : public System
{
    typedef System Super;
    typedef ZoneSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    void PostTimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSpawnable;
    }

    bool ShouldDestroyZone(const Entity& zone_entity) const;

private:
    void SummarizeZoneEffectTypes(
        const Entity& zone,
        const AbilityType ability_type,
        bool* out_beneficial,
        bool* out_detrimental) const;

    // Listen to world events
    void OnZoneActivated(const event_data::ZoneActivated& data);

    // Has this zone collided with this entity
    bool HasZoneCollidedWith(
        const ZoneComponent& zone_component,
        const FilteringComponent& filtering_component,
        const EntityID entity_id) const;

    // Add the entity to the set of entities this component collided with
    void AddZoneCollidedWith(
        const ZoneComponent& zone_component,
        FilteringComponent& filtering_component,
        const EntityID entity_id) const;
};

}  // namespace simulation
