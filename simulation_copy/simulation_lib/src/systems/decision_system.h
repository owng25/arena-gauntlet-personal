#pragma once

#include "ecs/system.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * DecisionSystem
 *
 * This class serves to implement AI decision making
 * --------------------------------------------------------------------------------------------------------
 */
class DecisionSystem : public System
{
public:
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kDecision;
    }

    // Try to determine the next action for this entity
    DecisionNextAction GetNextAction(const Entity& entity) const;

private:
    bool DoesOmegaRequireFocus(const Entity& entity) const;

    bool IsAttackReady(const Entity& entity, bool* out_move) const;
    bool IsOmegaReady(const Entity& entity) const;

    // Check whether current focus exist and in attack/omega range
    // is_omega_range is false to check attack range, is_omega_range is true to use omega range
    // allow_refocus is true to allow updating a focus using FocusSystem::FindAndUpdateTargetInRange
    bool HasFocusInRange(const Entity& entity, bool is_omega_range, bool allow_refocus) const;

    DecisionNextAction GetActionForTaunted(const Entity& entity) const;
};

}  // namespace simulation
