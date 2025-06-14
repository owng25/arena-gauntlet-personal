#pragma once

// utility class for performing vector operations using a deterministic integer representation.

#include "data/constants.h"
#include "nlohmann/json.hpp"
#include "utility/custom_formatter.h"
#include "utility/ivector2d.h"
#include "utility/math.h"

namespace simulation
{
// Specifies the grid cardinal direction
// NOTE: that the hexes are pointy topped
// References: https://www.redblobgames.com/grids/hexagons/#neighbors-axial
enum class HexGridCardinalDirection : int
{
    kNone = 0,

    kRight,
    kTopRight,
    kTopLeft,
    kLeft,
    kBottomLeft,
    kBottomRight,

    kNum
};

// Defines limits of the grid on one axis
struct GridLimit
{
    // Is this value in the limit
    constexpr bool IsInLimit(const int value) const
    {
        return value >= min && value <= max;
    }

    int min = 0;
    int max = 0;
};

// Struct representing a position in grid space in a hex grid using the axial system of
// representation.
// The shape of the map is a square rectangle.
// TODO(vampy): Add support for a hex map shape
// More Info here:
// https://illuvium.atlassian.net/wiki/spaces/TECHNOLOGY/pages/206013000/Hex+System
//
struct HexGridPosition final
{
    constexpr HexGridPosition() = default;
    constexpr HexGridPosition(const int in_q, const int in_r) : q(in_q), r(in_r) {}

    // Copyable and movable
    constexpr HexGridPosition(const HexGridPosition& other) = default;
    constexpr HexGridPosition(HexGridPosition&&) = default;
    constexpr HexGridPosition& operator=(const HexGridPosition& other) = default;
    constexpr HexGridPosition& operator=(HexGridPosition&&) = default;

    constexpr bool operator==(const HexGridPosition& other) const
    {
        return q == other.q && r == other.r;
    }
    constexpr bool operator!=(const HexGridPosition& other) const
    {
        return !(*this == other);
    }
    constexpr HexGridPosition& operator*=(const int other)
    {
        q *= other;
        r *= other;
        return *this;
    }
    constexpr HexGridPosition& operator+=(const HexGridPosition& other)
    {
        q += other.q;
        r += other.r;
        return *this;
    }
    constexpr HexGridPosition& operator-=(const HexGridPosition& other)
    {
        q -= other.q;
        r -= other.r;
        return *this;
    }

    // Compare HexGridPosition instances so they can be used in sorted containers
    constexpr bool operator<(const HexGridPosition& other) const
    {
        // Ordered only up to a grid size of kPrecisionFactor x kPrecisionFactor
        return (r * kPrecisionFactor + q) < (other.r * kPrecisionFactor + other.q);
    }

    // Conversion functions from axial to offset coordinates in odd-r form
    // References:
    // - https://www.redblobgames.com/grids/hexagons/#conversions-offset
    constexpr IVector2D ToOffSetOddR() const
    {
        // x - col
        // y - row
        // TODO(vampy): Replace this with r >> 1 ? This seems to be just be doing floor(x / 2.0)
        const int col = q + (r - (r & 1)) / 2;
        const int row = r;
        return IVector2D{col, row};
    }

    // Conversion functions from offset coordinates in odd-r form to axial
    // References:
    // - https://www.redblobgames.com/grids/hexagons/#conversions-offset
    static constexpr HexGridPosition FromOffsetOddR(const IVector2D& position)
    {
        // x - col
        // y - row
        const int col = position.x;
        const int row = position.y;

        const int q = col - (row - (row & 1)) / 2;
        const int r = row;
        return HexGridPosition(q, r);
    }

    // Minimum/Maximum value for q, r to form a rectangle given boundaries
    // References:
    // - https://www.redblobgames.com/grids/hexagons/implementation.html#shape-rectangle
    static constexpr int RectangleRLimitMin(const int top)
    {
        return top;
    }
    static constexpr int RectangleRLimitMax(const int bottom)
    {
        return bottom;
    }
    static constexpr int RectangleQLimitMin(const int left, const int r)
    {
        // Convert r from axial to odd-r offset
        const int r_offset = r >> 1;
        return left - r_offset;
    }
    static constexpr int RectangleQLimitMax(const int right, const int r)
    {
        // Convert r from axial to odd-r offset
        const int r_offset = r >> 1;
        return right - r_offset;
    }

    // Minimum/Maximum value for q, r to form an hexagon given a radius
    // References:
    // - https://www.redblobgames.com/grids/hexagons/implementation.html#shape-hexagon
    // - https://www.redblobgames.com/grids/hexagons/#range
    static constexpr int HexagonQLimitMin(const int radius)
    {
        return -radius;
    }
    static constexpr int HexagonQLimitMax(const int radius)
    {
        return radius;
    }
    static constexpr int HexagonRLimitMin(const int radius, const int q)
    {
        return (std::max)(-radius, -q - radius);
    }
    static constexpr int HexagonRLimitMax(const int radius, const int q)
    {
        return (std::min)(radius, -q + radius);
    }
    static constexpr GridLimit HexagonQLimits(const int radius)
    {
        return GridLimit{HexagonQLimitMin(radius), HexagonQLimitMax(radius)};
    }
    static constexpr GridLimit HexagonRLimits(const int radius, const int q)
    {
        return GridLimit{HexagonRLimitMin(radius, q), HexagonRLimitMax(radius, q)};
    }

    // Reflect the position (both axes)
    // References:
    // - https://www.redblobgames.com/grids/hexagons/#reflection
    constexpr HexGridPosition Reflect() const
    {
        return HexGridPosition(-q, -r);
    }

    // Return this unit position converted to absolute subunit position
    // Opposite of ToUnits
    constexpr HexGridPosition ToSubUnits() const
    {
        return HexGridPosition{q * kSubUnitsPerUnit, r * kSubUnitsPerUnit};
    }

    // Return this absolute subunit position converted to unit position withing a grid
    // Opposite of ToSubUnits
    constexpr HexGridPosition ToUnits() const
    {
        return HexGridPosition{q / kSubUnitsPerUnit, r / kSubUnitsPerUnit};
    }

    // Returns the this absolute subunits position converted to the remainder of the conversion to
    // units
    constexpr HexGridPosition ToSubUnitsRemainder() const
    {
        return HexGridPosition{q % kSubUnitsPerUnit, r % kSubUnitsPerUnit};
    }

    // Get distance from grid position vector
    // References: https://www.redblobgames.com/grids/hexagons/#distances
    constexpr int Length() const
    {
        return (Math::Abs(q) + Math::Abs(r) + Math::Abs(s())) / 2;
    }

    // Is this the null vector (length 0)
    // NOTE: q = 0, r = 0 is a valid position, it represents the center of the grid
    constexpr bool IsNull() const
    {
        return q == 0 && r == 0;
    }

    // Convert this to a normalized and scaled HexGridPosition by kPrecisionFactor
    constexpr HexGridPosition ToNormalizedAndScaled() const
    {
        const int length = Length();

        // Prevent division by zero
        if (length == 0)
        {
            return {0, 0};
        }

        const int q_normal = (q * kPrecisionFactor) / length;
        const int r_normal = (r * kPrecisionFactor) / length;
        return HexGridPosition{q_normal, r_normal};
    }

    void FormatTo(fmt::format_context& ctx) const;

    std::string ToString() const;
    nlohmann::json ToJSONObject() const;

    // Round position to nearest hex
    static constexpr void
    Round(const HexGridPosition& scaled_position, HexGridPosition* out_units, HexGridPosition* out_sub_units)
    {
        // Default round is using not inclusive comparison
        constexpr bool round_inclusive = false;
        RoundExtended(scaled_position, round_inclusive, out_units, out_sub_units);
    }

    // Round position to nearest hex, extended version
    // Inclusivity of comparison can be specified
    static constexpr void RoundExtended(
        const HexGridPosition& scaled_position,
        bool round_inclusive,
        HexGridPosition* out_units,
        HexGridPosition* out_sub_units)
    {
        static_assert(
            kPrecisionFactor == kSubUnitsPerUnit,
            "For this code to work the precision factor must be the same as kSubUnitsPerUnit");

        // References:
        // - https://www.redblobgames.com/grids/hexagons/#rounding
        // - https://www.redblobgames.com/grids/hexagons/implementation.html#rounding

        // Calculate s coordinate for cube
        const int scaled_q = scaled_position.q;
        const int scaled_r = scaled_position.r;
        const int scaled_s = scaled_position.s();

        // Round to nearest integers
        int q = Math::FractionalRound(scaled_q, kPrecisionFactor);
        int r = Math::FractionalRound(scaled_r, kPrecisionFactor);
        const int s = Math::FractionalRound(scaled_s, kPrecisionFactor);

        // Calculate differences
        const int q_diff = q * kPrecisionFactor - scaled_q;
        const int r_diff = r * kPrecisionFactor - scaled_r;
        const int s_diff = s * kPrecisionFactor - scaled_s;

        // Calculate differences
        const int q_diff_abs = Math::Abs(q_diff);
        const int r_diff_abs = Math::Abs(r_diff);
        const int s_diff_abs = Math::Abs(s_diff);

        // Update based on largest difference
        if (round_inclusive)
        {
            if (q_diff_abs >= r_diff_abs && q_diff_abs >= s_diff_abs)
            {
                q = -r - s;
            }
            else if (r_diff_abs >= s_diff_abs)
            {
                r = -q - s;
            }
        }
        else
        {
            if (q_diff_abs > r_diff_abs && q_diff_abs > s_diff_abs)
            {
                q = -r - s;
            }
            else if (r_diff_abs > s_diff_abs)
            {
                r = -q - s;
            }
        }

        // Store results
        *out_units = HexGridPosition{q, r};
        *out_sub_units = HexGridPosition{-q_diff, -r_diff};
    }

    constexpr int s() const
    {
        return -q - r;
    }
    int q = 0;
    int r = 0;
};

// Out of class member operators
constexpr HexGridPosition operator+(const HexGridPosition& lhs, const HexGridPosition& rhs)
{
    return HexGridPosition{lhs.q + rhs.q, lhs.r + rhs.r};
}
constexpr HexGridPosition operator-(const HexGridPosition& lhs, const HexGridPosition& rhs)
{
    return HexGridPosition{lhs.q - rhs.q, lhs.r - rhs.r};
}
constexpr HexGridPosition operator-(const HexGridPosition& lhs, const int rhs)
{
    return HexGridPosition{lhs.q - rhs, lhs.r - rhs};
}
constexpr HexGridPosition operator/(const HexGridPosition& lhs, const int rhs)
{
    return HexGridPosition{lhs.q / rhs, lhs.r / rhs};
}
constexpr HexGridPosition operator%(const HexGridPosition& lhs, const int rhs)
{
    return HexGridPosition{lhs.q % rhs, lhs.r % rhs};
}
constexpr HexGridPosition operator*(const HexGridPosition& lhs, const int rhs)
{
    return HexGridPosition{lhs.q * rhs, lhs.r * rhs};
}

// clang-format off
// Axial Offsets between grid neighbours - going counterclockwise from the right (and pointy topped)
// Reference: https://www.redblobgames.com/grids/hexagons/#neighbors-axial
static constexpr std::array<HexGridPosition, 6> kHexGridNeighboursOffsets = {
    HexGridPosition{+1, 0}, HexGridPosition{+1, -1}, HexGridPosition{0, -1},
    HexGridPosition{-1, 0}, HexGridPosition{-1, +1}, HexGridPosition{0, +1}
};
// clang-format on

// The invalid hex grid position
static constexpr HexGridPosition kInvalidHexHexGridPosition{kInvalidPosition, kInvalidPosition};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(HexGridPosition, FormatTo);
