#pragma once

#include <array>
#include <queue>
#include <set>
#include <unordered_set>

#include "data/constants.h"
#include "ecs/entity.h"
#include "ecs/hex_grid_config.h"
#include "utility/hex_grid_position.h"
#include "utility/logger_consumer.h"
#include "utility/vector_helper.h"

namespace simulation
{
class PositionComponent;
class World;

// Struct describing an obstacle on the hex grid
// All obstacles are hexagons
struct HexGridObstacle
{
    HexGridPosition position;
    int radius = 0;
};

// Structure for A* graph nodes
// Holds all of the necessary information (distance from start to node,
// heuristic distance to goal node, and parent node)
struct AStarGraphNode
{
    // Parent of this node, if any
    size_t parent_index = kInvalidIndex;

    // Heuristic distance to target in squared units
    int goal = 0;

    // Cos of angle between vector to source and target
    int angle = 0;
};

// Map of all of the visited nodes in the graph for the find path operation
using AStarGraph = std::vector<AStarGraphNode>;

// Comparer object for A* graph nodes
class AStarNodeComparer
{
public:
    // Construct comparison object
    explicit AStarNodeComparer(const AStarGraph* graph_nodes)
    {
        graph_nodes_ = graph_nodes;
    }

    bool operator()(const size_t& first, const size_t& second) const
    {
        const auto& first_data = graph_nodes_->at(first);
        const auto& second_data = graph_nodes_->at(second);

        // WARNING: We always need to find a difference between the nodes because
        // the C++ standard does not require sort stability in a priority queue
        // See https://en.cppreference.com/w/cpp/named_req/Compare
        // Compare type must be strict weak ordering
        if (first_data.goal == second_data.goal)
        {
            if (first_data.angle == second_data.angle)
            {
                // Same goals, sort by node index to ensure sort stability
                return first > second;
            }

            // The bigger cos of angle the more it align with direction to target
            return first_data.angle > second_data.angle;
        }

        return first_data.goal > second_data.goal;
    }

private:
    const AStarGraph* graph_nodes_ = nullptr;
};

// Priority queue where we store the untested nodes for find path in order of distance
// to the goal so that we can investigate the most promising nodes first
using AStarGraphNodesQueueBase = std::priority_queue<size_t, std::vector<size_t>, AStarNodeComparer>;
// Extend std priority queue with clear method
class AStarGraphNodesQueue : public AStarGraphNodesQueueBase
{
public:
    explicit AStarGraphNodesQueue(const AStarNodeComparer& Comparer) : AStarGraphNodesQueueBase(Comparer) {}

    // Clear container
    void Clear()
    {
        // Call cleat of priority container
        c.clear();
    }
};

/* -------------------------------------------------------------------------------------------------------
 * GridHelper
 *
 * This defines grid helper functions
 * --------------------------------------------------------------------------------------------------------
 */
class GridHelper final : public LoggerConsumer
{
public:
    // Contants for obstacle map ref
    static constexpr uint8_t kEntityObstacle = 0b0000'0001;
    static constexpr uint8_t kBorderObstacle = 0b0000'0010;
    static constexpr uint8_t kEnemySideObstacle = 0b0000'0100;
    static constexpr uint8_t kAnyObstacleMask = 0b1111'1111;
    static constexpr uint8_t kNoObstacles = 0;

    GridHelper() = default;
    explicit GridHelper(World* world);

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;
    int GetTimeStepCount() const override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kMovement;
    }

    //
    // Own functions
    //

    // Does the the two hexagons defined by a center and radius intersect?
    static constexpr bool DoHexagonsIntersect(
        const HexGridPosition& center_a,
        const int radius_a,
        const HexGridPosition& center_b,
        const int radius_b)
    {
        // As hexagons just behave like circles in hex space we do the same checks as if there
        // were two circles intersecting
        const int distance_between_centers = (center_a - center_b).Length();
        const int sum_radiuses = radius_a + radius_b;

        // If the distance between the two hexagons is smaller the the total radius then it means
        // they overlap
        return distance_between_centers <= sum_radiuses;
    }

    // Gets the intersect positions between two hexagons.
    // If you want to check if two hexagons just intersect (without positions), just use
    // DoHexagonsIntersect. Reference:
    // - https://www.redblobgames.com/grids/hexagons/#range-intersection
    static std::vector<HexGridPosition> GetHexagonsIntersectPositions(
        const HexGridPosition& center_a,
        const int radius_a,
        const HexGridPosition& center_b,
        const int radius_b)
    {
        std::vector<HexGridPosition> results;

        const int q_min = (std::max)(center_a.q - radius_a, center_b.q - radius_b);
        const int r_min = (std::max)(center_a.r - radius_a, center_b.r - radius_b);
        const int s_min = (std::max)(center_a.s() - radius_a, center_b.s() - radius_b);

        const int q_max = (std::min)(center_a.q + radius_a, center_b.q + radius_b);
        const int r_max = (std::min)(center_a.r + radius_a, center_b.r + radius_b);
        const int s_max = (std::min)(center_a.s() + radius_a, center_b.s() + radius_b);
        for (int q = q_min; q <= q_max; q++)
        {
            const int r_loop_min = (std::max)(r_min, -q - s_max);
            const int r_loop_max = (std::min)(r_max, -q - s_min);
            for (int r = r_loop_min; r <= r_loop_max; r++)
            {
                results.push_back(HexGridPosition{q, r});
            }
        }

        return results;
    }

    // Gets the neighbour of position given a direction
    static constexpr HexGridPosition GetAxialNeighbour(
        const HexGridPosition& position,
        const size_t neighbour_direction_index)
    {
        return position + kHexGridNeighboursOffsets[neighbour_direction_index];
    }

    // Appends positions of a ring with specified radius to an array
    void GetSingleRingPositions(
        const HexGridPosition& center,
        const int radius,
        std::vector<HexGridPosition>* out_results) const
    {
        // Reference: https://www.redblobgames.com/grids/hexagons/#rings
        static constexpr size_t neighbours_offsets_size = kHexGridNeighboursOffsets.size();
        out_results->reserve(
            out_results->size() + neighbours_offsets_size * static_cast<size_t>((std::max)(0, radius)));
        HexGridPosition hex = center + kHexGridNeighboursOffsets[4] * radius;
        for (size_t i = 0; i < neighbours_offsets_size; i++)
        {
            for (int j = 0; j < radius; j++)
            {
                if (grid_config_.IsInMapRectangleLimits(hex))
                {
                    out_results->push_back(hex);
                }
                hex = GetAxialNeighbour(hex, i);
            }
        }
    }

    // Returns the positions of a ring around the center given radius
    std::vector<HexGridPosition> GetSingleRingPositions(const HexGridPosition& center, const int radius) const
    {
        // Reference: https://www.redblobgames.com/grids/hexagons/#rings
        std::vector<HexGridPosition> results;
        GetSingleRingPositions(center, radius, &results);
        return results;
    }

    // Returns all the rings from center with a given radius
    std::vector<HexGridPosition> GetSpiralRingsPositions(const HexGridPosition& center, const int radius) const
    {
        // Reference: https://www.redblobgames.com/grids/hexagons/#rings
        std::vector<HexGridPosition> results{center};
        for (int k = 1; k <= radius; k++)
        {
            VectorHelper::AppendVector(results, GetSingleRingPositions(center, k));
        }

        return results;
    }

    // Gets units Vector between the entities src and dst
    HexGridPosition GetUnitsVectorBetween(const EntityID src_id, const EntityID dst_id) const;
    static HexGridPosition GetUnitsVectorBetween(const Entity& src_entity, const Entity& dst_entity);

    // Gets units Distance between the entities src and dst
    static int GetUnitsDistanceBetween(const Entity& src_entity, const Entity& dst_entity);

    // Gets Subunits Vector between the entities src and dst
    HexGridPosition GetSubUnitsVectorBetween(const EntityID src_id, const EntityID dst_id) const;
    static HexGridPosition GetSubUnitsVectorBetween(const Entity& src_entity, const Entity& dst_entity);

    static HexGridPosition GetSubUnitsVectorBetweenLocation(
        const HexGridPosition& src_position,
        const HexGridPosition& dst_position)
    {
        const HexGridPosition src_position_sub_units = src_position.ToSubUnits();
        const HexGridPosition dst_position_sub_units = dst_position.ToSubUnits();
        return dst_position_sub_units - src_position_sub_units;
    }

    // Get the angle from this src to the dst position
    // Going counter-clockwise from where x > 0 and y = 0
    int GetAngle360Between(const EntityID src_id, const EntityID dst_id) const;
    static int GetAngle360Between(const Entity& src_entity, const Entity& dst_entity);

    // Helper method to get angle in the range [0, 360] between the entity and its current focus
    // If the entity does not have a focus it returns 0
    int GetAngle360BetweenFocus(const EntityID id) const;
    int GetAngle360BetweenFocus(const Entity& entity) const;

    // Helper method to get position component for a source/sender
    PositionComponent* GetSourcePositionComponent(const Entity& entity) const;

    // Get obstacles with new data for pathfinding or spawning (used by unreal)
    std::vector<HexGridObstacle> GetObstacles(const Entity* source, const int radius_needed = -1) const;

    struct BuildObstaclesParameters
    {
        const Entity* source = nullptr;
        int radius_needed = -1;
        bool mark_borders_as_obstacles = false;
        Team mark_team_side_as_obstacles = Team::kNone;
        int q_margin = 0;
        int r_margin = 0;
    };

    // Build obstacle map with new data for pathfinding or spawning
    // "Obstacles" are only used for pathfinding from the point of view of the center of the hex.
    // For example if there are two entities, one of radius 5 and the second of radius 6,
    // and we build obstacles map for an entity of radius 5, then this function will mark an
    // area of radius 11 around the second entity as an obstacle.
    // Optionally, this method can mark an area close to the grid border as an obstacle too -
    // it is currently used by pathfinding only as faster alternative to `IsInMapRectangleLimits`.
    void BuildObstacles(const BuildObstaclesParameters& build_parameters) const;

    // Helper for building obstacles
    void BuildObstacles(const int radius_needed) const
    {
        BuildObstaclesParameters parameters;
        parameters.radius_needed = radius_needed;
        BuildObstacles(parameters);
    }
    void BuildObstacles(const Entity* source) const
    {
        BuildObstaclesParameters parameters;
        parameters.source = source;
        BuildObstacles(parameters);
    }

    // Returns true if there an obstacle at specified position.
    // By default returns true if there any kind of obstacle.
    bool HasObstacleAt(const HexGridPosition& position, const uint8_t mask = kAnyObstacleMask) const
    {
        const size_t grid_index = grid_config_.GetGridIndex(position);
        return HasObstacleAt(grid_index, mask);
    }
    // Returns true if there an obstacle at specified node index.
    // By default returns true if there any kind of obstacle.
    bool HasObstacleAt(const size_t node_index, const uint8_t mask = kAnyObstacleMask) const
    {
        auto& obstacles = GetObstaclesMapRef();

        // Out of bounds, has obstacle
        if (node_index >= obstacles.size())
        {
            // assert(false);
            return true;
        }

        return (obstacles[node_index] & mask) != 0;
    }

    // Sets the obstacle map node at specified position
    void SetObstacleAt(const HexGridPosition& position, const bool value, uint8_t kind = kEntityObstacle) const
    {
        const size_t grid_index = grid_config_.GetGridIndex(position);
        if (grid_index == kInvalidIndex)
        {
            return;
        }
        GetObstaclesMapRef()[grid_index] = value ? kind : kNoObstacles;
    }

    // Sets a rectangle obstacle for specified radius
    void SetRectangleObstacle(const HexGridPosition& center, const int radius_units, const bool value) const
    {
        const int r_min = HexGridPosition::RectangleRLimitMin(-radius_units);
        const int r_max = HexGridPosition::RectangleRLimitMax(radius_units);
        for (int r = r_min; r <= r_max; r++)
        {
            const int q_min = HexGridPosition::RectangleQLimitMin(-radius_units, r);
            const int q_max = HexGridPosition::RectangleQLimitMax(radius_units, r);
            for (int q = q_min; q <= q_max; q++)
            {
                // Shift by center
                const HexGridPosition offset = HexGridPosition{q, r} + center;

                // Mark all positions for radius as an obstacle
                // row - r
                // column - q
                SetObstacleAt(offset, value);
            }
        }
    }

    // Sets a hexagon obstacle for specified radius
    void SetHexagonObstacle(const HexGridPosition& center, const int radius_units, const bool value) const
    {
        const GridLimit q_limits = HexGridPosition::HexagonQLimits(radius_units);
        for (int q = q_limits.min; q <= q_limits.max; q++)
        {
            const GridLimit r_limits = HexGridPosition::HexagonRLimits(radius_units, q);
            for (int r = r_limits.min; r <= r_limits.max; r++)
            {
                // Shift by center
                const HexGridPosition offset = HexGridPosition{q, r} + center;

                // Mark all positions for radius as an obstacle
                // row - r
                // column - q
                SetObstacleAt(offset, value);
            }
        }
    }

    // Gets an open position near an entity in the preferred direction
    HexGridPosition GetOpenPositionNear(
        const Entity& entity,
        const HexGridCardinalDirection preferred_direction,
        const int radius_needed) const;

    // Gets an open position near an location in the preferred direction
    HexGridPosition GetOpenPositionNearLocation(
        const HexGridPosition& location,
        const HexGridCardinalDirection preferred_direction,
        const int min_radius,
        const int max_radius) const;

    // Gets an open position near a location, preferred in the direction of source
    // Requires built obstacle map by calling `BuildObstacles`
    HexGridPosition GetOpenPositionNearLocationOnPath(
        const HexGridPosition& source_position,
        const HexGridPosition& target_position,
        const int radius_needed,
        const int max_distance_units) const;

    // Gets open positions within range of a location, preferred in the direction of source
    // Requires built obstacle map by calling `BuildObstacles`
    std::vector<HexGridPosition> GetOpenPositionsInRange(
        const HexGridPosition& source_position,
        const HexGridPosition& target_position,
        const int radius_needed,
        const int range_units,
        const int minimum_trial_distance,
        const size_t max_positions) const;

    // Gets farthest position from 'from_position' that is in the radius of 'center_position'
    // Requires built obstacle map by calling `BuildObstacles`
    HexGridPosition GetFarthestOpenPositionFromPointInRange(
        const HexGridPosition& center_position,
        const HexGridPosition& from_position,
        const int radius) const;

    // Gets an open position near a target location
    // Requires built obstacle map by calling `BuildObstacles`
    HexGridPosition GetOpenPositionNearby(
        const HexGridPosition& target_position,
        const int radius_needed,
        const int minimum_trial_distance = 0) const;

    // Gets an open position near a target location
    // Return closes hex beased on 2D disatnce
    // Requires built obstacle map by calling `BuildObstacles`
    HexGridPosition GetOpenPositionNearby2D(const HexGridPosition& target_position, const int radius_needed0) const;

    // Gets an open position near a target location
    // Uses preferred position for distance comparison and chooses closest to it
    // Requires built obstacle map by calling `BuildObstacles`
    HexGridPosition GetOpenPositionNearbyWithPreferredPosition(
        const HexGridPosition& target_position,
        const HexGridPosition& preferred_position,
        const int radius_needed,
        const int minimum_trial_distance = 0) const;

    // Gets an open position behind a target location
    // Requires built obstacle map by calling `BuildObstacles`
    HexGridPosition GetOpenPositionBehind(
        const HexGridPosition& source_position,
        const HexGridPosition& target_position,
        const int radius_needed,
        const int minimum_trial_distance = 0) const;

    // Gets an open position in front of a target location
    // Requires built obstacle map by calling `BuildObstacles`
    HexGridPosition GetOpenPositionInFront(
        const HexGridPosition& source_position,
        const HexGridPosition& target_position,
        const int radius_needed,
        const int minimum_trial_distance = 0) const;

    // Gets an open position at the far end of the opposite side of the board
    // Requires built obstacle map by calling `BuildObstacles`
    HexGridPosition GetOpenPositionAcross(const HexGridPosition& current_position, const int source_radius) const;

    // Helper function to find possible positions near a point
    template <typename PositionSort>
    std::vector<HexGridPosition> GetPossiblePositions(
        const HexGridPosition& position,
        const int radius_units,
        const PositionSort position_sort,
        const int trial_distance,
        const size_t max_positions) const;

    // This this hexagon position overlapping the line between the enemy and ally positions
    static constexpr bool DoesHexagonOverlapEnemyAllyLine(
        const HexGridPosition& center,
        const int radius_units,
        const int middle_line_width = 0)
    {
        // Find if top/bottom edge of the hexagon are in is_in_ally_grid_space
        // const HexGridPosition edge_position1 = center + kHexagonTopBottomOffsets[0] * radius_units;
        // const HexGridPosition edge_position2 = center + kHexagonTopBottomOffsets[1] * radius_units;

        // const bool is_on_opposite_side = IsInBlueGridSpace(edge_position1) !=
        // IsInBlueGridSpace(edge_position2); const bool is_on_middle_line = edge_position1.r == 0 ||
        // edge_position2.r == 0;

        // Simplified version, just compare the absolute radius to see if it intersects
        return Math::Abs(center.r) - radius_units <= middle_line_width;
    }

    struct ParamsIsValidHexagonPosition
    {
        HexGridPosition center;
        int radius_units = 0;
        bool is_taking_space = false;
        const std::unordered_set<EntityID>& ignored_entities = {};
        int q_margin = 0;
        int r_margin = 0;
    };

    // Checks whether a center + radius (that forms a hexagon) is valid
    // IsHexagonInGridLimits or/and !IsHexagonPositionTaken
    bool IsValidHexagonPosition(const ParamsIsValidHexagonPosition& params, std::string* out_error_message = nullptr)
        const;

    // Checks whether a hexagon position is occupied by another entity besides ignored_entities
    bool IsHexagonPositionTaken(
        const HexGridPosition& center,
        const int radius_units,
        const std::unordered_set<EntityID>& ignored_entities = {},
        std::string* out_error_message = nullptr) const;

    // Gets a list of all the positions which are in radius distance from center
    static std::vector<HexGridPosition> GetHexagonTakenPositions(const HexGridPosition& center, const int radius_units);

    // Is the position in the blue side of the board
    // NOTE: This meant previously that it was the ally space (for survival game mode for example)
    static constexpr bool IsInBlueGridSpace(const HexGridPosition& position)
    {
        return position.r > 0;
    }

    // Is the position in the red side of the board
    static constexpr bool IsInRedGridSpace(const HexGridPosition& position)
    {
        return !IsInBlueGridSpace(position);
    }

    // Try to reserve a position for abilities that use them
    void TryToReservePosition(
        const Entity& sender_entity,
        const std::set<EntityID>& predicted_targets,
        const ReservedPositionType position_type) const;

    // Return neighbor position for spawn direction
    static constexpr HexGridPosition DirectionToNeighbourPositionOffset(const HexGridCardinalDirection direction)
    {
        if (direction <= HexGridCardinalDirection::kNone || direction >= HexGridCardinalDirection::kNum)
        {
            return kInvalidHexHexGridPosition;
        }

        const size_t index = static_cast<size_t>(direction) - 1;
        return kHexGridNeighboursOffsets[index];
    }

    // Helper function to apply excessive subunits to units
    static void ApplyExcessiveSubUnitsToUnits(HexGridPosition* out_units, HexGridPosition* out_sub_units)
    {
        // Subunit value of sum
        const HexGridPosition sub_units = out_units->ToSubUnits() + *out_sub_units;

        // Use round function
        HexGridPosition::Round(sub_units, out_units, out_sub_units);
    }

    // Adds the position sub_units_position which is a position in the absolute value of the grid
    // space.
    // Units are added to the out_unit_position.
    // Sub units which do not form a total unit are added to the out_sub_unit_position
    static void AddSubUnitsPositionToPosition(
        const HexGridPosition& sub_units_position,
        HexGridPosition* out_unit_position,
        HexGridPosition* out_sub_unit_position)
    {
        // Create vectors
        const HexGridPosition add_units = sub_units_position.ToUnits();
        const HexGridPosition add_sub_units = sub_units_position.ToSubUnitsRemainder();

        // Update position
        *out_unit_position += add_units;
        *out_sub_unit_position += add_sub_units;

        // Shift excessive subunits to units
        ApplyExcessiveSubUnitsToUnits(out_unit_position, out_sub_unit_position);
    }

    // Adds the position sub_units_position which is a position in the absolute value of the grid
    // space.
    // Units are added to the out_unit_position.
    // Sub units which do not form a total unit are added to the out_sub_unit_position
    // Uses alternative inclusive rounding
    static void AddSubUnitsPositionToPositionWithAlternativeRounding(
        const HexGridPosition& sub_units_position,
        HexGridPosition* out_unit_position,
        HexGridPosition* out_sub_unit_position)
    {
        // Update position
        *out_unit_position += sub_units_position.ToUnits();
        *out_sub_unit_position += sub_units_position.ToSubUnitsRemainder();

        const HexGridPosition sub_units = out_unit_position->ToSubUnits() + *out_sub_unit_position;

        // Use alternative inclusive round method
        constexpr bool round_inclusive = true;
        HexGridPosition::RoundExtended(sub_units, round_inclusive, out_unit_position, out_sub_unit_position);
    }

    // This is the actual A* (modified) algorithm
    bool FindPathOnGrid(
        AStarGraph& graph_nodes,
        AStarGraphNodesQueue& untested_nodes,
        const HexGridPosition& source,
        const int source_radius,
        const HexGridPosition& destination,
        const int reach_distance,
        const int max_iterations,
        size_t* out_found_node) const;

    // Gets a 2d scaled position
    static constexpr IVector2D ToScaled2D(const HexGridPosition& grid_position)
    {
        // References:
        // - https://www.redblobgames.com/grids/hexagons/#hex-to-pixel-axial
        // - https://www.redblobgames.com/grids/hexagons/implementation.html#hex-to-pixel

        // Calculate world position
        // Relationship between XY and QR is represented by matrix [sqrt(3), 0, sqrt(3)/2, 3/2]
        // which can be derived from the basis vectors
        const int x = kSqrt3Scaled * grid_position.q + (kSqrt3Scaled * grid_position.r) / 2;
        const int y = (3 * kPrecisionFactor * grid_position.r) / 2;

        // Return position
        return {x, y};
    }

    // Gets a 2d scaled position (lower precision)
    static constexpr IVector2D ToScaled2D_LowerPrecision(const HexGridPosition& grid_position)
    {
        // References:
        // - https://www.redblobgames.com/grids/hexagons/#hex-to-pixel-axial
        // - https://www.redblobgames.com/grids/hexagons/implementation.html#hex-to-pixel

        // Calculate world position
        // Relationship between XY and QR is represented by matrix [sqrt(3), 0, sqrt(3)/2, 3/2]
        // which can be derived from the basis vectors// Square root of 3 scaled by kPrecisionFactor
        constexpr int sqrt3_scaled = kSqrt3Scaled / 10;
        const int precision_factor = kPrecisionFactor / 10;
        const int x = sqrt3_scaled * grid_position.q + (sqrt3_scaled * grid_position.r) / 2;
        const int y = (3 * precision_factor * grid_position.r) / 2;

        // Return position
        return {x, y};
    }

    // Gets a 2d scaled position
    static constexpr void NormalizeAndScaleVector2D(IVector2D& vector)
    {
        vector = (vector * kPrecisionFactor) / vector.Length();
    }

    // Heuristic distance for A* pathfinding
    static constexpr int CalculateAStarHeuristicGoal(const HexGridPosition& position, const HexGridPosition& target)
    {
        return (target - position).Length();
    }

    // Heuristic angle for A* pathfinding
    static int CalculateAStarHeuristicAngle(
        const HexGridPosition& source,
        const HexGridPosition& position,
        const HexGridPosition& target)
    {
        // Find distance from 'position' to projection on 'source-target' vector.
        // It is faster than finding an angle because we don't have to normalize vectors.
        const auto sp = ToScaled2D_LowerPrecision(position - source);
        const auto st = ToScaled2D_LowerPrecision(target - source);
        const auto num = Math::Sqr(static_cast<int64_t>(sp.x * st.y - sp.y * st.x));
        // Basically st.Dot(st) just with int64
        const auto den = Math::Sqr(static_cast<int64_t>(st.x)) + Math::Sqr(static_cast<int64_t>(st.y));
        // This will be a squared distance but we don't need to take a root as long as we use it for comparison only
        return static_cast<int>(num / den);
    }

    // Find closest entity
    EntityID FindClosest(
        const Entity& entity,
        const std::unordered_set<EntityID>& exclude_entities,
        const bool include_unreachable = false) const;

    const HexGridConfig& GetGridConfig() const
    {
        return grid_config_;
    }

    World& GetWorld() const
    {
        return *world_;
    }

private:
    // Comparer object for the positions
    class DefaultPositionComparer
    {
    public:
        // Construct comparison object
        DefaultPositionComparer(const GridHelper* grid_helper, const HexGridPosition& preferred_position)
        {
            grid_helper_ = grid_helper;
            preferred_position_ = &preferred_position;
        }

        // Update preferred position
        constexpr void SetPreferredPosition(const HexGridPosition& preferred_position)
        {
            preferred_position_ = &preferred_position;
        }

        // Do comparison
        // Arrange positions according to distance from the specified position
        // When we encounter an equal distance we MUST still distinguish the positions somehow
        // to ensure the sorting is stable
        bool operator()(const HexGridPosition& first, const HexGridPosition& second) const
        {
            // Get both distances from the preferred position
            const HexGridPosition first_vector = first - *preferred_position_;
            const HexGridPosition second_vector = second - *preferred_position_;

            // Prefer greater distance for priority queue
            return grid_helper_->IsPositionDistanceGreaterThanOther(first, second, first_vector, second_vector);
        }

    private:
        const GridHelper* grid_helper_ = nullptr;
        const HexGridPosition* preferred_position_ = nullptr;
    };

    class PositionComparer2D
    {
    public:
        // Construct comparison object
        PositionComparer2D(const GridHelper* grid_helper, const HexGridPosition& in_target_position)
        {
            grid_helper_ = grid_helper;
            target_position = in_target_position;
        }

        bool operator()(const HexGridPosition& first, const HexGridPosition& second) const
        {
            return grid_helper_->IsPositionDistanceGreaterThanOther2D(first, second, target_position);
        }

    private:
        const GridHelper* grid_helper_ = nullptr;
        HexGridPosition target_position;
    };

    bool IsPositionDistanceGreaterThanOther(
        const HexGridPosition& first,
        const HexGridPosition& second,
        const HexGridPosition& first_vector,
        const HexGridPosition& second_vector) const
    {
        const int first_length = first_vector.Length();
        const int second_length = second_vector.Length();
        if (first_length == second_length)
        {
            // Tiebreaker on grid index
            return grid_config_.GetGridIndex(first) > grid_config_.GetGridIndex(second);
        }
        return first_length > second_length;
    }

    bool IsPositionDistanceGreaterThanOther2D(
        const HexGridPosition& first,
        const HexGridPosition& second,
        const HexGridPosition& target) const;

    // Top/Bottom offsets of a hexagon
    // clang-format off
    static constexpr std::array<HexGridPosition, 2> kHexagonTopBottomOffsets{
        // TopRight                   // BottomLeft
        kHexGridNeighboursOffsets[1], kHexGridNeighboursOffsets[4]
    };
    // clang-format on

    ObstaclesMapType& GetObstaclesMapRef() const;

    // Owner world of this helper
    World* world_ = nullptr;

    HexGridConfig grid_config_{};
};

}  // namespace simulation
