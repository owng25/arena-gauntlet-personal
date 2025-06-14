#pragma once

#include "draw_helper.h"
#include "rectangle.h"
#include "vector2f.h"

extern "C"
{
#include "raylib.h"
}

namespace simulation::tool
{
class RayFont : public Font
{
public:
    int GetBaseSize() const override
    {
        return font_.baseSize;
    }

    ::Font font_;
};

inline Vector2 ToRay(const Vector2f& v)
{
    return Vector2{v.x, v.y};
}

inline Vector2f FromRay(const Vector2& v)
{
    return Vector2f{v.x, v.y};
}

inline ::Rectangle ToRay(const Rectangle& r)
{
    return ::Rectangle{r.X(), r.Y(), r.Width(), r.Height()};
}

inline Rectangle FromRay(const ::Rectangle& r)
{
    return Rectangle{{r.x, r.y}, {r.width, r.height}};
}

inline ::Color ToRay(const Color& c)
{
    return ::Color{c.r, c.g, c.b, c.a};
}

inline Color FromRay(const ::Color& c)
{
    return Color{c.r, c.g, c.b, c.a};
}

inline const ::Font& ToRay(const Font& font)
{
    return static_cast<const RayFont&>(font).font_;
}
}  // namespace simulation::tool
