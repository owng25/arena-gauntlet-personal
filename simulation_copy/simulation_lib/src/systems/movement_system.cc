#include "systems/movement_system.h"

#include <algorithm>
#include <queue>

#include "components/dash_component.h"
#include "components/decision_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/zone_component.h"
#include "data/constants.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "systems/decision_system.h"
#include "systems/focus_system.h"
#include "utility/entity_helper.h"
#include "utility/grid_helper.h"
#include "utility/math.h"

namespace simulation
{
MovementSystem::MovementSystem()
    : pathfinding_graph_comparer(&pathfinding_graph),
      pathfinding_queue(pathfinding_graph_comparer)
{
}

void MovementSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kFocusFound>(this, &Self::OnFocusFound);
}

void MovementSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not movable
    if (!EntityHelper::IsMovable(entity))
    {
        return;
    }

    // If entity has AI, check decision
    if (entity.Has<DecisionComponent>())
    {
        if (!ShouldMoveWithoutDecision(entity))
        {
            auto& decision_component = entity.Get<DecisionComponent>();
            if (decision_component.GetNextAction() != DecisionNextAction::kMovement)
            {
                // AI decided not to move
                return;
            }
            decision_component.SetPreviousAction(DecisionNextAction::kMovement);
        }
    }

    // Get movement component
    auto& movement_component = entity.Get<MovementComponent>();

    // Make sure movement is not paused
    if (movement_component.IsMovementPaused())
    {
        return;
    }

    // Whether we are navigating or going direct to position/vector
    bool navigating = false;
    bool direct_position = false;
    bool direct_vector = false;
    bool instant_move = false;
    switch (movement_component.GetMovementType())
    {
    case MovementComponent::MovementType::kNavigation:
        navigating = true;
        break;
    case MovementComponent::MovementType::kDirectPosition:
        direct_position = true;
        break;
    case MovementComponent::MovementType::kDirectVector:
        direct_vector = true;
        break;
    case MovementComponent::MovementType::kSnap:
        instant_move = true;
        break;
    default:
        break;
    }

    // Ensure focus is set if needed
    auto& focus_component = entity.Get<FocusComponent>();
    if (!(direct_position || direct_vector || instant_move) && !focus_component.IsFocusActive() &&
        !EntityHelper::IsFleeing(entity))
    {
        LogDebug(
            entity.GetID(),
            "MovementSystem::TimeStep - focus = {} is NOT active, won't move",
            focus_component.GetFocusID());
        return;
    }
    LogDebug(entity.GetID(), "MovementSystem::TimeStep");

    // Movement speed, in units per time step
    const FixedPoint movement_speed_sub_units = movement_component.GetMovementSpeedSubUnits();

    // Check if the value is valid
    if (movement_speed_sub_units == 0_fp)
    {
        LogDebug(entity.GetID(), "MovementSystem::TimeStep - movement_speed_units = 0, CAN'T MOVE.");
        return;
    }

    // Movement waypoints
    HexGridPosition waypoint = kInvalidHexHexGridPosition;

    // Store destination of movement
    HexGridPosition destination_position = kInvalidHexHexGridPosition;

    // Whether the focus is deemed unreachable
    bool unreachable_focus = false;

    int distance_moved_sub_units = 0;

    // Get the source position component
    const GridHelper& grid_helper = world_->GetGridHelper();
    PositionComponent* position_component = grid_helper.GetSourcePositionComponent(entity);

    // Bail out on nullptr
    if (position_component == nullptr)
    {
        return;
    }

    // Find waypoints
    if (navigating)
    {
        // Build obstacle map for pathfinding and collision detection
        GridHelper::BuildObstaclesParameters build_parameters;
        build_parameters.mark_borders_as_obstacles = true;
        build_parameters.source = &entity;
        grid_helper.BuildObstacles(build_parameters);

        if (!ShouldSkipFindingWaypoint(entity))
        {
            if (EntityHelper::IsFleeing(entity))
            {
                FindFleeingWaypoints(entity, *position_component, &destination_position, &unreachable_focus);
            }
            else
            {
                FindNavigatingWaypoints(entity, *position_component, &destination_position, &unreachable_focus);
            }
        }
        else
        {
            // NOTE: Here we are trying to reach closest entity if it's in unreachable list.
            // We are adding entities to this list once we can't build a path to it.
            // But it can become reachable in any next step.
            // So while we have a valid path and moving to the current focus,
            // we are trying to build a path to closest entity if it's in unreachable list.
            // This allows retargeting to much closer units if they were unreachable before.

            const auto& unreachable = focus_component.GetUnreachable();
            if (!unreachable.empty() &&
                focus_component.GetSelectionType() == FocusComponent::SelectionType::kClosestEnemy)
            {
                constexpr bool include_unreachable = true;
                const EntityID current_closest = grid_helper.FindClosest(entity, {entity.GetID()}, include_unreachable);
                if (unreachable.count(current_closest))
                {
                    // Try to reach closest entity if it's in unreachable list
                    // TODO(shchavinskyi) we may optimize it further and limit our tries to reach target
                    if (TryToReachTarget(entity, current_closest))
                    {
                        focus_component.ClearUnreachable();
                    }
                }
            }
        }

        // If it's not unreachable, but it has no waypoints, something has gone wrong
        if (!unreachable_focus && !movement_component.HasWaypoints())
        {
            LogWarn(entity.GetID(), "MovementSystem::TimeStep - no valid waypoint, not moving");
            return;
        }
    }
    else if (instant_move)
    {
        const EntityID snap_entity_id = movement_component.GetSnapEntityID();
        if (world_->HasEntity(snap_entity_id))
        {
            const Entity& snap_entity = world_->GetByID(snap_entity_id);
            const HexGridPosition source_position_sub_unit = position_component->ToSubUnits();
            const HexGridPosition destination_position_sub_unit = snap_entity.Get<PositionComponent>().ToSubUnits();
            const HexGridPosition sub_unit_delta = destination_position_sub_unit - source_position_sub_unit;
            MoveEntity(entity, *position_component, sub_unit_delta);
            distance_moved_sub_units = ComputeDistanceMoved(sub_unit_delta);
        }
    }
    else
    {
        // Not navigating aka not using pathfinding
        if (direct_position)
        {
            destination_position = movement_component.GetMovementTargetPosition();
        }
        else if (direct_vector)
        {
            destination_position = movement_component.GetMovementTargetVector();
        }
        else
        {
            // At this point, we have already confirmed that there is a valid focus and position

            // Get destination
            const auto& destination_entity = *focus_component.GetFocus();
            const auto& destination_position_component = destination_entity.Get<PositionComponent>();
            destination_position = destination_position_component.GetPosition();
        }

        if (destination_position == kInvalidHexHexGridPosition)
        {
            LogWarn(
                entity.GetID(),
                "MovementSystem::TimeStep - Direct navigation does not have a valid target "
                "position");
            return;
        }

        // Direct movement
        waypoint = destination_position;
    }

    if (!instant_move)
    {
        // Move the actual entity with the movement speed
        distance_moved_sub_units = TimeStepMoveEntity(entity, navigating, *position_component, waypoint, direct_vector);
    }

    // Only emit a move event if the entity actually moved
    if (distance_moved_sub_units > 0)
    {
        // Add to history
        movement_component.AddToMovementHistory(position_component->GetPosition());

        // Emit final Moved event
        // Waypoints are backwards from target
        world_->BuildAndEmitEvent<EventType::kMoved>(
            entity.GetID(),
            position_component->GetPosition(),
            movement_component.GetVisualisationWaypoints());
    }
    else
    {
        // Log the condition
        LogDebug(entity.GetID(), "MovementSystem::TimeStep - entity did not move");
    }

    if (navigating)
    {
        if (movement_component.IsMovementRepeated())
        {
            // Log the condition
            LogDebug(entity.GetID(), "MovementSystem::TimeStep - entity movement is repetitive");
        }

        NavigatingCheckFocus(entity, unreachable_focus);
    }
}

int MovementSystem::TimeStepMoveEntity(
    const Entity& entity,
    const bool navigating,
    PositionComponent& position_component,
    HexGridPosition& waypoint,
    const bool waypoint_is_vector)
{
    static constexpr std::string_view method_name = "MovementSystem::TimeStepMoveEntity";

    auto& movement_component = entity.Get<MovementComponent>();

    // Movement speed, in units per time step
    const int movement_speed_sub_units = movement_component.GetMovementSpeedSubUnits().AsInt();

    // Whether to settle the CombatUnit at current location
    bool settle_now = false;

    // Distance moved per one iteration, in subunits
    int distance_moved_sub_units_iteration = 0;

    // The remaining distance an entity needs to move (in sub units)
    int distance_remainder_sub_units = movement_speed_sub_units;

    // Move until distance moved matches movement speed or no further movement
    // This loops through all the waypoints until movement speed is consumed
    do  // NOLINT
    {
        // Get the next waypoint if we're navigating
        if (navigating)
        {
            // Get the next waypoint
            if (!movement_component.HasWaypoints())
            {
                LogDebug(entity.GetID(), "{} - no more waypoints", method_name);

                settle_now = true;
                break;
            }

            waypoint = movement_component.TopWaypoint();

            LogDebug(entity.GetID(), "{} - waypoint {}", method_name, waypoint);
        }

        // Move in direction of waypoint
        if (waypoint_is_vector)
        {
            // Just apply movement using the vector
            const HexGridPosition sub_unit_delta = (waypoint * movement_speed_sub_units) / kPrecisionFactor;
            MoveEntity(entity, position_component, sub_unit_delta);
            distance_moved_sub_units_iteration = ComputeDistanceMoved(sub_unit_delta);
        }
        else
        {
            // Move towards the position
            distance_moved_sub_units_iteration =
                MoveToward(entity, position_component, waypoint, distance_remainder_sub_units);
        }

        distance_remainder_sub_units -= distance_moved_sub_units_iteration;

        // Entity could have hit something else and could have been deactivated
        if (!entity.IsActive())
        {
            LogDebug(entity.GetID(), "{} - entity no longer active, stopping trying to move", method_name);
            break;
        }

        // Check if waypoint reached
        const HexGridPosition& new_position = position_component.GetPosition();
        if (new_position == waypoint)
        {
            if (!navigating)
            {
                break;
            }

            movement_component.PopWaypoint();
            if (!movement_component.HasWaypoints())
            {
                break;
            }
        }

    } while (distance_moved_sub_units_iteration > 0 && distance_remainder_sub_units > 0 && !waypoint_is_vector);

    // Check if we are done moving
    // Settle position in center of grid hex at end of time step if the the next action
    // in the future is not move
    if (entity.Has<DecisionComponent>() &&
        world_->GetSystem<DecisionSystem>().GetNextAction(entity) != DecisionNextAction::kMovement)
    {
        settle_now = true;
    }

    // Settle entity in center of unit
    if (settle_now)
    {
        // Settle at location, this sets the subunit to be the center
        position_component.SetPosition(position_component.GetPosition());
        // assert(position_component.GetPosition().IsInMapRectangleLimits());
        LogDebug(
            entity.GetID(),
            "{} - set location to center of position = {}, subunit = {}",
            method_name,
            position_component.GetPosition(),
            position_component.GetSubUnitPosition());
    }

    return movement_speed_sub_units - distance_remainder_sub_units;
}

void MovementSystem::NavigatingCheckFocus(const Entity& entity, const bool unreachable_focus)
{
    auto& focus_component = entity.Get<FocusComponent>();

    if (unreachable_focus)
    {
        // Increment the unreachable counter
        focus_component.IncrementUnreachableFocusTimeSteps();

        // Subtract 1 from limit because we find new focus in next time step
        if (focus_component.GetUnreachableFocusTimeSteps() >= world_->GetUnreachableFocusTimeStepLimit() - 1)
        {
            const auto& focus_system = world_->GetSystem<FocusSystem>();
            bool has_other_potential_focus = focus_system.HasOtherPotentialFocus(entity);

            // Try again if we've exhausted all units
            if (!has_other_potential_focus)
            {
                focus_component.ClearUnreachable();
                has_other_potential_focus = focus_system.HasOtherPotentialFocus(entity);
            }

            // Only proceed if there's an alternative
            if (has_other_potential_focus)
            {
                // Consider the entity unreachable
                world_->BuildAndEmitEvent<EventType::kFocusUnreachable>(entity.GetID(), focus_component.GetFocusID());
            }
        }
    }
}

void MovementSystem::FindNavigatingWaypoints(
    const Entity& entity,
    const PositionComponent& position_component,
    HexGridPosition* out_destination_position,
    bool* out_unreachable_focus) const
{
    static constexpr std::string_view method_name = "MovementSystem::FindNavigatingWaypoints";
    const auto& focus_component = entity.Get<FocusComponent>();

    // Get destination
    const auto& destination = *focus_component.GetFocus();

    // Get destination position
    const auto& destination_position_component = destination.Get<PositionComponent>();

    // How far we need to be from the target
    const int sum_radius_units = position_component.GetRadius() + destination_position_component.GetRadius();

    // Get range of entity (cannot be less than 1 for position check)
    const StatsData stats_data = world_->GetLiveStats(entity);

    // Add 1 because range=0 means right next to each other.
    // See: PositionComponent::IsInRange
    int reach_range = sum_radius_units + 1;

    // Charmed units should go to the entity, so attack range is excluded for them
    if (!EntityHelper::IsCharmed(entity))
    {
        reach_range += stats_data.Get(StatType::kAttackRangeUnits).AsInt();
    }

    LogDebug(entity.GetID(), "{} - finding path to target {}", method_name, *out_destination_position);

    if (!FindPath(entity, destination_position_component.GetPosition(), reach_range, out_destination_position))
    {
        LogDebug(entity.GetID(), "{} - could not find path to target", method_name);
        *out_unreachable_focus = true;
    }
}

void MovementSystem::FindFleeingWaypoints(
    const Entity& entity,
    const PositionComponent& position_component,
    HexGridPosition* out_destination_position,
    bool* out_unreachable_focus) const
{
    static constexpr std::string_view method_name = "MovementSystem::TimeStep";
    const auto& movement_component = entity.Get<MovementComponent>();

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    const auto& flee_negative_states = attached_effects_component.GetNegativeStates().at(EffectNegativeState::kFlee);
    const auto& last_flee_state = flee_negative_states.back();

    // Sender is not valid for fleeing from it
    if (!world_->HasEntity(last_flee_state->sender_id))
    {
        LogDebug(entity.GetID(), "{} - Flee sender is not valid", method_name);
        *out_unreachable_focus = true;
        return;
    }

    const auto& sender_entity = world_->GetByID(last_flee_state->sender_id);
    const auto& sender_position_component = sender_entity.Get<PositionComponent>();
    const auto& sender_position = sender_position_component.GetPosition();

    // Use move speed for a radius where we can reach
    const int range_units =
        std::max(1, Math::FractionalCeil(movement_component.GetMovementSpeedSubUnits().AsInt(), kSubUnitsPerUnit));

    // NOTE: We are not limiting any direction explicitly, but moving toward sender_position is limited by distance
    // filter. Any position in half circle closer to sender will lose a distance filter to current position.

    // Find a farthest position from sender where we can reach in one time step
    const GridHelper& grid_helper = world_->GetGridHelper();
    const HexGridPosition farthest_position = grid_helper.GetFarthestOpenPositionFromPointInRange(
        position_component.GetPosition(),
        sender_position,
        range_units);

    LogDebug(entity.GetID(), "{} - flee to target = {}", method_name, farthest_position);

    // Can't find any
    if (farthest_position == kInvalidHexHexGridPosition)
    {
        // Can't get there
        LogDebug(entity.GetID(), "{} - could not find open position", method_name);
        *out_unreachable_focus = true;
        return;
    }

    // Find a path to the destination
    LogDebug(entity.GetID(), "{} - finding path to target {}", method_name, *out_destination_position);

    if (position_component.GetPosition() == farthest_position)
    {
        LogWarn(entity.GetID(), "{} - no need to move", method_name);
        return;
    }

    if (!FindPath(entity, farthest_position, position_component.GetRadius(), out_destination_position))
    {
        LogDebug(entity.GetID(), "{} - could not find path", method_name);
        *out_unreachable_focus = true;
    }
}

bool MovementSystem::ShouldSkipFindingWaypoint(const Entity& entity) const
{
    const auto& movement_component = entity.Get<MovementComponent>();

    // We should not skip finding path when we don't have any waypoints
    if (!movement_component.HasWaypoints())
    {
        return false;
    }

    // At least find a path once every 18 time steps (kPathfindingUpdateMaxTimeSteps)
    // This is a multiple of 3 to align with the prediction in the visualisation
    const int current_time_step = world_->GetTimeStepCount();
    if (current_time_step > movement_component.GetPathfindingLastUpdatedTimeStep() + kPathfindingUpdateMaxTimeSteps)
    {
        return false;
    }

    const auto& position_component = entity.Get<PositionComponent>();

    // We should not skip path finding if an obstacle occured
    const auto& top_waypoint = movement_component.TopWaypoint();
    return !IsThereObstacleBetween(top_waypoint, position_component.GetPosition());
}

bool MovementSystem::FindPath(
    const Entity& source,
    const HexGridPosition& target_position,
    const int reach_radius,
    HexGridPosition* out_reach_position) const
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Get movement component
    auto& movement_component = source.Get<MovementComponent>();
    const auto& position_component = source.Get<PositionComponent>();
    const int source_radius = position_component.GetRadius();
    const HexGridPosition& source_position = position_component.GetPosition();

    // Validate input parameters
    if (!world_->GetGridConfig().IsInMapRectangleLimits(source_position))
    {
        assert(false);
        return false;
    }
    if (!world_->GetGridConfig().IsInMapRectangleLimits(target_position))
    {
        assert(false);
        return false;
    }

    // Record the last time this entity was updated
    movement_component.SetPathfindingLastUpdatedTimeStep(world_->GetTimeStepCount());

    // Clear the waypoints
    movement_component.ClearWaypoints();

    // Reset A* graph
    ResetPathfindingGraph();

    // Create A* queue
    // AStarGraphNodesQueue find_path_untested_nodes(pathfinding_graph_comparer);

    // Emit pathfinding event for the sender
    world_->BuildAndEmitEvent<EventType::kFindPath>(source.GetID(), target_position);

    // Proceed with A* path finding
    // Setup node indices for path finding
    const HexGridConfig& grid_config = world_->GetGridConfig();
    const GridHelper& grid_helper = world_->GetGridHelper();
    const size_t start_node = grid_config.GetGridIndex(source_position);
    const size_t target_node = grid_config.GetGridIndex(target_position);

    if (start_node == target_node)
    {
        assert(false);
        return false;
    }

    size_t found_node = kInvalidIndex;
    const bool found_path = grid_helper.FindPathOnGrid(
        pathfinding_graph,
        pathfinding_queue,
        source_position,
        source_radius,
        target_position,
        reach_radius,
        world_->GetPathfindingIterationLimit(),
        &found_node);

    // End is unreachable
    if (!found_path)
    {
        return false;
    }

    *out_reach_position = grid_config.GetCoordinates(found_node);

    AddWaypointsToFromFindPathToMovementComponent(&movement_component, pathfinding_graph, start_node, found_node);

    // Found a path
    return true;
}

bool MovementSystem::TryToReachTarget(const Entity& source, const EntityID target_id) const
{
    ILLUVIUM_PROFILE_FUNCTION();

    const auto& source_position_component = source.Get<PositionComponent>();
    const auto& source_position = source.Get<PositionComponent>().GetPosition();

    const int source_radius = source.Get<PositionComponent>().GetRadius();

    const Entity& target_entity = world_->GetByID(target_id);
    const auto& target_position_component = target_entity.Get<PositionComponent>();

    // How far we need to be from the target
    const int sum_radius_units = source_position_component.GetRadius() + target_position_component.GetRadius();

    // Add 1 because range=0 means right next to each other.
    // See: PositionComponent::IsInRange
    const int reach_units = sum_radius_units + 1;

    // Try to reach to only one open position
    const GridHelper& grid_helper = world_->GetGridHelper();

    const HexGridPosition& target_position = target_position_component.GetPosition();

    ResetPathfindingGraph();
    // AStarGraphNodesQueue untested_nodes(pathfinding_graph_comparer);

    // Limit unreachable check
    const int max_iterations = world_->GetPathfindingIterationLimit();
    size_t reached_node = kInvalidIndex;
    return grid_helper.FindPathOnGrid(
        pathfinding_graph,
        pathfinding_queue,
        source_position,
        source_radius,
        target_position,
        reach_units,
        max_iterations,
        &reached_node);
}

void MovementSystem::AddWaypointsToFromFindPathToMovementComponent(
    MovementComponent* movement_component,
    AStarGraph& find_path_graph_nodes,
    const size_t start_node,
    const size_t end_node) const
{
    // Make sure that start node works as a stop point
    find_path_graph_nodes[start_node].parent_index = kInvalidIndex;

    // Remove intermediary nodes if there are no obstacles
    size_t obstacle_current_node = end_node;
    size_t parent_parent_node = find_path_graph_nodes[obstacle_current_node].parent_index;
    if (parent_parent_node != kInvalidIndex)
    {
        parent_parent_node = find_path_graph_nodes[parent_parent_node].parent_index;
    }

    const HexGridConfig& grid_config = world_->GetGridConfig();

    while (parent_parent_node != kInvalidIndex)
    {
        // Grid coordinates from indices
        const HexGridPosition current_position = grid_config.GetCoordinates(obstacle_current_node);

        // Grid parent coordinates from indices
        const HexGridPosition parent_parent_position = grid_config.GetCoordinates(parent_parent_node);

        // No obstacle between the two points, so discard the middle node
        if (!IsThereObstacleBetween(parent_parent_position, current_position))
        {
            // We don't bother updating the goal values because they aren't used
            // later in the function.
            find_path_graph_nodes[obstacle_current_node].parent_index = parent_parent_node;
        }
        // There's an obstacle, so keep the middle node
        else
        {
            obstacle_current_node = find_path_graph_nodes[obstacle_current_node].parent_index;
        }

        parent_parent_node = find_path_graph_nodes[parent_parent_node].parent_index;
    }

    AStarGraphNode& path = find_path_graph_nodes[end_node];

    // Grid coordinates from indices
    HexGridPosition current_position = grid_config.GetCoordinates(end_node);

    // Record end position
    movement_component->AddWaypoint(current_position);

    // Traverse the nodes
    while (path.parent_index != kInvalidIndex && path.parent_index != start_node)
    {
        // Update indices
        current_position = grid_config.GetCoordinates(path.parent_index);

        // Update position
        movement_component->AddWaypoint(current_position);

        // Follow parent
        path = find_path_graph_nodes[path.parent_index];
    }
}

bool MovementSystem::IsThereObstacleBetween(const HexGridPosition& position1, const HexGridPosition& position2) const
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Cube distance derived from Manhattan
    const HexGridPosition position_diff = position2 - position1;
    const int distance = position_diff.Length();

    // Iterate over the distance
    const GridHelper& grid_helper = world_->GetGridHelper();
    for (int i = 0; i <= distance; i++)
    {
        // Scale coordinates by the distance
        const int multiplier = Math::UnitsToSubUnits(i) / distance;
        const int q_scaled = Math::UnitsToSubUnits(position1.q) + position_diff.q * multiplier;
        const int r_scaled = Math::UnitsToSubUnits(position1.r) + position_diff.r * multiplier;

        // Round calculated position to the nearest hex
        HexGridPosition test_unit_position;
        HexGridPosition test_subunit_position;
        HexGridPosition::Round(HexGridPosition{q_scaled, r_scaled}, &test_unit_position, &test_subunit_position);

        // Check if position clear
        if (grid_helper.HasObstacleAt(test_unit_position))
        {
            return true;
        }
    }

    // No obstacles
    return false;
}

int MovementSystem::MoveToward(
    const Entity& entity,
    PositionComponent& position_component,
    const HexGridPosition& end_position,
    const int sub_units_to_move)
{
    ILLUVIUM_PROFILE_FUNCTION();

    assert(sub_units_to_move > 0);

    // Calculate vectors
    const HexGridPosition start = position_component.ToSubUnits();
    const HexGridPosition end = end_position.ToSubUnits();
    const HexGridPosition destination_vector = end - start;

    // Calculate direction
    const HexGridPosition destination_vector_normal = destination_vector.ToNormalizedAndScaled();

    // Calculate destination
    HexGridPosition movement_vector = (destination_vector_normal * sub_units_to_move) / kPrecisionFactor;

    // Don't move past the destination location
    if (movement_vector.Length() > destination_vector.Length())
    {
        movement_vector = destination_vector;
    }

    // Apply movement vector to entity
    MoveEntity(entity, position_component, movement_vector);

    const int distance_moved = ComputeDistanceMoved(movement_vector);

    return distance_moved;
}

void MovementSystem::MoveEntity(
    const Entity& entity,
    PositionComponent& position_component,
    const HexGridPosition& sub_unit_delta)
{
    ILLUVIUM_PROFILE_FUNCTION();

    if (sub_unit_delta.IsNull())
    {
        return;
    }

    // Get movement component
    auto& movement_component = entity.Get<MovementComponent>();

    // Update position
    HexGridPosition unit_position = position_component.GetPosition();
    HexGridPosition sub_unit_position = position_component.GetSubUnitPosition();
    GridHelper::AddSubUnitsPositionToPosition(sub_unit_delta, &unit_position, &sub_unit_position);

    LogDebug(
        entity.GetID(),
        "MovementSystem::MoveEntity - current position = {} subunit = {}",
        position_component.GetPosition(),
        position_component.GetSubUnitPosition());

    // Check if position clear
    if (movement_component.GetMovementType() == MovementComponent::MovementType::kNavigation)
    {
        const GridHelper& grid_helper = world_->GetGridHelper();
        if (grid_helper.HasObstacleAt(unit_position))
        {
            LogDebug(
                entity.GetID(),
                "- Can't move to position = {}, subunit = {}, try alternative rounding",
                unit_position,
                sub_unit_position);

            unit_position = position_component.GetPosition();
            sub_unit_position = position_component.GetSubUnitPosition();
            GridHelper::AddSubUnitsPositionToPositionWithAlternativeRounding(
                sub_unit_delta,
                &unit_position,
                &sub_unit_position);

            if (grid_helper.HasObstacleAt(unit_position))
            {
                LogDebug(
                    entity.GetID(),
                    "- Can't move to position = {}, subunit = {}, need to find new path",
                    unit_position,
                    sub_unit_position);

                // Discard path
                movement_component.ClearWaypoints();

                // Reset subunits
                position_component.SetPosition(position_component.GetPosition());

                return;
            }
        }
    }

    // Update position component
    position_component.SetPosition(unit_position, sub_unit_position);

    // Emit step Moved event
    assert(world_ != nullptr);
    LogDebug(
        entity.GetID(),
        "- Move Event to position = {}, subunit = {}",
        position_component.GetPosition(),
        position_component.GetSubUnitPosition());

    // Waypoints are backwards from target
    world_->BuildAndEmitEvent<EventType::kMovedStep>(
        entity.GetID(),
        position_component.GetPosition(),
        movement_component.GetVisualisationWaypoints());

    // For dash, emit moved step event for sender as well
    if (entity.Has<DashComponent>())
    {
        const auto& dash_component = entity.Get<DashComponent>();
        world_->BuildAndEmitEvent<EventType::kMovedStep>(
            dash_component.GetSenderID(),
            position_component.GetPosition(),
            movement_component.GetVisualisationWaypoints());
    }
}

void MovementSystem::OnFocusFound(const event_data::Focus& data)
{
    // Get entity from data
    const auto& entity = world_->GetByID(data.entity_id);

    // Only entities with movement component
    if (!entity.Has<MovementComponent>())
    {
        return;
    }

    // Clear waypoints and force pathfinding next time step
    auto& movement_component = entity.Get<MovementComponent>();
    movement_component.ClearWaypoints();
    movement_component.SetPathfindingLastUpdatedTimeStep(-1);
}

void MovementSystem::ResetPathfindingGraph() const
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Reset all node to default value
    pathfinding_graph.assign(world_->GetGridConfig().GetGridSize(), AStarGraphNode{});

    // Clear queue
    pathfinding_queue.Clear();
}

bool MovementSystem::ShouldMoveWithoutDecision(const Entity& entity)
{
    // Zones are a special case that move regardless of decision
    if (entity.Has<ZoneComponent>())
    {
        return true;
    }

    // We should be able to move during abilities when it's not locked
    if (entity.Has<AbilitiesComponent>())
    {
        const auto& ability_component = entity.Get<AbilitiesComponent>();
        if (ability_component.HasActiveAbility() && !ability_component.IsMovementLocked())
        {
            // Check if focus in attack range
            constexpr bool is_omega_range = false;
            if (!FocusSystem::HasFocusInAttackRange(entity, is_omega_range))
            {
                // Movement not locked and focus not in range
                return true;
            }
        }
    }

    return false;
}
}  // namespace simulation
