#pragma once

#include <cmath>

#include "utility/ivector2d.h"

namespace simulation::tool
{
struct Vector2f
{
    float x = 0.f;
    float y = 0.f;

    constexpr Vector2f() = default;
    constexpr Vector2f(const Vector2f&) noexcept = default;
    constexpr Vector2f(Vector2f&&) noexcept = default;
    constexpr Vector2f& operator=(const Vector2f&) noexcept = default;
    constexpr Vector2f& operator=(Vector2f&&) noexcept = default;

    constexpr Vector2f(const float x_, const float y_) noexcept : x(x_), y(y_) {}
    explicit constexpr Vector2f(const float v) noexcept : x(v), y(v) {}
    explicit constexpr Vector2f(const simulation::IVector2D& v) noexcept
        : x(static_cast<float>(v.x)),
          y(static_cast<float>(v.y))
    {
    }

    constexpr Vector2f& operator-=(const Vector2f& r)
    {
        x -= r.x;
        y -= r.y;
        return *this;
    }

    constexpr Vector2f operator-(const Vector2f& r) const
    {
        auto copy = *this;
        copy -= r;
        return copy;
    }

    constexpr Vector2f& operator-=(const float v)
    {
        x -= v;
        y -= v;
        return *this;
    }

    constexpr Vector2f operator-(const float v) const
    {
        auto copy = *this;
        copy -= v;
        return copy;
    }

    constexpr Vector2f operator-() const
    {
        return {-x, -y};
    }

    constexpr Vector2f& operator+=(const Vector2f& r)
    {
        x += r.x;
        y += r.y;
        return *this;
    }

    constexpr Vector2f operator+(const Vector2f& r) const
    {
        auto copy = *this;
        copy += r;
        return copy;
    }

    constexpr Vector2f& operator+=(const float v)
    {
        x += v;
        y += v;
        return *this;
    }

    constexpr Vector2f operator+(const float v) const
    {
        auto copy = *this;
        copy += v;
        return copy;
    }

    constexpr Vector2f& operator/=(const float v)
    {
        x /= v;
        y /= v;
        return *this;
    }

    constexpr Vector2f operator/(const float v) const
    {
        auto copy = *this;
        copy /= v;
        return copy;
    }

    constexpr Vector2f& operator/=(const Vector2f& v)
    {
        x /= v.x;
        y /= v.y;
        return *this;
    }

    constexpr Vector2f operator/(const Vector2f& v) const
    {
        auto copy = *this;
        copy /= v;
        return copy;
    }

    constexpr Vector2f& operator*=(const float v)
    {
        x *= v;
        y *= v;
        return *this;
    }

    constexpr Vector2f operator*(const float v) const
    {
        auto copy = *this;
        copy *= v;
        return copy;
    }

    constexpr Vector2f& operator*=(const Vector2f& v)
    {
        x *= v.x;
        y *= v.y;
        return *this;
    }

    constexpr Vector2f operator*(const Vector2f& v) const
    {
        auto copy = *this;
        copy *= v;
        return copy;
    }

    constexpr float Dot(const Vector2f& r) const
    {
        return x * r.x + y * r.y;
    }

    float Length() const
    {
        return std::sqrt(Dot(*this));
    }

    void Normalize()
    {
        *this /= Length();
    }

    Vector2f Normalized() const
    {
        auto copy = *this;
        copy.Normalize();
        return copy;
    }

    constexpr float Max() const
    {
        return x < y ? y : x;
    }
    constexpr float Min() const
    {
        return x > y ? y : x;
    }
    constexpr Vector2f SwappedXY() const
    {
        return {y, x};
    }
};
}  // namespace simulation::tool
