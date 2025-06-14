#pragma once

#include "utility/hex_grid_position.h"
#include "utility/ivector2d.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * IntersectionHelper
 *
 * This defines the helper functions to apply hyper to combat units
 * --------------------------------------------------------------------------------------------------------
 */

class World;

// Some cached values for one specific zone
// For cases when you want to test for intersection of this zone with a few entities
struct TriangleZoneIntersectionCache
{
    IVector2D triangle_a{};
    IVector2D triangle_b{};
    IVector2D triangle_c{};
    int64_t area_triangle_abc = 0;
};

// Some cached values for one specific beam
// For cases when you want to test for intersection of this beam with a few entities
struct BeamIntersectionCache
{
    int direction_degrees = 0;
    IVector2D beam_center_rotated_sub_units{};
    int world_length_sub_units_half = 0;
    int world_width_sub_units_half = 0;
};

class IntersectionHelper
{
public:
    /// Hexagonal zone

    static bool DoesHexZoneIntersectEntity(
        const int zone_radius_units,
        const HexGridPosition zone_position,
        const HexGridPosition other_entity_position);

    /// Triangle zone

    static TriangleZoneIntersectionCache MakeTriangleIntersectionCache(
        const World& world,
        const HexGridPosition zone_position_units,
        const int direction_degrees,
        const int radius_units);

    static bool DoesTriangleZoneIntersectEntity(
        const TriangleZoneIntersectionCache& cache,
        const World& world,
        const HexGridPosition other_entity_position_units);

    static bool DoesTriangleZoneIntersectEntity(
        const World& world,
        const HexGridPosition zone_position_units,
        const int direction_degrees,
        const int radius_units,
        const HexGridPosition other_entity_position_units);

    /// Rectangle zone

    static bool DoesRectangleZoneIntersectEntity(
        const HexGridPosition zone_position_sub_units,
        const HexGridPosition other_entity_position_sub_units,
        const int zone_width_sub_units,
        const int zone_height_sub_units);

    /// Beam

    static BeamIntersectionCache MakeBeamIntersectionCache(
        const World& world,
        const HexGridPosition beam_position_units,
        const int direction_degrees,
        const int width_sub_units,
        const int world_length_sub_units);

    static bool DoesBeamIntersectEntity(
        const World& world,
        const BeamIntersectionCache& cache,
        const HexGridPosition other_entity_position_units,
        const int other_entity_radius_units);

    static bool DoesBeamIntersectEntity(
        const World& world,
        const HexGridPosition beam_position_units,
        const int direction_degrees,
        const int width_sub_units,
        const int world_length_sub_units,
        const HexGridPosition other_entity_position_units,
        const int other_entity_radius_units);
};

}  // namespace simulation
