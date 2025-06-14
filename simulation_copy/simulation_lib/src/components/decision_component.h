#pragma once

#include "data/enums.h"
#include "ecs/component.h"

namespace simulation
{

/* -------------------------------------------------------------------------------------------------------
 * DecisionComponent
 *
 * This class defines the AI decision state for an entity
 * --------------------------------------------------------------------------------------------------------
 */
class DecisionComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<DecisionComponent>(*this);
    }

    DecisionNextAction GetNextAction() const
    {
        return next_action_;
    }
    void SetNextAction(const DecisionNextAction next_action)
    {
        next_action_ = next_action;
    }

    DecisionNextAction GetPreviousAction() const
    {
        return previous_action_;
    }
    void SetPreviousAction(const DecisionNextAction previous_action)
    {
        previous_action_ = previous_action;
    }

private:
    // Next action for the entity
    DecisionNextAction next_action_ = DecisionNextAction::kNone;

    // Previous action of the entity
    DecisionNextAction previous_action_ = DecisionNextAction::kNone;
};

}  // namespace simulation
