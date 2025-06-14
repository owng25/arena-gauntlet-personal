#pragma once

#include "data/constants.h"
#include "data/enums.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * StateComponent
 *
 * This class tracks the state of a combat unit
 * --------------------------------------------------------------------------------------------------------
 */
class StateComponent : public Component
{
public:
    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<StateComponent>(*this);
    }
    void Init() override {}

    // Get/set the current state
    CombatUnitState GetState() const
    {
        return state_;
    }
    void SetState(const CombatUnitState state)
    {
        state_ = state;
    }

    // Get time steps to wait between fainted and disappear
    int GetDisappearTimeSteps() const
    {
        return disappear_time_steps_;
    }
    int GetAndDecrementDisappearTimeSteps()
    {
        return disappear_time_steps_--;
    }

    // Helpers to check state
    bool IsAlive() const
    {
        return state_ == CombatUnitState::kAlive;
    }
    bool IsFainted() const
    {
        return state_ == CombatUnitState::kFainted;
    }
    bool IsDisappeared() const
    {
        return state_ == CombatUnitState::kDisappeared;
    }

private:
    // State of the entity
    CombatUnitState state_ = CombatUnitState::kAlive;

    // Time steps until fainted entity disappears
    int disappear_time_steps_ = kDisappearTimeSteps;
};

}  // namespace simulation
