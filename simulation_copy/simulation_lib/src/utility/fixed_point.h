#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

#include "utility/custom_formatter.h"

namespace simulation
{

constexpr int PowInt(const int value, const int power)
{
    int result = 1;
    assert(power >= 0);
    for (int p = power; p > 0; --p)
    {
        result *= value;
    }
    return result;
}

// https://en.wikipedia.org/wiki/Fixed_point_(mathematics)
// We can't use float or double in our calculations but sometimes it is required
// to do some operations with precision higher than integral.
// This class is a fixed point integer which reserves a few (kPrecisionDigits)
// decimal digits for a fractional part. For example FixedPoint::FromInt(42) will internally be represented as 42000.
// There are no implicit conversion from or to builtin integers and that was done intentionally to highlight places
// where you convert such values.
// There are two user-defined literals so you can construct literals simpler.
// 123_fp - when you have just integer literal and it should become fixed point.
// "123.12"_fp - for the cases when you want to specify fractional part.
class FixedPoint
{
public:
    // The number for digits reserved for fractional part
    static constexpr int kPrecisionDigits = 3;

    // The number that will be used as multiplier for an original integer
    static constexpr int64_t kPrecision = PowInt(10, kPrecisionDigits);

    // Initializes to zero by default
    constexpr FixedPoint() = default;

    // Constructs fixed point from regular integer.
    // It is required since there is public constructor for that
    constexpr static FixedPoint FromInt(int64_t value)
    {
        return FixedPoint(value);
    }

    // Helper function to make fixed point from integral and fractional parts.
    // When using it keep in mind the difference shown in the examples below.
    //     FromDecimalReal(100,  24) -> 100.024
    //     FromDecimalReal(100, 240) -> 100.240
    constexpr static FixedPoint FromDecimalReal(const int64_t integral, const int64_t fractional)
    {
        assert(fractional / kPrecision == 0);
        int64_t result = integral * kPrecision;
        result += (fractional % kPrecision);
        return FixedPoint::Exact(result);
    }

    // zeroes fractional part
    constexpr FixedPoint Floor() const
    {
        return FixedPoint(value_ / kPrecision);
    }

    // Returns just fractional part
    constexpr int64_t GetFractionalPart() const
    {
        const int64_t v = value_ % kPrecision;
        // abs is not constexpr in msvc so...
        if (v > 0)
        {
            // using if instead of ternary operator here is intentional since
            // in 99% of cases this value will be positive thus the branch will
            // be predicted by cpu
            return v;
        }

        return -v;
    }

    // Calculates the percentage of a value in an integer safe manner.
    //
    // Calculation:
    // AsPercentageOf (with floats) = percentage * value
    // percentage = percentage / kMaxPercentage (because percentages are scaled by this constant)
    // => AsPercentageOf (with ints) = (percentage * value) / kMaxPercentage
    //
    // Example:
    // As we can't use floats, 25% (0.25) will represented as 25.
    // - 25% of 1000
    // 25 * 1000 / 100 = 250
    constexpr FixedPoint AsPercentageOf(const FixedPoint arg) const
    {
        return (*this * arg) / FixedPoint(100);
    }

    // AsPercentageOf Variant with high precision (everything is scaled up by 100)
    // And where 0.01% is percentage = 1
    // Examples:
    // 1% is percentage = 100
    // 6.25% is percentage = 625
    // 23.35% is percentage = 2335
    // 100% is percentage = 10000
    constexpr FixedPoint AsHighPrecisionPercentageOf(const FixedPoint arg) const
    {
        return (*this * arg) / FixedPoint(10000);
    }

    // Calculates the proportional percentage of value relative to max_value
    // If value == max_value then this returns kMaxPercentage
    // Calculation is basically: value / max_value * 100
    constexpr FixedPoint AsProportionalPercentageOf(const FixedPoint max_value) const
    {
        return FixedPoint(value_ * 100 / max_value.value_);
    }

    // Converts fixed point to integer. Drops fractional part.
    constexpr int AsInt() const
    {
        return static_cast<int>(value_ / kPrecision);
    }

    constexpr float AsFloat() const
    {
        return static_cast<float>(value_) / kPrecision;
    }

    constexpr bool operator==(const FixedPoint another) const
    {
        return value_ == another.value_;
    }

    constexpr bool operator!=(const FixedPoint another) const
    {
        return !(value_ == another.value_);
    }

    constexpr bool operator<(const FixedPoint another) const
    {
        return value_ < another.value_;
    }

    constexpr bool operator<=(const FixedPoint another) const
    {
        return value_ <= another.value_;
    }

    constexpr bool operator>(const FixedPoint another) const
    {
        return value_ > another.value_;
    }

    constexpr bool operator>=(const FixedPoint another) const
    {
        return value_ >= another.value_;
    }

    constexpr const FixedPoint& operator*=(const FixedPoint another)
    {
        // Let's imagine we had two integers that should be multiplied 1.234 and 5.678. They are represented as 1234 and
        // 5678 internally. As a result of multiplication of these internal value we will have 7006652, (representation
        // for 7006.652) which is kPrecision times bigger than expected 7006 (7.006). This happens because in fact we
        // have this operation: (a * kPrecision) * (b * kPrecision). What we want to get there is c * kPrecision but
        // just multiplication will return c * kPrecision * kPrecision.
        // Similar thing happens during division but in an opposite way.
        value_ *= another.value_;
        value_ /= kPrecision;
        return *this;
    }

    constexpr FixedPoint operator*(const FixedPoint another) const
    {
        // reuse *= implementation
        auto copy = *this;
        copy *= another;
        return copy;
    }

    constexpr const FixedPoint& operator/=(const FixedPoint another)
    {
        // See comment in operator*= why we multiply by kPrecision here
        value_ *= kPrecision;
        value_ /= another.value_;
        return *this;
    }

    constexpr FixedPoint operator/(const FixedPoint another) const
    {
        // reuse /= implementation
        auto copy = *this;
        copy /= another;
        return copy;
    }

    constexpr const FixedPoint& operator+=(const FixedPoint another)
    {
        value_ += another.value_;
        return *this;
    }

    constexpr FixedPoint operator+(const FixedPoint another) const
    {
        // reuse += implementation
        auto copy = *this;
        copy += another;
        return copy;
    }

    constexpr const FixedPoint& operator-=(const FixedPoint another)
    {
        value_ -= another.value_;
        return *this;
    }

    constexpr FixedPoint operator-(const FixedPoint another) const
    {
        // reuse -= implementation
        auto copy = *this;
        copy -= another;
        return copy;
    }

    constexpr FixedPoint operator-() const
    {
        auto copy = *this;
        copy.value_ = -copy.value_;
        return copy;
    }

    inline void FormatWithExactDigits(fmt::format_context& ctx, const size_t num_digits) const
    {
        fmt::format_to(ctx.out(), "{}", AsInt());

        if (num_digits == 0)
        {
            return;
        }

        // Print only non-zero fractional part
        int64_t fractional_part = GetFractionalPart();

        const size_t k = (std::min<size_t>)(num_digits, kPrecisionDigits);
        for (size_t i = k; i != kPrecisionDigits; ++i)
        {
            fractional_part /= 10;
        }

        fmt::format_to(ctx.out(), ".{:0>{}}", fractional_part, k);
    }

    inline void FormatTo(fmt::format_context& ctx) const
    {
        fmt::format_to(ctx.out(), "{}", AsInt());

        // Print only non-zero fractional part
        const int64_t fractional_part = GetFractionalPart();
        if (fractional_part != 0)
        {
            fmt::format_to(ctx.out(), ".{:0>{}}", fractional_part, kPrecisionDigits);
        }
    }

    std::string ToString() const;

    static constexpr bool TryParse(const std::string_view view, FixedPoint* out_value)
    {
        if (view.empty()) return false;

        size_t dots_count = 0;
        for (char c : view)
        {
            if (c == '.') dots_count += 1;
        }

        if (dots_count >= 2) return false;

        int64_t integral_part = 0;
        size_t index = 0;
        while (index != view.size() && view[index] != '.')
        {
            const char c = view[index];
            assert(c >= '0' && c <= '9');
            const int digit = c - '0';
            integral_part *= 10;
            integral_part += digit;
            index += 1;
        }

        int fractional_part = 0;
        if (index != view.size() && view[index] == '.')
        {
            index += 1;
            int m = FixedPoint::kPrecision;
            while (index != view.size())
            {
                const char c = view[index];
                assert(c >= '0' && c <= '9');
                const int digit = c - '0';
                fractional_part *= 10;
                fractional_part += digit;
                index += 1;
                m /= 10;

                if (m <= 0)
                {
                    // This assertion means that you use literal with more fractional digits than fixed point can have.
                    return false;
                }
            }

            fractional_part *= m;
        }

        *out_value = FixedPoint::FromDecimalReal(integral_part, fractional_part);
        return true;
    }

    constexpr int64_t GetUnderlyingValue() const
    {
        return value_;
    }

private:
    // Constructs fixed point from an integer.
    // It multiplies the value by kPrecision.
    // It is private to avoid implicit conversions.
    constexpr explicit FixedPoint(int64_t value) : value_(value * kPrecision) {}

    // Utility to make a fixed point from a value which was already multiplied by kPrecision
    static constexpr FixedPoint Exact(int64_t value)
    {
        FixedPoint result;
        result.value_ = value;
        return result;
    }

    int64_t value_ = 0;
};

// separate namespace so that we can do using namespace in unreal project and import literals only
namespace fixed_point_literals
{

// User defined literal to construct fixed point values from integer literals
// Example: 123_fpi
constexpr FixedPoint operator""_fp(unsigned long long literal)
{
    assert(literal <= static_cast<unsigned long long>((std::numeric_limits<int64_t>::max)() / 100));
    return FixedPoint::FromInt(static_cast<int64_t>(literal));
}

// User-defined literal to construct fixed point values with fractional part
// Example: "123.456"_fpi
// Note: theoretically it can be implemented without double quotes but
// such a literal would accept double value which we want to avoid
constexpr FixedPoint operator""_fp(const char* literal, size_t len)
{
    const std::string_view view(literal, len);
    FixedPoint value{};
    const bool parsed = FixedPoint::TryParse(view, &value);
    assert(parsed);
    (void)parsed;  // prevent unused wariable warning in MSVC non-debug build...
    return value;
}
}  // namespace fixed_point_literals

using namespace fixed_point_literals;

}  // namespace simulation

template <>
struct fmt::formatter<simulation::FixedPoint> : simulation::FormatterWithEmptyParse
{
    auto format(const simulation::FixedPoint& value, fmt::format_context& ctx) const -> decltype(ctx.out())
    {
        if (opt_num_digits)
        {
            value.FormatWithExactDigits(ctx, *opt_num_digits);
        }
        else
        {
            value.FormatTo(ctx);
        }
        return ctx.out();
    }

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        const auto begin = ctx.begin();
        const auto end = std::find(ctx.begin(), ctx.end(), '}');
        const auto distance = std::distance(begin, end);
        if (distance > 0)
        {
            if (distance == 1)
            {
                if (const char c = *begin; c >= '0' && c <= '3')
                {
                    opt_num_digits = static_cast<size_t>(c - '0');
                    return end;
                }
            }

            assert(distance == 1);
            return ctx.end();
        }

        return end;
    }

    std::optional<size_t> opt_num_digits;
};

template <>
struct simulation::HasSimFormatter<simulation::FixedPoint>
{
    static constexpr bool kValue = true;
};
