#pragma once

#include <unordered_set>

#include "ecs/component.h"

namespace simulation
{
class Entity;

/* -------------------------------------------------------------------------------------------------------
 * FocusComponent
 *
 * This is the current attack ability target. It is used to work out who to use attack abilities
 * against, and if out of attack range, where to move.
 * --------------------------------------------------------------------------------------------------------
 */
class FocusComponent : public Component
{
public:
    enum class RefocusType
    {
        // Always changes focus when more convenient focus is available
        kAlways = 0,

        // Never changes focus, once set
        kNever,

        // Changes focus whenever focus is unreachable
        // kUnreachable = 0,
    };

    enum class SelectionType
    {
        // Find the closest enemy and set them as our focus
        kClosestEnemy = 0,

        // Use a pre defined value, for projectiles this is the receiver_id
        kPredefined,
    };

    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override;
    void Init() override
    {
        refocus_type_ = RefocusType::kAlways;
        selection_type_ = SelectionType::kClosestEnemy;
        ResetFocus();
    }

    // Gets/Sets the pointer to the focus
    std::shared_ptr<Entity> GetFocus() const;
    void SetFocus(const std::shared_ptr<Entity>& focus);

    // Helper method to get the id of the focus, if any
    EntityID GetFocusID() const;

    // Reset the focus
    void ResetFocus()
    {
        previous_focus_id_ = GetFocusID();
        focus_.reset();
    }

    // Get the previous focus_id before it was reset
    EntityID GetPreviousFocusID() const
    {
        return previous_focus_id_;
    }

    void SetRefocusType(const RefocusType refocus_type)
    {
        refocus_type_ = refocus_type;
    }
    RefocusType GetRefocusType() const
    {
        return refocus_type_;
    }

    void SetSelectionType(const SelectionType selection_type)
    {
        selection_type_ = selection_type;
    }
    SelectionType GetSelectionType() const
    {
        return selection_type_;
    }

    bool IsFocusSet() const
    {
        return !focus_.expired();
    }
    bool IsFocusActive() const;

    const std::unordered_set<EntityID>& GetUnreachable() const
    {
        return unreachable_focus_;
    }
    void AddUnreachable(const EntityID entity_id)
    {
        unreachable_focus_.insert(entity_id);
    }
    void ClearUnreachable()
    {
        unreachable_focus_.clear();
    }

    // Increment the unreachable focus time steps
    void IncrementUnreachableFocusTimeSteps()
    {
        unreachable_focus_time_steps_++;
    }

    // Return the unreachable focus time steps
    int GetUnreachableFocusTimeSteps() const
    {
        return unreachable_focus_time_steps_;
    }

    // Reset the unreachable focus time steps
    void ResetUnreachableFocusTimeSteps()
    {
        unreachable_focus_time_steps_ = 0;
    }

    // Return distance-based refocus scan last time step
    int GetDistanceScanLastTimeStep() const
    {
        return distance_scan_last_time_step_;
    }

    // Update pathfinding last updated time step
    void SetDistanceScanLastTimeStep(const int value)
    {
        distance_scan_last_time_step_ = value;
    }

private:
    // Current focus entity
    // This is a weak reference because we don't manage the lifetime of it
    // The world does it
    std::weak_ptr<Entity> focus_{};

    // Keep track of the previous focus_id before it was reset
    EntityID previous_focus_id_ = kInvalidEntityID;

    RefocusType refocus_type_ = RefocusType::kAlways;
    SelectionType selection_type_ = SelectionType::kClosestEnemy;

    // Keep track of entities that could not be reached
    std::unordered_set<EntityID> unreachable_focus_;

    // How long has focus been unreachable
    int unreachable_focus_time_steps_ = 0;

    // Last distance-based refocus scan time step
    int distance_scan_last_time_step_ = -1;
};

}  // namespace simulation
