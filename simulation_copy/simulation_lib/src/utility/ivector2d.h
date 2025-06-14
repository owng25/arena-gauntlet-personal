#pragma once
// utility class for performing vector operations using a deterministic integer representation.

#include "data/constants.h"
#include "utility/custom_formatter.h"
#include "utility/math.h"

namespace simulation
{
// 2D vector struct that helps up deal with high precision number calculations
// Also used to store the unit and subunit position of the grid
struct IVector2D final
{
    constexpr IVector2D() = default;
    constexpr IVector2D(const int in_x, const int in_y) : x(in_x), y(in_y) {}

    // Copyable and movable
    constexpr IVector2D(const IVector2D& other) = default;
    constexpr IVector2D(IVector2D&&) = default;
    constexpr IVector2D& operator=(const IVector2D& other) = default;
    constexpr IVector2D& operator=(IVector2D&&) = default;

    constexpr int Dot(const IVector2D& r) const
    {
        return x * r.x + y * r.y;
    }

    constexpr bool operator==(const IVector2D& other) const
    {
        return x == other.x && y == other.y;
    }
    constexpr bool operator!=(const IVector2D& other) const
    {
        return !(*this == other);
    }
    constexpr IVector2D& operator+=(const IVector2D& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr IVector2D& operator-=(const IVector2D& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    // Compare IVector2D instances so they can be used in sorted containers
    constexpr bool operator<(const IVector2D& other) const
    {
        // Ordered only up to a grid size of kPrecisionFactor x kPrecisionFactor
        return (y * kPrecisionFactor + x) < (other.y * kPrecisionFactor + other.x);
    }

    // Create a new rotated version of this IVector2D
    constexpr IVector2D Rotate(const int angle) const
    {
        return IVector2D{Math::RotatePointX(x, y, angle), Math::RotatePointY(x, y, angle)};
    }

    // Get the SquareInt64 Magnitude of this vector
    constexpr int64_t SquareMagnitude() const
    {
        return Math::Square(x) + Math::Square(y);
    }

    // Alias for SquareMagnitude
    constexpr int64_t SquareLength() const
    {
        return SquareMagnitude();
    }

    // Get the Magnitude of the vector
    constexpr int Magnitude() const
    {
        return static_cast<int>(Math::Sqrt(SquareMagnitude()));
    }

    // Alias for Magnitude
    constexpr int Length() const
    {
        return Magnitude();
    }

    // Is this the null vector (length 0)
    constexpr bool IsNull() const
    {
        return x == 0 && y == 0;
    }

    // Is this the unit vector (length 1)
    constexpr bool IsUnit() const
    {
        return Magnitude() == 1;
    }

    // Return this unit position converted to absolute subnit position
    // Opposite of ToUnits
    constexpr IVector2D ToSubUnits() const
    {
        return IVector2D{x * kSubUnitsPerUnit, y * kSubUnitsPerUnit};
    }

    // Return this absolute subunit position converted to unit position withing a grid
    // Opposite of ToSubUnits
    constexpr IVector2D ToUnits() const
    {
        return IVector2D{x / kSubUnitsPerUnit, y / kSubUnitsPerUnit};
    }

    // Returns the this absolute subunits position converted to the remainder of the conversion to
    // units
    constexpr IVector2D ToSubUnitsRemainder() const
    {
        return IVector2D{x % kSubUnitsPerUnit, y % kSubUnitsPerUnit};
    }

    // Gets the angle between this vector's position and another position, going counter-clockwise
    // from where x > 0 and y = 0
    // Return degrees in the range [0, 360)
    int AngleToPosition(const IVector2D& other) const
    {
        return Math::RadianTurnsToDegrees(RadiansTurnsBetween(other));
    }

    // Get the vertices of an Equilateral Triangle from the vector represented by this object, given direction and
    // length of median intersecting the vector
    //
    // As the triangle is an Equilateral Triangle, the median splits the triangle in half.
    //
    // Point A - out_a
    // Point B - out_b
    // Point C - out_c
    // AM - median_length
    // BM - side_half_length - half-length of a side (all sides are equal in size as this is an Equilateral triangle)
    //
    // tan 30 = BM / AM
    // tan PI / 6 = BM / AM
    // sqrt (3) / 3 = BM / AM
    //
    // BM = AM * sqrt(3) / 3
    void GetTriangleVertices(
        const int direction_degrees,
        const int median_length,
        IVector2D* out_a,
        IVector2D* out_b,
        IVector2D* out_c) const
    {
        const int side_half_length = median_length * kSqrt3Scaled / (kPrecisionFactor * 3);

        // Calculate the offsets from origin
        *out_b = IVector2D{median_length, -side_half_length}.Rotate(direction_degrees);
        *out_c = IVector2D{median_length, side_half_length}.Rotate(direction_degrees);

        // Output values
        *out_a = *this;
        *out_b += *out_a;
        *out_c += *out_a;
    }

    // Calculate the area of a triangle
    // See https://en.wikipedia.org/wiki/Triangle#Using_coordinates and
    // https://www.mathopenref.com/coordtrianglearea.html
    static constexpr int64_t TriangleArea(const IVector2D& a, const IVector2D& b, const IVector2D& c)
    {
        return Math::Abs((a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y)) / 2);
    }

    void FormatTo(fmt::format_context& ctx) const;

    int x = 0;
    int y = 0;

private:
    // Interval function to get the radian turns between this vector's position and another
    // This is the arctangent of the difference between the positions
    uint16_t RadiansTurnsBetween(const IVector2D& other) const
    {
        const int diff_x = other.x - x;
        const int diff_y = other.y - y;
        return Math::ATan2(static_cast<int16_t>(diff_y), static_cast<int16_t>(diff_x));
    }
};

// Out of class member operators
constexpr IVector2D operator+(const IVector2D& lhs, const IVector2D& rhs)
{
    return IVector2D{lhs.x + rhs.x, lhs.y + rhs.y};
}
constexpr IVector2D operator+(const IVector2D& lhs, const int rhs)
{
    return IVector2D{lhs.x + rhs, lhs.y + rhs};
}
constexpr IVector2D operator-(const IVector2D& lhs, const IVector2D& rhs)
{
    return IVector2D{lhs.x - rhs.x, lhs.y - rhs.y};
}
constexpr IVector2D operator-(const IVector2D& lhs, const int rhs)
{
    return IVector2D{lhs.x - rhs, lhs.y - rhs};
}
constexpr IVector2D operator/(const IVector2D& lhs, const int rhs)
{
    return IVector2D{lhs.x / rhs, lhs.y / rhs};
}
constexpr IVector2D operator%(const IVector2D& lhs, const int rhs)
{
    return IVector2D{lhs.x % rhs, lhs.y % rhs};
}
constexpr IVector2D operator*(const IVector2D& lhs, const int rhs)
{
    return IVector2D{lhs.x * rhs, lhs.y * rhs};
}

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(IVector2D, FormatTo);
