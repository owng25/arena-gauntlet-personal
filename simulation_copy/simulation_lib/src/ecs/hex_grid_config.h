#pragma once

#include "data/enums.h"
#include "utility/hex_grid_position.h"

namespace simulation
{

// Hex grid configuration alongside with some cached values.
// These methods are used a lot so their implementation is in the header for better inlining
class HexGridConfig
{
public:
    HexGridConfig() = default;
    explicit HexGridConfig(const int grid_width, const int grid_height)
    {
        // Grid width - used for the column (x coordinate)
        grid_width_ = grid_width;

        // Grid height - used for the row (y coordinate)
        grid_height_ = grid_height;

        // Grid width extent - how far from the center line the grid width extends
        map_rectangle_width_extent_ = (GetGridWidth() - 1) / 2;

        // Grid height extent - how far from the center line the grid height extends
        map_rectangle_height_extent_ = (GetGridHeight() - 1) / 2;

        // The Hex position with the lowest possible q, r value
        min_hex_grid_position_ =
            HexGridPosition{MapRectangleQLimitMin(MapRectangleRLimitMin()), MapRectangleRLimitMin()};

        // The Hex position with the highest possible q, r value
        max_hex_grid_position_ =
            HexGridPosition{MapRectangleQLimitMax(MapRectangleRLimitMax()), MapRectangleRLimitMax()};

        // The maximum hex grid distance possible on the board between the lowest possible position and max possible
        // position max_hex_grid_position_ - min_hex_grid_position_
        max_hex_grid_distance_units_ = (max_hex_grid_position_ - min_hex_grid_position_).Length();

        hex_grid_corner_positions_[0] = min_hex_grid_position_;

        // The Hex position with the highest possible q value and lowest possible r value
        hex_grid_corner_positions_[1] =
            HexGridPosition(MapRectangleQLimitMax(MapRectangleRLimitMin()), MapRectangleRLimitMin());

        hex_grid_corner_positions_[2] = max_hex_grid_position_;

        // The Hex position with the lowest possible q value and highest possible r value
        hex_grid_corner_positions_[3] =
            HexGridPosition(MapRectangleQLimitMin(MapRectangleRLimitMax()), MapRectangleRLimitMax());
    }

    HexGridConfig(const HexGridConfig&) = default;
    HexGridConfig& operator=(const HexGridConfig&) = default;

    // Grid height - used for the row (y coordinate)
    constexpr int GetGridHeight() const
    {
        return grid_height_;
    }

    // Grid width - used for the column (x coordinate)
    constexpr int GetGridWidth() const
    {
        return grid_width_;
    }

    // Grid height extent - how far from the center line the grid height extends
    constexpr int GetRectangleHeightExtent() const
    {
        return map_rectangle_height_extent_;
    }

    // Grid width extent - how far from the center line the grid width extends
    constexpr int GetRectangleWidthExtent() const
    {
        return map_rectangle_width_extent_;
    }

    // Minimum/Maximum value of q, r for a rectangle map shape
    // NOTE: Assumes the shape of the map is a square. (See kGridExtent)
    // References:
    // - https://www.redblobgames.com/grids/hexagons/implementation.html#shape-rectangle
    constexpr int MapRectangleQLimitMin(const int r) const
    {
        return HexGridPosition::RectangleQLimitMin(-GetRectangleWidthExtent(), r);
    }

    constexpr int MapRectangleQLimitMax(const int r) const
    {
        return HexGridPosition::RectangleQLimitMax(GetRectangleWidthExtent(), r);
    }

    constexpr int MapRectangleRLimitMin() const
    {
        return HexGridPosition::RectangleRLimitMin(-GetRectangleHeightExtent());
    }

    constexpr int MapRectangleRLimitMax() const
    {
        return HexGridPosition::RectangleRLimitMax(GetRectangleHeightExtent());
    }

    // Check if coordinates are in the rectangle grid limits (with some margins for columns/rows if specified)
    constexpr bool IsInMapRectangleLimits(
        const HexGridPosition& hex_grid_position,
        const int q_margin = 0,
        const int r_margin = 0) const
    {
        const int r = hex_grid_position.r;
        const int q = hex_grid_position.q;

        return r >= MapRectangleRLimitMin() + r_margin && r <= MapRectangleRLimitMax() - r_margin &&
               q >= MapRectangleQLimitMin(r) + q_margin && q <= MapRectangleQLimitMax(r) - q_margin;
    }

    // Check if coordinates are in the rectangle grid limits or within specified units outside
    constexpr bool IsInMapRectangleLimitsWithMargin(const HexGridPosition& hex_grid_position, const int margin_units)
        const
    {
        const int r = hex_grid_position.r;
        const int q = hex_grid_position.q;

        return r >= HexGridPosition::RectangleRLimitMin(-GetRectangleHeightExtent() - margin_units) &&
               r <= HexGridPosition::RectangleRLimitMax(GetRectangleHeightExtent() + margin_units) &&
               q >= HexGridPosition::RectangleQLimitMin(-GetRectangleWidthExtent() - margin_units, r) &&
               q <= HexGridPosition::RectangleQLimitMax(GetRectangleWidthExtent() + margin_units, r);
    }

    // Get the farthest distance to any hex in the map rectangle
    // In practice, this amounts to picking the maximum distance to one of the four corners
    constexpr int GetDistanceToFarthestHexInMapRectangle(const HexGridPosition& hex_grid_position) const
    {
        int farthest_distance = (hex_grid_position - hex_grid_corner_positions_[0]).Length();

        // Start at element 1 because element 0 doesn't need to use the max operation.
        for (size_t i = 1; i < hex_grid_corner_positions_.size(); ++i)
        {
            farthest_distance =
                (std::max)(farthest_distance, (hex_grid_position - hex_grid_corner_positions_[i]).Length());
        }

        return farthest_distance;
    }

    // The Hex position with the lowest possible q, r value
    constexpr const HexGridPosition& GetMinHexGridPosition() const
    {
        return min_hex_grid_position_;
    }

    // The Hex position with the highest possible q, r value
    constexpr const HexGridPosition& GetMaxHexGridPosition() const
    {
        return max_hex_grid_position_;
    }

    // Get maximum hex grid distance possible on the board between max and min
    // This is just (GetMaxHexGridPosition() - GetMinHexGridPosition()).Distance()
    constexpr int GetMaxHexGridDistanceUnits() const
    {
        return max_hex_grid_distance_units_;
    }

    // Grid size (height x width)
    constexpr size_t GetGridSize() const
    {
        return static_cast<size_t>(GetGridWidth() * GetGridHeight());
    }

    // Gets a grid index from row and column specified
    // Ensure input to this method is valid before using
    // row - r
    // column - q
    constexpr size_t GetGridIndex(const HexGridPosition& position) const
    {
        if (!IsInMapRectangleLimits(position))
        {
            // assert(false);
            return kInvalidIndex;
        }

        return GetGridIndexUnsafe(position);
    }

    // Unsafe version when position is proven to be in grid limits
    constexpr size_t GetGridIndexUnsafe(const HexGridPosition& position) const
    {
        // x - col
        // y - row
        const IVector2D offset_odd_r = position.ToOffSetOddR();

        // We add kGridExtent so that the numbers are always positive (correct index)
        const int column = offset_odd_r.x + GetRectangleWidthExtent();
        const int row = offset_odd_r.y + GetRectangleHeightExtent();

        // r * kGridWidth + q
        return static_cast<size_t>((row * GetGridWidth()) + column);
    }

    // Get hex grid coordinates from a grid index (returned from GetGridIndex)
    constexpr HexGridPosition GetCoordinates(const size_t index) const
    {
        if (index >= GetGridSize())
        {
            // assert(false);
            return kInvalidHexHexGridPosition;
        }

        // Get row/column from index
        const int index_as_int = static_cast<int>(index);
        const int row = index_as_int / GetGridWidth();
        const int column = index_as_int % GetGridWidth();

        // Convert back from index to offset_odd_r form
        const IVector2D offset_odd_r = IVector2D{column - GetRectangleWidthExtent(), row - GetRectangleHeightExtent()};

        // Convert to axial
        return HexGridPosition::FromOffsetOddR(offset_odd_r);
    }

    // Is this hexagon position in the limits of the grid (with some margins for columns/rows if specified)
    constexpr bool IsHexagonInGridLimits(
        const HexGridPosition& center,
        const int radius_units,
        const int q_margin = 0,
        const int r_margin = 0) const
    {
        // Center is not in grid limits
        if (!IsInMapRectangleLimits(center, q_margin, r_margin))
        {
            return false;
        }

        // Find if edges of the hexagon are in the map limits
        if (radius_units != 0)
        {
            constexpr size_t neighbours_offsets_size = kHexGridNeighboursOffsets.size();
            for (size_t i = 0; i < neighbours_offsets_size; i++)
            {
                const HexGridPosition& neighbour_position = kHexGridNeighboursOffsets[i];

                // Shift by center
                const HexGridPosition edge_position = center + neighbour_position * radius_units;
                if (!IsInMapRectangleLimits(edge_position, q_margin, r_margin))
                {
                    return false;
                }
            }
        }

        return true;
    }

    // Converts PredefinedGridPosition enumeration to actual coordinates
    HexGridPosition ResolvePredefinedPosition(const Team ally_team, const PredefinedGridPosition predefined_position)
        const;

private:
    // Grid width - used for the column (x coordinate)
    int grid_width_ = 0;

    // Grid height - used for the row (y coordinate)
    int grid_height_ = 0;

    // Grid height extent - how far from the center line the grid height extends
    int map_rectangle_height_extent_ = 0;

    // Grid width extent - how far from the center line the grid width extends
    int map_rectangle_width_extent_ = 0;

    // The maximum hex grid distance possible on the board between the lowest possible position and max possible
    // position max_hex_grid_position_ - min_hex_grid_position_
    int max_hex_grid_distance_units_ = 0;

    // The Hex position with the lowest possible q, r value
    HexGridPosition min_hex_grid_position_{};

    // The Hex position with the highest possible q, r value
    HexGridPosition max_hex_grid_position_{};

    // Hex positions of the four corners of the hex grid
    std::array<HexGridPosition, 4> hex_grid_corner_positions_{};
};
}  // namespace simulation
