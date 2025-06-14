#pragma once

#include "ecs/system.h"

namespace simulation
{

namespace event_data
{
struct BeamActivated;
}

/* -------------------------------------------------------------------------------------------------------
 * BeamSystem
 *
 * Handles the beams inside the world
 * --------------------------------------------------------------------------------------------------------
 */
class BeamSystem : public System
{
    typedef System Super;
    typedef BeamSystem Self;

    struct FoundEntityCollision
    {
        EntityID id = kInvalidEntityID;
        int distance = 0;
    };

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSpawnable;
    }

    // Determines which allegiance should block the beam if any
    bool IsBlockableByBeam(const Entity& beam, const Entity& entity) const;

    void SetUpdatedBeamLengthAndDirection(const Entity& beam, const EntityID receiver_id) const;

private:
    // Listen to world events
    void OnBeamActivated(const event_data::BeamActivated& data);
};

}  // namespace simulation
