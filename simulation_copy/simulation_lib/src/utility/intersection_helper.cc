#include "intersection_helper.h"

#include "ecs/world.h"
#include "utility/math.h"

namespace simulation
{

static int64_t TriangleArea(const IVector2D a, const IVector2D b, const IVector2D c)
{
    return Math::TriangleArea(a.x, a.y, b.x, b.y, c.x, c.y);
};

bool IntersectionHelper::DoesHexZoneIntersectEntity(
    const int zone_radius_units,
    const HexGridPosition zone_position,
    const HexGridPosition other_entity_position)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const HexGridPosition distance_vector = zone_position - other_entity_position;
    const int distance_units = distance_vector.Length();
    return distance_units <= zone_radius_units;
}

TriangleZoneIntersectionCache IntersectionHelper::MakeTriangleIntersectionCache(
    const World& world,
    const HexGridPosition zone_position,
    const int direction_degrees,
    const int radius_units)
{
    ILLUVIUM_PROFILE_FUNCTION();

    TriangleZoneIntersectionCache cache;

    const IVector2D world_source_sub_units = world.ToWorldPosition(zone_position).ToSubUnits();
    const int world_radius = world.ToWorldPosition(Math::UnitsToSubUnits(radius_units));

    // Calculate vertices for triangle ABC with A at source
    world_source_sub_units
        .GetTriangleVertices(direction_degrees, world_radius, &cache.triangle_a, &cache.triangle_b, &cache.triangle_c);

    // Calculate area of the triangle ABC
    cache.area_triangle_abc = TriangleArea(cache.triangle_a, cache.triangle_b, cache.triangle_c);
    return cache;
}

bool IntersectionHelper::DoesTriangleZoneIntersectEntity(
    const TriangleZoneIntersectionCache& cache,
    const World& world,
    const HexGridPosition other_entity_position_units)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // World positions based on centre point of current hex
    const IVector2D world_other_position_sub_units = world.ToWorldPosition(other_entity_position_units).ToSubUnits();

    // Calculate areas to determine if entity is inside triangle
    const int64_t area_triangle_pbc = TriangleArea(world_other_position_sub_units, cache.triangle_b, cache.triangle_c);
    const int64_t area_triangle_pac = TriangleArea(cache.triangle_a, world_other_position_sub_units, cache.triangle_c);
    const int64_t area_triangle_pab = TriangleArea(cache.triangle_a, cache.triangle_b, world_other_position_sub_units);

    // Check if sum of other triangles add up to triangle ABC
    return cache.area_triangle_abc == area_triangle_pbc + area_triangle_pac + area_triangle_pab;
}

bool IntersectionHelper::DoesTriangleZoneIntersectEntity(
    const World& world,
    const HexGridPosition zone_position_units,
    const int direction_degrees,
    const int radius_units,
    const HexGridPosition other_entity_position_units)
{
    const TriangleZoneIntersectionCache cache =
        MakeTriangleIntersectionCache(world, zone_position_units, direction_degrees, radius_units);
    return DoesTriangleZoneIntersectEntity(cache, world, other_entity_position_units);
}

bool IntersectionHelper::DoesRectangleZoneIntersectEntity(
    const HexGridPosition zone_position_sub_units,
    const HexGridPosition other_entity_position_sub_units,
    const int zone_width_sub_units,
    const int zone_height_sub_units)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Get individual distances along each axis
    const int distance_q = Math::Abs(other_entity_position_sub_units.q - zone_position_sub_units.q);
    const int distance_r = Math::Abs(other_entity_position_sub_units.r - zone_position_sub_units.r);

    // Check rectangle collision
    // Source in center, so check against half values
    return distance_q <= zone_width_sub_units / 2 && distance_r <= zone_height_sub_units / 2;
}

BeamIntersectionCache IntersectionHelper::MakeBeamIntersectionCache(
    const World& world,
    const HexGridPosition beam_position_units,
    const int direction_degrees,
    const int width_sub_units,
    const int world_length_sub_units)
{
    // Get remaining beam details
    // NOTE: All the units here are defined/converted in world space because otherwise we can't do
    // atan2
    const int world_length_sub_units_half = world_length_sub_units / 2;
    const int world_width_sub_units = world.ToWorldPosition(width_sub_units);
    const int world_width_sub_units_half = world_width_sub_units / 2;
    const IVector2D position_rotated_sub_units =
        world.ToWorldPosition(beam_position_units).Rotate(-direction_degrees).ToSubUnits();
    const IVector2D beam_center_rotated_sub_units =
        position_rotated_sub_units + IVector2D(world_length_sub_units_half, 0);

    BeamIntersectionCache cache;
    cache.direction_degrees = direction_degrees;
    cache.beam_center_rotated_sub_units = beam_center_rotated_sub_units;
    cache.world_length_sub_units_half = world_length_sub_units_half;
    cache.world_width_sub_units_half = world_width_sub_units_half;

    return cache;
}

bool IntersectionHelper::DoesBeamIntersectEntity(
    const World& world,
    const BeamIntersectionCache& cache,
    const HexGridPosition other_entity_position_units,
    const int other_entity_radius_units)
{
    // Rotate the point to align with the axis
    const IVector2D other_position_rotated_sub_units =
        world.ToWorldPosition(other_entity_position_units).Rotate(-cache.direction_degrees).ToSubUnits();
    const IVector2D distance_rotated_sub_units = other_position_rotated_sub_units - cache.beam_center_rotated_sub_units;

    // Consider size of entity
    const int distance_from_center_sub_units = Math::UnitsToSubUnits(other_entity_radius_units);

    // Get distances between constructed points
    const int distance_x = Math::Abs(distance_rotated_sub_units.x) - distance_from_center_sub_units;
    const int distance_y = Math::Abs(distance_rotated_sub_units.y) - distance_from_center_sub_units;

    return distance_x <= cache.world_length_sub_units_half && distance_y <= cache.world_width_sub_units_half;
}

bool IntersectionHelper::DoesBeamIntersectEntity(
    const World& world,
    const HexGridPosition beam_position_units,
    const int direction_degrees,
    const int width_sub_units,
    const int world_length_sub_units,
    const HexGridPosition other_entity_position_units,
    const int other_entity_radius_units)
{
    const BeamIntersectionCache cache = MakeBeamIntersectionCache(
        world,
        beam_position_units,
        direction_degrees,
        width_sub_units,
        world_length_sub_units);

    return DoesBeamIntersectEntity(world, cache, other_entity_position_units, other_entity_radius_units);
}
}  // namespace simulation
