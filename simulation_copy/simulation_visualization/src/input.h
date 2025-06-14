#pragma once

#include "vector2f.h"

namespace simulation::tool
{

enum class KeyboardKey
{
    Enter,
    Space,
    W,
    A,
    S,
    D,
    E,
    Q,
    ShiftLeft,
    ShiftRight,
    kNum
};

enum class MouseButton
{
    Left,
    Right,
    kNum
};

class Input
{
public:
    static bool IsKeyPressed(const KeyboardKey key);
    static bool IsKeyUp(const KeyboardKey key);
    static bool IsKeyDown(const KeyboardKey key);
    static bool IsMouseButtonReleased(const MouseButton key);
    static Vector2f GetMousePosition();
    static float GetMouseWheelMove();
};

}  // namespace simulation::tool
