#pragma once

#include <deque>
#include <list>
#include <set>
#include <stack>

#include "data/constants.h"
#include "ecs/component.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * MovementComponent
 *
 * This class defines the movement behaviour for an entity
 * --------------------------------------------------------------------------------------------------------
 */
class MovementComponent : public Component
{
public:
    enum class MovementType
    {
        // Direct movement to target (follow it).
        // For projectiles this means that homing is enabled.
        kDirect = 0,

        // Direct movement to a position
        // For projectiles this means that homing is not enabled
        kDirectPosition,

        // Direct movement following a vector
        // Continues moving in one direction
        kDirectVector,

        // Intelligent movement, such as a Combat Unit
        kNavigation,

        // Exactly repeats specified entity position
        // (used for attached zones)
        kSnap
    };

    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<MovementComponent>(*this);
    }
    void Init() override
    {
        movement_history_.clear();
        visualisation_waypoints_.clear();
    }

    MovementType GetMovementType() const
    {
        return movement_type_;
    }
    void SetMovementType(const MovementType movement_type)
    {
        movement_type_ = movement_type;
    }

    // Gets the movement speed in subunits
    FixedPoint GetMovementSpeedSubUnits() const
    {
        return movement_speed_sub_units_;
    }
    void SetMovementSpeedSubUnits(const FixedPoint movement_speed_sub_units)
    {
        movement_speed_sub_units_ = movement_speed_sub_units;
    }

    // Gets/sets the movement target position
    const HexGridPosition& GetMovementTargetPosition() const
    {
        return movement_target_position_;
    }
    void SetMovementTargetPosition(const HexGridPosition& movement_target_position)
    {
        movement_target_position_ = movement_target_position;
    }

    // Gets/sets the movement target vector
    const HexGridPosition& GetMovementTargetVector() const
    {
        return movement_target_vector_;
    }
    void SetMovementTargetVector(const HexGridPosition& movement_target_vector)
    {
        movement_target_vector_ = movement_target_vector;
    }

    // Gets or increments/decrements movement paused counter
    int IsMovementPaused() const
    {
        return movement_paused_ > 0;
    }
    void IncrementMovementPaused()
    {
        movement_paused_++;
    }
    void DecrementMovementPaused()
    {
        movement_paused_--;
    }

    // Add movement to history
    void AddToMovementHistory(const HexGridPosition& position)
    {
        movement_history_.push_back(position);

        // Only keep last 5 moves
        if (movement_history_.size() > 5)
        {
            movement_history_.pop_front();
        }
    }

    // Clear the movement history
    void ClearMovementHistory()
    {
        movement_history_.clear();
    }

    // Check for repetitive movement
    bool IsMovementRepeated() const
    {
        const std::set<HexGridPosition> positions(movement_history_.begin(), movement_history_.end());
        if (movement_history_.size() != positions.size())
        {
            return true;
        }

        return false;
    }

    // Whether there are waypoints available for navigation
    bool HasWaypoints() const
    {
        return !waypoints_.empty();
    }

    // Returns the first waypoint
    const HexGridPosition& TopWaypoint() const
    {
        return waypoints_.top();
    }

    // Removes the first waypoint
    void PopWaypoint()
    {
        if (!waypoints_.empty())
        {
            waypoints_.pop();
            visualisation_waypoints_.pop_front();
        }
    }

    // Get waypoints for visualisation algorithms
    const std::list<HexGridPosition>& GetVisualisationWaypoints() const
    {
        return visualisation_waypoints_;
    }

    // Add a waypoint to the movement component
    void AddWaypoint(const HexGridPosition& position)
    {
        waypoints_.push(position);
        visualisation_waypoints_.push_front(position);
    }

    // Clear all waypoints
    void ClearWaypoints()
    {
        if (!waypoints_.empty())
        {
            std::stack<HexGridPosition> empty;
            std::swap(waypoints_, empty);
        }
        visualisation_waypoints_.clear();
    }

    // Return pathfinding last updated time step
    int GetPathfindingLastUpdatedTimeStep() const
    {
        return pathfinding_last_updated_time_step_;
    }

    // Update pathfinding last updated time step
    void SetPathfindingLastUpdatedTimeStep(const int value)
    {
        pathfinding_last_updated_time_step_ = value;
    }

    void SetSnapEntityID(const EntityID& entity_id)
    {
        snap_entity_id_ = entity_id;
    }

    const EntityID& GetSnapEntityID() const
    {
        return snap_entity_id_;
    }

private:
    // Type of movement for entity
    MovementType movement_type_ = MovementType::kNavigation;

    // Movement speed for entity, in subunits per time step
    FixedPoint movement_speed_sub_units_ = 2_fp * FixedPoint::FromInt(kSubUnitsPerUnit);  // Sane default

    // Movement target position, for kDirectPosition movement
    HexGridPosition movement_target_position_ = kInvalidHexHexGridPosition;

    // Movement target vector, for kDirectVector movement
    HexGridPosition movement_target_vector_ = kInvalidHexHexGridPosition;

    // Movement history - to detect entities on a repetitive path
    std::deque<HexGridPosition> movement_history_;

    // Movement is temporarily paused - integer so it can stack
    int movement_paused_ = 0;

    // Reusable waypoint queues
    std::stack<HexGridPosition> waypoints_;
    std::list<HexGridPosition> visualisation_waypoints_;

    // Pathfinding last updated value
    int pathfinding_last_updated_time_step_ = -1;

    // Entity id to snap to. Valid only if movement type is kSnap
    EntityID snap_entity_id_ = kInvalidEntityID;
};

}  // namespace simulation
