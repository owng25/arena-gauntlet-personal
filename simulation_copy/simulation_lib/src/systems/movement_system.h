#pragma once

#include "components/position_component.h"
#include "data/constants.h"
#include "ecs/system.h"
#include "utility/grid_helper.h"
#include "utility/gtest_friend.h"

namespace simulation
{
namespace event_data
{
struct Focus;
}
class MovementComponent;

/* -------------------------------------------------------------------------------------------------------
 * MovementSystem
 *
 * This class serves to move an entity towards a focus destination
 * --------------------------------------------------------------------------------------------------------
 */
class MovementSystem : public System
{
    typedef System Super;
    typedef MovementSystem Self;

public:
    MovementSystem();

    // System initialisation function
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;

    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kMovement;
    }

    // Compute distance moved, dropping last digit of precision to avoid tiny movements
    static constexpr int ComputeDistanceMoved(const HexGridPosition& movement_vector)
    {
        return Math::FractionalRound(movement_vector.Length(), kMovementRoundingValue) * kMovementRoundingValue;
    }

private:
    // Updates the state if the focus is unreachable or not
    void NavigatingCheckFocus(const Entity& entity, const bool unreachable_focus);

    // Find all the waypoints for the entity
    void FindNavigatingWaypoints(
        const Entity& entity,
        const PositionComponent& position_component,
        HexGridPosition* out_destination_position,
        bool* out_unreachable_focus) const;

    // Find all the waypoints for the entity while fleeing
    void FindFleeingWaypoints(
        const Entity& entity,
        const PositionComponent& position_component,
        HexGridPosition* out_destination_position,
        bool* out_unreachable_focus) const;

    // Checks whether we should perform pathfinding this time step for a given entity
    bool ShouldSkipFindingWaypoint(const Entity& entity) const;

    // Move until distance moved matches movement speed or no further movement.
    // This loops through all the waypoints until movement speed is consumed.
    // Returns the distance moved, in subunits
    int TimeStepMoveEntity(
        const Entity& entity,
        const bool navigating,
        PositionComponent& position_component,
        HexGridPosition& waypoint,
        const bool waypoint_is_vector);

    // Finds the waypoints we need to get to the entity active destination
    // This is the actual A* (modified) algorithm
    bool FindPath(
        const Entity& source,
        const HexGridPosition& target_position,
        const int reach_radius,
        HexGridPosition* out_reach_position) const;

    // Checks if a path to target is available using A* (modified) algorithm
    bool TryToReachTarget(const Entity& source, const EntityID target_id) const;

    void AddWaypointsToFromFindPathToMovementComponent(
        MovementComponent* movement_component,
        AStarGraph& find_path_graph_nodes,
        const size_t start_node,
        const size_t end_node) const;

    // Checks if there is an obstacle while tracing a line between two points
    bool IsThereObstacleBetween(const HexGridPosition& position1, const HexGridPosition& position2) const;

    // Moves entity with the start position = position_component
    // to the end position = end_position
    // Returns the movement vector length
    int MoveToward(
        const Entity& entity,
        PositionComponent& position_component,
        const HexGridPosition& end_position,
        const int sub_units_to_move);

    // Moves the entity by sub_unit_delta subunits
    void MoveEntity(const Entity& entity, PositionComponent& position_component, const HexGridPosition& sub_unit_delta);

    // Listen for new focus
    void OnFocusFound(const event_data::Focus& event_data);

    // Checks if an entity check decision system before proceed with movement
    static bool ShouldMoveWithoutDecision(const Entity& entity);

    // How much should we round the movement vector inside MoveToward
    static constexpr int kMovementRoundingValue = 10;

    void ResetPathfindingGraph() const;

    mutable AStarGraph pathfinding_graph;
    AStarNodeComparer pathfinding_graph_comparer;
    mutable AStarGraphNodesQueue pathfinding_queue;

    // Google test friends
#ifdef GOOGLE_UNIT_TEST
    FRIEND_TEST(MovementSystemTest, FindPathSimple);
    FRIEND_TEST(MovementSystemTest, FindPathDirectObstructed1);
    FRIEND_TEST(MovementSystemTest, FindPathDirectObstructed2);
    FRIEND_TEST(MovementSystemTest, FindPathDirectCloseObstacle1);
    FRIEND_TEST(MovementSystemTest, FindPathDirectCloseObstacle2);
    FRIEND_TEST(MovementSystemTest, FindPathComplex);
    FRIEND_TEST(MovementSystemTest, FindPathBlocked);
    FRIEND_TEST(MovementSystemTest, FindValidLocation);
    FRIEND_TEST(MovementSystemTest, GetDistance);
    FRIEND_TEST(MovementSystemTest, BuildObstacles);
    FRIEND_TEST(MovementSystemTest, MoveToward);
    FRIEND_TEST(MovementSystemTest, MoveEntity);
    FRIEND_TEST(MovementSystemTest, HasObstacleFalse);
    FRIEND_TEST(MovementSystemTest, HasObstacleTrue);
    FRIEND_TEST(MovementSystemTest, HasObstacleSmallSize);
    FRIEND_TEST(MovementSystemTest, HasObstacleMediumSize);
    FRIEND_TEST(MovementSystemTest, HasObstacleLargeSize);
#endif  // GOOGLE_UNIT_TEST

};  // class MovementSystem

}  // namespace simulation
