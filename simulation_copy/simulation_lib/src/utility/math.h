#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "data/constants.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * Math
 *
 * This defines defines math functions working on integers
 * --------------------------------------------------------------------------------------------------------
 */
class Math
{
public:
    // Absolute value of n
    template <std::integral T>
    static constexpr T Abs(const T n)
    {
        return n < 0 ? -n : n;
    }

    // Std version is not constexpr
    template <std::integral T>
    static constexpr T Max(const T a, const T b)
    {
        return a > b ? a : b;
    }

    // Calculates the percentage of a value in an integer safe manner.
    //
    // Calculation:
    // PercentageOf (with floats) = percentage * value
    // percentage = percentage / kMaxPercentage (because percentages are scaled by this constant)
    // => PercentageOf (with ints) = (percentage * value) / kMaxPercentage
    //
    // Example:
    // As we can't use floats, 25% (0.25) will represented as 25.
    // - 25% of 1000
    // 25 * 1000 / 100 = 250
    static constexpr int PercentageOf(const int percentage, const int value)
    {
        const int64_t numerator = percentage * static_cast<int64_t>(value);
        return static_cast<int>(numerator / kMaxPercentage);
    }

    // Calculate energy gain depends on attack speed
    static constexpr FixedPoint CalculateEnergyGain(
        const FixedPoint base_energy_gain,
        const FixedPoint type_data_attack_speed)
    {
        // Prevent division by zero
        if (type_data_attack_speed == 0_fp)
        {
            return 0_fp;
        }

        return base_energy_gain * kMaxPercentageFP / type_data_attack_speed;
    }

    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/236783470/Overload
    static constexpr FixedPoint CalculateOverloadDamage(
        const FixedPoint overload_damage_percentage,
        const FixedPoint live_max_health,
        const int alive_team_members_count)
    {
        if (alive_team_members_count <= 0)
        {
            return 0_fp;
        }
        const FixedPoint overload_damage =
            overload_damage_percentage.AsPercentageOf(live_max_health) / FixedPoint::FromInt(alive_team_members_count);
        return overload_damage;
    }

    // Returns true if narrowing conversion will overflow the limits of resulting type
    template <typename To, typename From>
    static constexpr bool CastWillOverflow(From v)
    {
        static_assert(std::is_integral_v<To> && std::is_integral_v<From>);
        static_assert(!(std::is_signed_v<From> ^ std::is_signed_v<To>), "Both types must be either signed or unsigned");

        if constexpr (sizeof(To) >= sizeof(From))
        {
            return false;
        }
        else
        {
            // Check max value overflow
            if (v > static_cast<From>((std::numeric_limits<To>::max)()))
            {
                return true;
            }

            // Also check min value overflow (for signed types only)
            if constexpr (std::is_signed_v<To>)
            {
                if (v < static_cast<From>(std::numeric_limits<To>::lowest()))
                {
                    return true;
                }
            }

            return false;
        }
    }

    // Calculates how much energy combat unit gets for received damage
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/72321254/Energy+Calculation
    //   - health_lost is the amount of Health lost from the Effect Package after being mitigated by everything, and
    //   then destroying all shields
    //   - post_mitigated_damage is the total value of damage after it has been reduced via damage mitigators
    //   - pre_mitigated_damage is the total value of the damage before it has been reduced via damage mitigators
    static constexpr FixedPoint CalculateEnergyGainOnTakeDamage(
        const FixedPoint health_lost,
        const FixedPoint pre_mitigated_damage,
        const FixedPoint post_mitigated_damage)
    {
        // prevent division by zero
        if (post_mitigated_damage == 0_fp)
        {
            return 0_fp;
        }

        return (health_lost * pre_mitigated_damage) / post_mitigated_damage;
    }

    // Calculates the final value of the damage after we apply the mitigation (energy resist for
    // example). The formula is like this one bellow, but we must make it integer safe. Post
    // Mitigation Damage = Base Damage * [100 / (100 + Mitigation)]
    static constexpr FixedPoint PostMitigationDamage(const FixedPoint damage, const FixedPoint mitigation_percentage)
    {
        return (damage * 100_fp) / (mitigation_percentage + 100_fp);
    }

    // Calculates the final value of the damage after we apply the vulnerability
    //
    // This formula is the one bellow but it is integer safe:
    // Damage Out (with floats) = Damage / Mitigation
    // Mitigation = Mitigation / kMaxPercentage (Because of the int scaling)
    // => Damage Out (with ints) = (Damage * Mitigation) / kMaxPercentage
    static constexpr FixedPoint VulnerabilityMitigation(
        const FixedPoint damage,
        const FixedPoint vulnerability_percentage)
    {
        return vulnerability_percentage.AsPercentageOf(damage);
    }

    template <typename T, typename Enable = std::enable_if_t<std::is_integral_v<T>>>
    static constexpr bool IsOdd(const T value)
    {
        return value % 2;
    }

    template <typename T, typename Enable = std::enable_if_t<std::is_integral_v<T>>>
    static constexpr bool IsEven(const T value)
    {
        return !IsOdd(value);
    }

    // A deterministic integer sqrt function.
    // Note we don't need a strictly correct integer sqrt ( a positive integer of n is the positive
    // integer m which is the greatest integer less than or equal to the square root of n). What we
    // do need is for it to be deterministic.
    //
    // n value to get the integer SQRT for.
    //
    // Returns The integer square root of n (approximation)
    static constexpr int64_t Sqrt(const int64_t n)
    {
        if (n < 0)
        {
            // Cannot calculate negative square root, return 0 as a workaround
            return 0;
        }

        // We shift bits below, so use unsigned values exclusively
        const uint64_t un = static_cast<uint64_t>(n);

        // https://en.wikipedia.org/wiki/Integer_square_root
        // https://rosettacode.org/wiki/Isqrt_(integer_square_root)_of_X#C.2B.2B
        uint64_t x0 = un >> 1;  // Initial estimate

        // Sanity check
        if (x0)
        {
            uint64_t x1 = (x0 + un / x0) >> 1;  // Update
            while (x1 < x0)                     // This also checks for cycle
            {
                x0 = x1;
                x1 = (x0 + un / x0) >> 1;
            }

            return static_cast<int64_t>(x0);
        }

        return n;
    }

    // Calculate the square of the number n
    static constexpr int64_t Square(const int64_t n)
    {
        return n * n;
    }

    // Returns x * x.
    // Useful when x is some expression and you don't want to make a seaprate variable to get square of it.
    template <typename T>
    static constexpr T Sqr(T x)
    {
        return x * x;
    }

    // Calculate the area of a triangle
    // See https://en.wikipedia.org/wiki/Triangle#Using_coordinates and
    // https://www.mathopenref.com/coordtrianglearea.html
    static constexpr int64_t TriangleArea(
        const int64_t x1,
        const int64_t y1,
        const int64_t x2,
        const int64_t y2,
        const int64_t x3,
        const int64_t y3)
    {
        const int64_t temp = (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2;
        if (temp < 0)
        {
            return -temp;
        }
        return temp;
    }

    // Divides an integer while rounding up
    static constexpr int FractionalCeil(const int numerator, const int denominator)
    {
        // Divide and add 0 or 1 to the value depending on remainder
        return (numerator / denominator) + (((numerator < 0) ^ (denominator > 0)) && (numerator % denominator));
    }

    // Divides an integer while rounding down to closest integer
    template <std::integral T>
    static constexpr T FractionalRound(const T numerator, const T denominator)
    {
        // Reference: https://stackoverflow.com/a/18067292
        return ((numerator < 0) ^ (denominator < 0)) ? ((numerator - denominator / 2) / denominator)
                                                     : ((numerator + denominator / 2) / denominator);
    }

    // Calculates sin * kPrecisionFactor of an angle in degrees
    static constexpr int SinScaled(const int angle)
    {
        const int index = AngleLimitTo360(angle);
        if (index > 180)
        {
            return -SinScaled(index - 180);
        }
        else if (index > 90)
        {
            return sine_table_[static_cast<size_t>(180 - index)];
        }
        return sine_table_[static_cast<size_t>(index)];
    }

    // Calculates cos * kPrecisionFactor of an angle in degrees
    static constexpr int CosScaled(const int angle)
    {
        return SinScaled(90 - angle);
    }

    // Rotate a point around the origin (counter clockwise) and return the x coordinate
    static constexpr int RotatePointX(const int x, const int y, const int angle)
    {
        return (Math::CosScaled(angle) * x - Math::SinScaled(angle) * y) / kPrecisionFactor;
    }

    // Rotate a point around the origin (counter clockwise) and return the y coordinate
    static constexpr int RotatePointY(const int x, const int y, const int angle)
    {
        return (Math::SinScaled(angle) * x + Math::CosScaled(angle) * y) / kPrecisionFactor;
    }

    /**
     * Calculates the angle in radians turns in the euclidean plane between the positive x axis
     * and the ray to the point(x, y).
     *
     * 16-bit fixed point four-quadrant arctangent. Given some Cartesian vector
     * (x, y), find the angle subtended by the vector and the positive x-axis.
     *
     * The value returned is in units of 1/65536ths of one turn. This allows the use
     * of the full 16-bit unsigned range to represent a turn. e.g. 0x0000 is 0
     * radians, 0x8000 is pi radians, and 0xFFFF is (65535 / 32768) * pi radians.
     *
     * Because the magnitude of the input vector does not change the angle it
     * represents, the inputs can be in any signed 16-bit fixed-point format.
     *
     * @param y y-coordinate in signed 16-bit
     * @param x x-coordinate in signed 16-bit
     *
     * @return angle in (val / 32768) * pi radian increments in the range [0, 65535] (aka radian
     * turns)
     */
    static uint16_t ATan2(const int16_t y, const int16_t x);

    // Convert angle from radian turns (range [0, 65535]) to normal form in degrees.
    // Returns the angle in degrees in the range [0, 360).
    static constexpr int RadianTurnsToDegrees(const uint16_t radian_turns)
    {
        return (radian_turns / kOneDegreeRadianTurns) % 360;
    }

    // Calculate the difference between two angles (in degrees)
    // Returns the angle difference in the range [-180, 180]
    static constexpr int AngleDifference180(const int angle1, const int angle2)
    {
        int difference = angle2 - angle1;

        // Too low, add full turn
        while (difference < -180)
        {
            difference += 360;
        }

        // Too high, remove full turn
        while (difference > 180)
        {
            difference -= 360;
        }

        return difference;
    }

    // Applies 360 degree turns until an angle falls within the range [0, 360)
    static constexpr int AngleLimitTo360(const int angle)
    {
        int new_angle = angle;

        // Greater or equal to 360 degrees, reduce
        while (new_angle >= 360)
        {
            new_angle -= 360;
        }

        // Less than 0 degrees, increase
        while (new_angle < 0)
        {
            new_angle += 360;
        }

        return new_angle;
    }

    // Convert units to sub units
    static constexpr int UnitsToSubUnits(const int units)
    {
        return units * kSubUnitsPerUnit;
    }

    // Convert sub units to units
    static constexpr int SubUnitsToUnits(const int sub_units)
    {
        return sub_units / kSubUnitsPerUnit;
    }

    // Max angle radians turns (65536)  which represents the full circle (aka 360 degrees)
    // See Atan2 method above
    static constexpr uint16_t kMaxRadianTurns = (std::numeric_limits<uint16_t>::max)();

    // The number of radian turns that represent 1 degree (aka 1/360 of a turn)
    static constexpr uint16_t kOneDegreeRadianTurns = kMaxRadianTurns / 360;  // 182

    static constexpr FixedPoint Round(const FixedPoint v)
    {
        // https://en.wikipedia.org/wiki/Rounding#Rounding_half_away_from_zero
        constexpr int p = PowInt(10, Max(FixedPoint::kPrecisionDigits - 1, 0));
        int64_t int_part = v.GetUnderlyingValue() / p;
        const auto mod = Abs(int_part % 10);
        int_part /= 10;
        if (mod >= 5)
        {
            if (v < 0_fp)
            {
                --int_part;
            }
            else
            {
                ++int_part;
            }
        }

        return FixedPoint::FromInt(int_part);
    }

private:
    /* Q15 functionality */
    static int16_t S16NAbs(const int16_t j);
    static int16_t Q15Mul(const int16_t j, const int16_t k);
    static int16_t Q15Div(const int16_t numer, const int16_t denom);

    //
    // NOTE: All the tables below are generated from this script: https://ideone.com/HiFceO
    //

    // Pre-calculated sine table.
    // Int Scale = 1000 (kPrecisionFactor).
    static constexpr int kSinMaxDegrees = 90;
    static constexpr std::array<int, kSinMaxDegrees + 1> sine_table_ = {
        0,   17,  34,  52,  69,  87,  104, 121, 139, 156, 173, 190, 207, 224, 241, 258, 275, 292, 309,
        325, 342, 358, 374, 390, 406, 422, 438, 453, 469, 484, 500, 515, 529, 544, 559, 573, 587, 601,
        615, 629, 642, 656, 669, 681, 694, 707, 719, 731, 743, 754, 766, 777, 788, 798, 809, 819, 829,
        838, 848, 857, 866, 874, 882, 891, 898, 906, 913, 920, 927, 933, 939, 945, 951, 956, 961, 965,
        970, 974, 978, 981, 984, 987, 990, 992, 994, 996, 997, 998, 999, 999, 1000};
};

}  // namespace simulation
