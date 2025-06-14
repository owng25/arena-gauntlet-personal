#include "utility/grid_helper.h"

#include <queue>

#include "components/combat_unit_component.h"
#include "components/dash_component.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "components/shield_component.h"
#include "components/stats_component.h"
#include "ecs/event_types_data.h"
#include "entity_helper.h"
#include "utility/enum.h"

namespace simulation
{
GridHelper::GridHelper(World* world) : world_(world), grid_config_(world->GetGridConfig()) {}

std::shared_ptr<Logger> GridHelper::GetLogger() const
{
    return world_->GetLogger();
}

void GridHelper::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int GridHelper::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

HexGridPosition GridHelper::GetUnitsVectorBetween(const EntityID src_id, const EntityID dst_id) const
{
    if (!world_->HasEntity(src_id) || !world_->HasEntity(dst_id))
    {
        return {};
    }

    return GetUnitsVectorBetween(world_->GetByID(src_id), world_->GetByID(dst_id));
}

HexGridPosition GridHelper::GetUnitsVectorBetween(const Entity& src_entity, const Entity& dst_entity)
{
    if (!src_entity.Has<PositionComponent>() || !dst_entity.Has<PositionComponent>())
    {
        return {};
    }

    const HexGridPosition& src_position = src_entity.Get<PositionComponent>().GetPosition();
    const HexGridPosition& dst_position = dst_entity.Get<PositionComponent>().GetPosition();
    return dst_position - src_position;
}

int GridHelper::GetUnitsDistanceBetween(const Entity& src_entity, const Entity& dst_entity)
{
    if (!src_entity.Has<PositionComponent>() || !dst_entity.Has<PositionComponent>())
    {
        return 0;
    }

    const HexGridPosition position = GetUnitsVectorBetween(src_entity, dst_entity);
    return position.Length();
}

HexGridPosition GridHelper::GetSubUnitsVectorBetween(const EntityID src_id, const EntityID dst_id) const
{
    if (!world_->HasEntity(src_id) || !world_->HasEntity(dst_id))
    {
        return {};
    }

    return GetSubUnitsVectorBetween(world_->GetByID(src_id), world_->GetByID(dst_id));
}

HexGridPosition GridHelper::GetSubUnitsVectorBetween(const Entity& src_entity, const Entity& dst_entity)
{
    if (!src_entity.Has<PositionComponent>() || !dst_entity.Has<PositionComponent>())
    {
        return {};
    }

    const HexGridPosition& src_position = src_entity.Get<PositionComponent>().GetPosition();
    const HexGridPosition& dst_position = dst_entity.Get<PositionComponent>().GetPosition();
    const HexGridPosition src_position_sub_units = src_position.ToSubUnits();
    const HexGridPosition dst_position_sub_units = dst_position.ToSubUnits();
    return dst_position_sub_units - src_position_sub_units;
}

int GridHelper::GetAngle360Between(const EntityID src_id, const EntityID dst_id) const
{
    if (!world_->HasEntity(src_id) || !world_->HasEntity(dst_id))
    {
        return 0;
    }

    return GetAngle360Between(world_->GetByID(src_id), world_->GetByID(dst_id));
}

int GridHelper::GetAngle360Between(const Entity& src_entity, const Entity& dst_entity)
{
    if (!src_entity.Has<PositionComponent>() || !dst_entity.Has<PositionComponent>())
    {
        return {};
    }

    const auto& src_position_component = src_entity.Get<PositionComponent>();
    const auto& dst_position_component = dst_entity.Get<PositionComponent>();
    return src_position_component.AngleToPosition(dst_position_component);
}

int GridHelper::GetAngle360BetweenFocus(const EntityID id) const
{
    if (!world_->HasEntity(id))
    {
        return 0;
    }

    return GetAngle360BetweenFocus(world_->GetByID(id));
}

int GridHelper::GetAngle360BetweenFocus(const Entity& entity) const
{
    if (!entity.Has<FocusComponent>())
    {
        LogWarn(entity.GetID(), "GetAngle360BetweenFocus - Does not have a FocusComponent");
        return 0;
    }

    // Get the focus entity
    const auto& focus_entity = entity.Get<FocusComponent>().GetFocus();
    if (!focus_entity)
    {
        LogWarn(entity.GetID(), "GetAngle360BetweenFocus - Does not have a valid Focus");
        return 0;
    }

    return GetAngle360Between(entity, *focus_entity);
}

PositionComponent* GridHelper::GetSourcePositionComponent(const Entity& entity) const
{
    PositionComponent* position_component = nullptr;
    EntityID sender_id = kInvalidEntityID;
    if (entity.Has<PositionComponent>())
    {
        position_component = &(entity.Get<PositionComponent>());
    }
    else if (entity.Has<DashComponent>())
    {
        // Get position from dash sender
        const auto& dash_component = entity.Get<DashComponent>();
        sender_id = dash_component.GetSenderID();
    }
    else if (entity.Has<ShieldComponent>())
    {
        // Get position component from shield owner
        const auto& shield_state_component = entity.Get<ShieldComponent>();
        sender_id = shield_state_component.GetSenderID();
    }

    // Is not an entity with a PositionComponent, find it in the sender/parents tree.
    if (position_component == nullptr && sender_id != kInvalidEntityID)
    {
        // Fallback to combat unit?
        bool use_combat_unit_parent_id = false;
        if (world_->HasEntity(sender_id))
        {
            // Has the entity, but it has the PositionComponent?
            const auto& sender_entity = world_->GetByID(sender_id);
            if (!sender_entity.Has<PositionComponent>())
            {
                use_combat_unit_parent_id = true;
            }
        }
        else
        {
            // Sender id does not exist, so use the top most parent combat unit
            use_combat_unit_parent_id = true;
        }

        if (use_combat_unit_parent_id)
        {
            sender_id = world_->GetCombatUnitParentID(entity.GetID());
        }

        if (world_->HasEntity(sender_id))
        {
            const auto& base_entity = world_->GetByID(sender_id);
            position_component = &(base_entity.Get<PositionComponent>());
        }
    }

    return position_component;
}

// Get obstacles with new data for pathfinding or spawning.
// Callback signature: void(const HexGridPosition& center, const int radius)
template <typename Callback>
static void
ForEachObstacle(const GridHelper& grid_helper, const Entity* source, const int radius_needed, Callback&& callback)
{
    ILLUVIUM_PROFILE_FUNCTION();

    static constexpr std::string_view method_name = "ForEachObstacle";

    // Default values
    EntityID source_id = kInvalidEntityID;
    EntityID destination_id = kInvalidEntityID;

    // Get source ID if specified
    if (source != nullptr)
    {
        source_id = source->GetID();
    }

    // One of these parameters must be valid
    if (source == nullptr && radius_needed < 0)
    {
        grid_helper.LogErr(source_id, "{} - called with invalid parameters", method_name);
        return;
    }

    // Use appropriate size for calculations
    int actual_radius_needed = radius_needed;
    if (actual_radius_needed < 0)
    {
        // Sanity check
        if (!(source->Has<PositionComponent>() || source->Has<DashComponent>()))
        {
            grid_helper.LogErr(source_id, "{} - Trying to use source entity without position", method_name);
            return;
        }

        // Get position component of source
        const auto* source_position_component = grid_helper.GetSourcePositionComponent(*source);
        actual_radius_needed = source_position_component->GetRadius();
    }

    // Add entities as obstacles
    for (const auto& other_entity : grid_helper.GetWorld().GetAll())
    {
        // Ignore entities that are not valid
        if (other_entity.get() == nullptr)
        {
            grid_helper.LogWarn(source_id, "{} - ignored some invalid entity", method_name);
            continue;
        }

        // Ignore entities that are not taking space
        if (!EntityHelper::IsCollidable(*other_entity))
        {
            continue;
        }

        // Ignore self or destination
        if (source_id == other_entity->GetID() || destination_id == other_entity->GetID())
        {
            continue;
        }

        // Get position component of other other_entity
        const auto& other_position_component = other_entity->Get<PositionComponent>();

        // Calculate radius encompassing self and other
        const int obstacle_radius = actual_radius_needed + other_position_component.GetRadius();

        // Ignore entities that can be overlapped
        if (!other_position_component.IsOverlapable())
        {
            callback(other_position_component.GetPosition(), obstacle_radius);
        }

        // Get reserved positions and add to results regardless of overlapable
        const HexGridPosition& reserved_position = other_position_component.GetReservedPosition();
        if (reserved_position != kInvalidHexHexGridPosition)
        {
            callback(reserved_position, obstacle_radius);
        }
    }
}

std::vector<HexGridObstacle> GridHelper::GetObstacles(const Entity* source, const int radius_needed) const
{
    std::vector<HexGridObstacle> obstacles;
    ForEachObstacle(
        *this,
        source,
        radius_needed,
        [&](const HexGridPosition& center, const int obstacle_radius)
        {
            obstacles.push_back(HexGridObstacle{center, obstacle_radius});
        });

    return obstacles;
}

void GridHelper::BuildObstacles(const BuildObstaclesParameters& build_parameters) const
{
    ILLUVIUM_PROFILE_FUNCTION();

    auto& obstacle_map_ref = world_->GetObstacleMapRef();

    // Clear the marked obstacles
    std::fill(obstacle_map_ref.begin(), obstacle_map_ref.end(), kNoObstacles);

    // Use appropriate size for calculations
    int actual_radius_needed = build_parameters.radius_needed;
    if (actual_radius_needed < 0)
    {
        // Sanity check
        const Entity* source = build_parameters.source;
        if (source && (source->Has<PositionComponent>() || source->Has<DashComponent>()))
        {
            // Get position component of source
            const auto* source_position_component = GetSourcePositionComponent(*source);
            actual_radius_needed = source_position_component->GetRadius();
        }
    }

    if (actual_radius_needed > 0)
    {
        const int width = grid_config_.GetGridWidth();
        const int height = grid_config_.GetGridHeight();

        if (build_parameters.mark_borders_as_obstacles)
        {
            int border_width_q = actual_radius_needed + build_parameters.q_margin;
            int border_width_r = actual_radius_needed + build_parameters.r_margin;

            // [0..radius)
            for (int row_index = 0; row_index != border_width_r; ++row_index)
            {
                const auto row_begin = obstacle_map_ref.begin() + width * row_index;
                const auto row_end = row_begin + width;
                std::fill(row_begin, row_end, kBorderObstacle);
            }

            // [radius..height-radius)
            const int last_middle_row = height - border_width_r;
            for (int row_index = border_width_r; row_index != last_middle_row; ++row_index)
            {
                const auto row_begin = obstacle_map_ref.begin() + width * row_index;
                const auto row_end = row_begin + width;
                std::fill(row_begin, row_begin + border_width_q, kBorderObstacle);
                std::fill(row_end - border_width_q, row_end, kBorderObstacle);
            }

            // [height-radius..height)
            for (int row_index = height - border_width_r; row_index != height; ++row_index)
            {
                const auto row_begin = obstacle_map_ref.begin() + width * row_index;
                const auto row_end = row_begin + width;
                std::fill(row_begin, row_end, kBorderObstacle);
            }
        }

        const Team team_side = build_parameters.mark_team_side_as_obstacles;
        if (team_side != Team::kNone)
        {
            constexpr int middle_line_width = 1;

            if (team_side == Team::kBlue)
            {
                const int half_board_floor = height / 2;
                const auto fill_amount = (half_board_floor + middle_line_width + actual_radius_needed) * width;
                std::fill_n(obstacle_map_ref.begin(), fill_amount, kEnemySideObstacle);
            }
            else if (team_side == Team::kRed)
            {
                const int half_board_ceil = (height + 1) / 2;
                const int begin_fill_index = (half_board_ceil - middle_line_width - actual_radius_needed) * width;
                std::fill(obstacle_map_ref.begin() + begin_fill_index, obstacle_map_ref.end(), kEnemySideObstacle);
            }
        }
    }

    // Get the new obstacles and mark on the obstacle map
    ForEachObstacle(
        *this,
        build_parameters.source,
        build_parameters.radius_needed,
        [&](const HexGridPosition& center, const int obstacle_radius)
        {
            SetHexagonObstacle(center, obstacle_radius, true);
        });
}

bool GridHelper::IsPositionDistanceGreaterThanOther2D(
    const HexGridPosition& first,
    const HexGridPosition& second,
    const HexGridPosition& target) const
{
    const IVector2D first_2d = ToScaled2D(first);
    const IVector2D second_2d = ToScaled2D(second);
    const IVector2D target_2d = ToScaled2D(target);

    const IVector2D first_vector = first_2d - target_2d;
    const IVector2D second_vector = second_2d - target_2d;

    const int64_t first_square_length = first_vector.SquareLength();
    const int64_t second_square_length = second_vector.SquareLength();

    if (first_square_length == second_square_length)
    {
        // Tiebreaker on grid index
        return grid_config_.GetGridIndex(first) > grid_config_.GetGridIndex(second);
    }
    return first_square_length > second_square_length;
}

ObstaclesMapType& GridHelper::GetObstaclesMapRef() const
{
    return world_->GetObstacleMapRef();
}

HexGridPosition GridHelper::GetOpenPositionNear(
    const Entity& entity,
    const HexGridCardinalDirection preferred_direction,
    const int radius_needed) const
{
    assert(entity.Has<PositionComponent>());

    // Get entity position component
    const auto& position_component = entity.Get<PositionComponent>();

    // Start beyond the edge of this entity
    const int trial_distance = position_component.GetRadius() + 1;

    return GetOpenPositionNearLocation(
        position_component.GetPosition(),
        preferred_direction,
        trial_distance,
        radius_needed);
}

HexGridPosition GridHelper::GetOpenPositionNearLocation(
    const HexGridPosition& position,
    const HexGridCardinalDirection preferred_direction,
    const int min_radius,
    const int max_radius) const
{
    int trial_distance = min_radius;

    // Find all occupied spaces
    BuildObstacles(max_radius);

    // Find the limit for searching
    const int limit = std::max(
        std::max(position.q, grid_config_.GetMaxHexGridPosition().q - position.q),
        std::max(position.r, grid_config_.GetMaxHexGridPosition().r - position.r));

    // Get the preferred neighbour
    const HexGridPosition neighbour_position = DirectionToNeighbourPositionOffset(preferred_direction);

    // Find the closest available spot, preferably in specified direction
    auto position_sort = DefaultPositionComparer(this, HexGridPosition{});
    do  // NOLINT
    {
        // TODO(vampy): Add using HexGridPosition overloads
        const int preferred_q = std::clamp(
            position.q + neighbour_position.q * trial_distance,
            grid_config_.GetMinHexGridPosition().q,
            grid_config_.GetMaxHexGridPosition().q);
        const int preferred_r = std::clamp(
            position.r + neighbour_position.r * trial_distance,
            grid_config_.GetMinHexGridPosition().r,
            grid_config_.GetMaxHexGridPosition().r);

        const HexGridPosition preferred_position{preferred_q, preferred_r};
        if (!grid_config_.IsInMapRectangleLimits(preferred_position))
        {
            continue;
        }

        // Get possible position
        position_sort.SetPreferredPosition(preferred_position);
        const std::vector<HexGridPosition> results =
            GetPossiblePositions(position, max_radius, position_sort, trial_distance, 1);

        // Check result and return if valid
        if (!results.empty())
        {
            return results.front();
        }

        // Try further location
        trial_distance++;
    } while (trial_distance < limit);

    return kInvalidHexHexGridPosition;
}

HexGridPosition GridHelper::GetOpenPositionNearLocationOnPath(
    const HexGridPosition& source_position,
    const HexGridPosition& target_position,
    const int radius_needed,
    const int max_distance_units) const
{
    // Start 1 away from actual location
    int trial_distance = 1;

    // Find the limit for searching
    const int max_range_units = std::max(
        grid_config_.GetMaxHexGridPosition().q - target_position.q,
        grid_config_.GetMaxHexGridPosition().r - target_position.r);
    const int limit = std::min(max_range_units, max_distance_units);

    // Find the closest available spot
    const auto position_sort = DefaultPositionComparer(this, source_position);
    do  // NOLINT
    {
        // Get possible position
        const std::vector<HexGridPosition> results =
            GetPossiblePositions(target_position, radius_needed, position_sort, trial_distance, 1);

        // Check result and return if valid
        if (!results.empty())
        {
            return results.front();
        }

        // Try further location
        trial_distance++;
    } while (trial_distance < limit);

    return kInvalidHexHexGridPosition;
}

std::vector<HexGridPosition> GridHelper::GetOpenPositionsInRange(
    const HexGridPosition& source_position,
    const HexGridPosition& target_position,
    const int radius_needed,
    const int range_units,
    const int minimum_trial_distance,
    const size_t max_positions) const
{
    const int maximum_map_range = grid_config_.GetDistanceToFarthestHexInMapRectangle(target_position);

    // Start at range away from actual location
    int trial_distance = std::min(maximum_map_range, range_units);

    std::vector<HexGridPosition> found_positions;

    // Find the closest available spot to source
    const auto position_sort = DefaultPositionComparer(this, source_position);
    do  // NOLINT
    {
        // Get possible positions
        const std::vector<HexGridPosition> results = GetPossiblePositions(
            target_position,
            radius_needed,
            position_sort,
            trial_distance,
            max_positions - found_positions.size());

        // Check result and return if valid
        if (!results.empty())
        {
            VectorHelper::AppendVector(found_positions, results);
            if (found_positions.size() >= max_positions)
            {
                return found_positions;
            }
        }

        // Try closer location
        trial_distance--;
    } while (trial_distance >= minimum_trial_distance);

    return found_positions;
}

HexGridPosition GridHelper::GetFarthestOpenPositionFromPointInRange(
    const HexGridPosition& center_position,
    const HexGridPosition& from_position,
    const int radius) const
{
    // Get all positions in radius from center
    auto positions = GetSpiralRingsPositions(center_position, radius);

    // Filter out blocked positions
    const auto last_not_blocked = std::remove_if(
        std::begin(positions),
        std::end(positions),
        [&](const HexGridPosition& position)
        {
            return HasObstacleAt(position);
        });

    // Init position comparer using from_position
    const auto position_sort = DefaultPositionComparer(this, from_position);

    // Use std::min_element to find farthest position
    const auto farthest_position = std::min_element(std::begin(positions), last_not_blocked, position_sort);

    // Check result and return if valid
    if (farthest_position != positions.end())
    {
        return *farthest_position;
    }

    return kInvalidHexHexGridPosition;
}

HexGridPosition GridHelper::GetOpenPositionNearby(
    const HexGridPosition& target_position,
    const int radius_needed,
    const int minimum_trial_distance) const
{
    return GetOpenPositionNearbyWithPreferredPosition(
        target_position,
        target_position,
        radius_needed,
        minimum_trial_distance);
}

HexGridPosition GridHelper::GetOpenPositionNearby2D(const HexGridPosition& target_position, const int radius_needed)
    const
{
    const int maximum_map_range = std::min(
        grid_config_.GetMaxHexGridDistanceUnits(),
        grid_config_.GetDistanceToFarthestHexInMapRectangle(target_position));

    // Find the closest available spot to preferred position
    const auto position_sort = PositionComparer2D(this, target_position);

    for (int trial_distance = 0; trial_distance <= maximum_map_range; ++trial_distance)
    {
        // Get possible positions
        const std::vector<HexGridPosition> results =
            GetPossiblePositions(target_position, radius_needed, position_sort, trial_distance, 1);

        // Check result and return if valid
        if (!results.empty())
        {
            return results.front();
        }
    }

    return kInvalidHexHexGridPosition;
}

HexGridPosition GridHelper::GetOpenPositionNearbyWithPreferredPosition(
    const HexGridPosition& target_position,
    const HexGridPosition& preferred_position,
    const int radius_needed,
    const int minimum_trial_distance) const
{
    const int maximum_map_range = std::min(
        grid_config_.GetMaxHexGridDistanceUnits(),
        grid_config_.GetDistanceToFarthestHexInMapRectangle(target_position));

    // Start at distance from known occupied space (zero if occupancy not known)
    int trial_distance = minimum_trial_distance;

    // Find the closest available spot to preferred position
    const auto position_sort = DefaultPositionComparer(this, preferred_position);
    do  // NOLINT
    {
        // Get possible positions
        const std::vector<HexGridPosition> results =
            GetPossiblePositions(target_position, radius_needed, position_sort, trial_distance, 1);

        // Check result and return if valid
        if (!results.empty())
        {
            return results.front();
        }

        // Try further location
        trial_distance++;
    } while (trial_distance <= maximum_map_range);

    return kInvalidHexHexGridPosition;
}

HexGridPosition GridHelper::GetOpenPositionBehind(
    const HexGridPosition& source_position,
    const HexGridPosition& target_position,
    const int radius_needed,
    const int minimum_trial_distance) const
{
    // Reflect on both axes using target_position as origin
    // Get reference for reflection (remove offset to target, target = 0, 0)
    const HexGridPosition reference_position = source_position - target_position;
    // Reflect and restore offset to target
    const HexGridPosition reflected_position = reference_position.Reflect() + target_position;

    return GetOpenPositionNearbyWithPreferredPosition(
        target_position,
        reflected_position,
        radius_needed,
        minimum_trial_distance);
}

HexGridPosition GridHelper::GetOpenPositionInFront(
    const HexGridPosition& source_position,
    const HexGridPosition& target_position,
    const int radius_needed,
    const int minimum_trial_distance) const
{
    return GetOpenPositionNearbyWithPreferredPosition(
        target_position,
        source_position,
        radius_needed,
        minimum_trial_distance);
}

HexGridPosition GridHelper::GetOpenPositionAcross(const HexGridPosition& current_position, const int source_radius)
    const
{
    // Convert to offset position
    IVector2D offset_target = current_position.ToOffSetOddR();

    // Get the furthest extreme on the opposite side
    if (IsInBlueGridSpace(current_position))
    {
        offset_target.y = source_radius - grid_config_.GetRectangleHeightExtent();
    }
    else
    {
        offset_target.y = grid_config_.GetRectangleHeightExtent() - source_radius;
    }

    // Convert back to axial
    const HexGridPosition target_position = HexGridPosition::FromOffsetOddR(offset_target);

    // Find the closest open space
    return GetOpenPositionNearby(target_position, source_radius);
}

template <typename PositionSort>
std::vector<HexGridPosition> GridHelper::GetPossiblePositions(
    const HexGridPosition& position,
    const int radius_units,
    const PositionSort position_sort,
    const int trial_distance,
    const size_t max_positions) const
{
    // Based on: https://www.redblobgames.com/grids/hexagons/#rings

    // Track possible positions
    std::vector<HexGridPosition> possible_positions_vector;
    std::priority_queue<HexGridPosition, std::vector<HexGridPosition>, decltype(position_sort)> possible_positions(
        position_sort,
        possible_positions_vector);
    std::vector<HexGridPosition> found_positions;

    // Add center point if open
    if (grid_config_.IsHexagonInGridLimits(position, radius_units) && !HasObstacleAt(position))
    {
        possible_positions.push(position);
    }

    // Get all other test positions
    const auto& test_positions = GetSingleRingPositions(position, trial_distance);
    for (const auto& test_position : test_positions)
    {
        if (grid_config_.IsHexagonInGridLimits(test_position, radius_units) && !HasObstacleAt(test_position))
        {
            possible_positions.push(test_position);
        }
    }

    // If there's any position, add them all
    while (!possible_positions.empty() && found_positions.size() < max_positions)
    {
        found_positions.push_back(possible_positions.top());
        possible_positions.pop();
    }

    return found_positions;
}

bool GridHelper::IsValidHexagonPosition(const ParamsIsValidHexagonPosition& params, std::string* out_error_message)
    const
{
    if (!grid_config_.IsHexagonInGridLimits(params.center, params.radius_units, params.q_margin, params.r_margin))
    {
        if (out_error_message)
        {
            *out_error_message = fmt::format(
                "Hexagon [center = {}, radius_units = {}] is NOT the map grid limits.",
                params.center,
                params.radius_units);
        }
        return false;
    }

    // Check if that position is taken
    if (params.is_taking_space)
    {
        if (IsHexagonPositionTaken(params.center, params.radius_units, params.ignored_entities, out_error_message))
        {
            return false;
        }

        // Game hasn't started but the hexagon of this unit overlap the line between the
        // ally/enemy zone
        if (!world_->IsBattleStarted() &&
            DoesHexagonOverlapEnemyAllyLine(params.center, params.radius_units, world_->GetMiddleLineWidth()))
        {
            if (out_error_message)
            {
                *out_error_message = fmt::format(
                    "Hexagon [center = {}, radius_units = {}] overlaps the enemy/ally line",
                    params.center,
                    params.radius_units);
            }
            return false;
        }
    }

    return true;
}

bool GridHelper::IsHexagonPositionTaken(
    const HexGridPosition& center,
    const int radius_units,
    const std::unordered_set<EntityID>& ignored_entities,
    std::string* out_error_message) const
{
    // Get all the valid position components
    for (const auto& entity : world_->GetAll())
    {
        // Skip specified entities
        const EntityID entity_id = entity->GetID();
        if (ignored_entities.contains(entity_id))
        {
            continue;
        }

        // Skip entities that don't take space
        if (!EntityHelper::IsCollidable(*entity))
        {
            continue;
        }

        // Don't place on occupied positions, or positions with insufficient space
        const auto& other_position_component = entity->Get<PositionComponent>();
        const int other_radius = other_position_component.GetRadius();
        if (DoHexagonsIntersect(center, radius_units, other_position_component.GetPosition(), other_radius))
        {
            if (out_error_message)
            {
                std::string other_type_id;
                if (entity->Has<CombatUnitComponent>())
                {
                    other_type_id = fmt::format("{}", entity->Get<CombatUnitComponent>().GetTypeID());
                }

                *out_error_message = fmt::format(
                    "Hexagon [center = {}, radius_units = {}] is TAKEN by - other_type_id: {{{}}} other_team = {}, "
                    "Other Hexagon [center = {}, radius_units = {}]",
                    center,
                    radius_units,
                    other_type_id,
                    entity->GetTeam(),
                    other_position_component.GetPosition(),
                    other_radius);
            }
            return true;
        }
    }

    return false;
}

std::vector<HexGridPosition> GridHelper::GetHexagonTakenPositions(const HexGridPosition& center, const int radius_units)
{
    std::vector<HexGridPosition> results;

    // Walk over all the values of the hexagon
    const GridLimit q_limits = HexGridPosition::HexagonQLimits(radius_units);
    for (int q = q_limits.min; q <= q_limits.max; q++)
    {
        const GridLimit r_limits = HexGridPosition::HexagonRLimits(radius_units, q);
        for (int r = r_limits.min; r <= r_limits.max; r++)
        {
            const HexGridPosition hex{q, r};

            // Shift by center
            results.push_back(hex + center);
        }
    }

    return results;
}

void GridHelper::TryToReservePosition(
    const Entity& sender_entity,
    const std::set<EntityID>& predicted_targets,
    const ReservedPositionType position_type) const
{
    // Get the source position component to work on
    PositionComponent* position_component = GetSourcePositionComponent(sender_entity);
    if (position_component == nullptr)
    {
        // Nothing to work with
        return;
    }

    // Build obstacle map to find open spots
    BuildObstacles(&sender_entity);

    HexGridPosition reserved_position = kInvalidHexHexGridPosition;

    const PositionComponent* receiver_position_component = nullptr;
    if (position_type == ReservedPositionType::kNearReceiver || position_type == ReservedPositionType::kBehindReceiver)
    {
        // Can only do it if we have at least one predicted receiver
        // Get the first one that is not sender
        const auto sender_id = sender_entity.GetID();
        for (const auto& predicted_receiver_id : predicted_targets)
        {
            if (predicted_receiver_id == sender_id)
            {
                continue;
            }

            const auto& predicted_receiver = world_->GetByID(predicted_receiver_id);
            if (predicted_receiver.Has<PositionComponent>())
            {
                receiver_position_component = &predicted_receiver.Get<PositionComponent>();

                // Success, don't continue
                break;
            }
        }
    }

    // Switch on position type
    switch (position_type)
    {
    case ReservedPositionType::kNearReceiver:
    {
        if (receiver_position_component != nullptr)
        {
            // Get open spot near receiver
            reserved_position =
                GetOpenPositionNearby(receiver_position_component->GetPosition(), position_component->GetRadius());
        }
        break;
    }
    case ReservedPositionType::kBehindReceiver:
    {
        if (receiver_position_component != nullptr)
        {
            // Get open spot behind receiver
            reserved_position = GetOpenPositionBehind(
                position_component->GetPosition(),
                receiver_position_component->GetPosition(),
                position_component->GetRadius());
        }
        break;
    }
    case ReservedPositionType::kAcross:
    {
        // Get open spot on opposite side
        reserved_position = GetOpenPositionAcross(position_component->GetPosition(), position_component->GetRadius());
        break;
    }
    case ReservedPositionType::kNone:
    default:
    {
        LogErr(sender_entity.GetID(), "Invalid position type for reservation");
    }
    }

    if (reserved_position != kInvalidHexHexGridPosition)
    {
        // Reserve the space
        position_component->SetReservedPosition(reserved_position);
        LogDebug(sender_entity.GetID(), "GridHelper::TryToReservePosition - reserved position = {}", reserved_position);
    }
}

bool GridHelper::FindPathOnGrid(
    AStarGraph& graph_nodes,
    AStarGraphNodesQueue& untested_nodes,
    const HexGridPosition& source,
    const int source_radius,
    const HexGridPosition& destination,
    const int reach_distance,
    const int max_iterations,
    size_t* out_found_node) const
{
    ILLUVIUM_PROFILE_FUNCTION();

    const size_t start_node = grid_config_.GetGridIndex(source);

    graph_nodes[start_node].goal = CalculateAStarHeuristicGoal(source, destination);
    constexpr int max_angle = 180;
    graph_nodes[start_node].angle = max_angle;
    graph_nodes[start_node].parent_index = kInvalidIndex;

    // Init untested nodes with start only if they empty
    if (untested_nodes.empty())
    {
        untested_nodes.push(start_node);
    }

    // Cache values
    static constexpr size_t neighbours_offsets_size = kHexGridNeighboursOffsets.size();

    // Count iterations of the loop and cap it at max_iterations parameter.
    int iterations = 0;

    // Scan untested nodes
    while (!untested_nodes.empty() && iterations < max_iterations)
    {
        // Explore front of list for possible path
        const size_t current_node = untested_nodes.top();
        untested_nodes.pop();

        // Grid coordinates from indices
        const HexGridPosition current_position = grid_config_.GetCoordinates(current_node);
        const HexGridPosition diff = destination - current_position;

        // Exit if in reach distance
        if (diff.Length() <= reach_distance)
        {
            *out_found_node = current_node;
            break;
        }

        // Iterate all neighbours
        for (size_t i = 0; i < neighbours_offsets_size; i++)
        {
            // Get neighbour details
            const HexGridPosition neighbour_position = current_position + kHexGridNeighboursOffsets[i];

            // Check bounds
            if (source_radius < 1 && !grid_config_.IsInMapRectangleLimits(neighbour_position))
            {
                continue;
            }

            // Check blocked
            const size_t neighbour = grid_config_.GetGridIndexUnsafe(neighbour_position);
            if (HasObstacleAt(neighbour))
            {
                continue;
            }

            // Check whether goal is assigned
            AStarGraphNode& neighbour_node = graph_nodes[neighbour];
            if (neighbour_node.goal != 0)
            {
                continue;
            }

            // Store parent reference
            neighbour_node.parent_index = current_node;

            // Add goal and angle value to this neighbour
            neighbour_node.goal = CalculateAStarHeuristicGoal(neighbour_position, destination);
            neighbour_node.angle = CalculateAStarHeuristicAngle(source, neighbour_position, destination);

            // Add neighbour for testing
            untested_nodes.push(neighbour);
        }

        iterations++;
    }

#ifdef ENABLE_VISUALIZATION
    // Send a copy of A* graph for debugging when visualization is enabled
    world_->BuildAndEmitEvent<EventType::kPathfindingDebugData>(
        start_node,
        grid_config_.GetGridIndex(destination),
        *out_found_node,
        source_radius,
        graph_nodes,
        world_->GetObstacleMapRef());
#endif  // ENABLE_VISUALIZATION

    // Return true if we found valid node that reach destination
    return *out_found_node != kInvalidIndex;
}

EntityID GridHelper::FindClosest(
    const Entity& entity,
    const std::unordered_set<EntityID>& exclude_entities,
    const bool include_unreachable) const
{
    EntityID closest_entity_id = kInvalidEntityID;

    const auto& focus_component = entity.Get<FocusComponent>();

    // Determine the set of entities to exclude by merging specified set with unreachable set
    const std::unordered_set<EntityID>& unreachable_entity_set = focus_component.GetUnreachable();

    // Get entity position
    const PositionComponent* position_component = GetSourcePositionComponent(entity);

    // Iterate over all the combat units
    // Find the closest enemy and set them as our focus
    int closest_dist = std::numeric_limits<int>::max();
    int closest_angle = 0;

    // Get the targeting guidance
    const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
    const EnumSet<GuidanceType> targeting_guidance = targeting_helper.GetTargetingGuidanceForEntity(entity);

    for (const auto& other_entity : world_->GetAll())
    {
        const EntityID other_entity_id = other_entity->GetID();

        if (!include_unreachable && unreachable_entity_set.count(other_entity_id))
        {
            continue;
        }

        if (exclude_entities.count(other_entity_id))
        {
            continue;
        }

        if (!EntityHelper::IsTargetable(*world_, other_entity_id))
        {
            continue;
        }

        // No friendly fire
        if (other_entity->IsAlliedWith(entity))
        {
            continue;
        }

        // Only focus entities that have required components
        if (!other_entity->Has<StatsComponent, PositionComponent>())
        {
            continue;
        }

        // Targeting guidance must match
        if (!targeting_helper.DoesEntityMatchesGuidance(targeting_guidance, entity.GetID(), other_entity_id))
        {
            continue;
        }

        // Get other entity position
        auto& other_position_component = other_entity->Get<PositionComponent>();

        // Get vector and distance between entities
        const HexGridPosition vector_to_other =
            other_position_component.GetPosition() - position_component->GetPosition();
        const int sum_dist_from_center = other_position_component.GetRadius() + position_component->GetRadius();
        const int dist = Math::Abs(vector_to_other.Length() - sum_dist_from_center);

        // Check if distance equal or close
        if (Math::Abs(closest_dist - dist) <= kAngleFocusHexTolerance)
        {
            const int test_angle = position_component->AngleToPosition(other_position_component);
            int reference_angle = kAngleRedFacingBlue;

            // Update reference angle for other side
            if (!IsInBlueGridSpace(other_position_component.GetPosition()))
            {
                reference_angle = kAngleBlueFacingRed;
            }

            // Compare both angles to reference angle
            if (Math::Abs(reference_angle - test_angle) < Math::Abs(reference_angle - closest_angle))
            {
                closest_angle = test_angle;
                closest_entity_id = other_entity_id;

                // Don't check closer
                continue;
            }
        }

        // Check if closest
        if (dist < closest_dist)
        {
            closest_dist = dist;
            closest_angle = position_component->AngleToPosition(other_position_component);
            closest_entity_id = other_entity_id;
        }
    }

    return closest_entity_id;
}

}  // namespace simulation
