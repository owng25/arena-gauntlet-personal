#pragma once

#include "data/enums.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * DeferredDestructionComponent
 *
 * This class holds details for entities that have deferred destruction
 * --------------------------------------------------------------------------------------------------------
 */
class DeferredDestructionComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<DeferredDestructionComponent>(*this);
    }

    // Mark the entity as pending destruction
    void SetPendingDestruction(DestructionReason destruction_reason)
    {
        is_pending_destruction_ = true;
        destruction_reason_ = destruction_reason;
    }

    // Whether the entity is going to be destroyed
    bool IsPendingDestruction() const
    {
        return is_pending_destruction_;
    }

    // Get the reason for destruction
    DestructionReason GetDestructionReason() const
    {
        return destruction_reason_;
    }

private:
    bool is_pending_destruction_ = false;
    DestructionReason destruction_reason_ = DestructionReason::kNone;
};

}  // namespace simulation
