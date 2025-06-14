#pragma once

#include "vector2f.h"

namespace simulation::tool
{
struct Rectangle
{
    // Rectangle top-left corner position x
    Vector2f top_left = {0, 0};
    Vector2f size = {0, 0};

    constexpr float Width() const
    {
        return size.x;
    }

    constexpr float Height() const
    {
        return size.y;
    }

    constexpr float X() const
    {
        return top_left.x;
    }

    constexpr float Y() const
    {
        return top_left.y;
    }
};
}  // namespace simulation::tool
