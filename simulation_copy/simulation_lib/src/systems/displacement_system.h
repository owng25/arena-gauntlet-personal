#pragma once

#include "ecs/system.h"

namespace simulation
{
struct HexGridPosition;
class Event;

namespace event_data
{
struct Displacement;
}

/* -------------------------------------------------------------------------------------------------------
 * DisplacementSystem
 *
 * This class handles entity displacement
 * --------------------------------------------------------------------------------------------------------
 */
class DisplacementSystem : public System
{
    typedef System Super;
    typedef DisplacementSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kDisplacement;
    }

private:
    void OnDisplacementStopped(const event_data::Displacement& event_data);

    void MoveToward(const Entity& entity, const HexGridPosition& position, const int sub_units_to_move);
    void MoveEntity(const Entity& entity, const HexGridPosition& sub_unit_delta);
};

}  // namespace simulation
