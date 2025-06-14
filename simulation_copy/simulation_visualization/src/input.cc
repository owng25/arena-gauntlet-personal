#include "input.h"

#include "raylib_cpp.h"
#include "utility/ensure_enum_size.h"

namespace simulation::tool
{

static int ToRayKeyboardKey(const KeyboardKey key)
{
    ILLUVIUM_ENSURE_ENUM_SIZE(KeyboardKey, 10);
    switch (key)
    {
    case KeyboardKey::Space:
        return KEY_SPACE;
    case KeyboardKey::Enter:
        return KEY_ENTER;
    case KeyboardKey::W:
        return KEY_W;
    case KeyboardKey::A:
        return KEY_A;
    case KeyboardKey::S:
        return KEY_S;
    case KeyboardKey::D:
        return KEY_D;
    case KeyboardKey::E:
        return KEY_E;
    case KeyboardKey::Q:
        return KEY_Q;
    case KeyboardKey::ShiftLeft:
        return KEY_LEFT_SHIFT;
    case KeyboardKey::ShiftRight:
        return KEY_RIGHT_SHIFT;
    default:
        assert(false);
        return -1;
    }
}

static int ToRayMouseButton(const MouseButton button)
{
    ILLUVIUM_ENSURE_ENUM_SIZE(MouseButton, 2);
    switch (button)
    {
    case MouseButton::Left:
        return MOUSE_LEFT_BUTTON;
    case MouseButton::Right:
        return MOUSE_RIGHT_BUTTON;
    default:
        assert(false);
        return -1;
    }
}

bool Input::IsKeyPressed(const KeyboardKey key)
{
    return ::IsKeyPressed(ToRayKeyboardKey(key));
}

bool Input::IsKeyUp(const KeyboardKey key)
{
    return ::IsKeyUp(ToRayKeyboardKey(key));
}

bool Input::IsKeyDown(const KeyboardKey key)
{
    return ::IsKeyDown(ToRayKeyboardKey(key));
}

bool Input::IsMouseButtonReleased(const MouseButton button)
{
    return ::IsMouseButtonReleased(ToRayMouseButton(button));
}

Vector2f Input::GetMousePosition()
{
    return FromRay(::GetMousePosition());
}

float Input::GetMouseWheelMove()
{
    return ::GetMouseWheelMove();
}

}  // namespace simulation::tool
