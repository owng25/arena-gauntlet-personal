#pragma once

#include "ecs/system.h"

namespace simulation
{
namespace event_data
{
struct Fainted;
}
/* -------------------------------------------------------------------------------------------------------
 * StateSystem
 *
 * Handles CombatUnit state changes
 * --------------------------------------------------------------------------------------------------------
 */
class StateSystem : public System
{
    typedef System Super;
    typedef StateSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;

private:
    // Listen to world events
    void OnFainted(const event_data::Fainted& fainted_data);
};

}  // namespace simulation
