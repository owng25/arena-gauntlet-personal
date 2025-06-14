#include "ui.h"

#include "draw_helper.h"
#include "input.h"

namespace simulation::tool::ui
{

static constexpr Color check_border_color = {73, 113, 116, 255};
static constexpr Color check_color = {235, 100, 64, 255};
static constexpr float check_padding = 4.0f;
static constexpr Color check_text_color = {101, 100, 124, 255};

static bool IsPointInsideRectangle(const Vector2f p, const Rectangle& rec)
{
    return (
        (p.x >= rec.top_left.x) && (p.x < (rec.top_left.x + rec.size.x)) && (p.y >= rec.top_left.y) &&
        (p.y < (rec.top_left.y + rec.size.y)));
}

bool CheckBox(
    const DrawHelper& helper,
    const Rectangle& bounds,
    std::string_view text,
    bool is_checked,
    const Font* font)
{
    // Update control
    const Vector2f mouse_point = Input::GetMousePosition();
    if (IsPointInsideRectangle(mouse_point, bounds))
    {
        if (Input::IsMouseButtonReleased(MouseButton::Left))
        {
            is_checked = !is_checked;
        }
    }

    helper.DrawRectangleLines(bounds, 2.0f, check_border_color);

    if (is_checked)
    {
        const Rectangle check_bound{bounds.top_left + check_padding, bounds.size - check_padding * 2.0f};
        helper.DrawRectangle(check_bound, check_color);
    }

    const Vector2f text_position{
        bounds.top_left.x + bounds.size.x + check_padding * 3,
        bounds.top_left.y + check_padding};

    if (!font)
    {
        font = &helper.GetFont();
    }

    constexpr float spacing = 2.f;
    helper.DrawText(*font, text, text_position, check_text_color, static_cast<float>(font->GetBaseSize()), spacing);

    return is_checked;
}

void ProgressBar(const Rectangle& rect, float progress, const Color& fill_color, const Color& bg_color)
{
    ProgressBarBorder(rect, progress, fill_color, bg_color, 0.0f, constants::color::White);
}

void ProgressBarBorder(
    const Rectangle& rect,
    float progress,
    const Color& fill_color,
    const Color& bg_color,
    float border_width,
    const Color& border_color)
{
    const Rectangle inner_rect{rect.top_left + border_width, rect.size - border_width * 2.0f};

    // Background Color
    DrawHelper::DrawRectangle(inner_rect, bg_color);

    // Fill Color
    const float fill_bar_width = inner_rect.size.x * progress;
    DrawHelper::DrawRectangle(inner_rect.top_left, {fill_bar_width, inner_rect.size.y}, fill_color);

    // Border Color
    if (border_width > 0.0f)
    {
        DrawHelper::DrawRectangleLines(rect, border_width, border_color);
    }
}

std::size_t ComboBox(
    const DrawHelper& helper,
    const Rectangle& bounds,
    const std::vector<std::string>& texts,
    std::size_t active,
    const Font* font)
{
    // Update control
    //--------------------------------------------------------------------
    const Vector2f mousePoint = Input::GetMousePosition();
    if (IsPointInsideRectangle(mousePoint, bounds))
    {
        if (Input::IsMouseButtonReleased(MouseButton::Left))
        {
            active += 1;
        }
    }

    if (active >= texts.size())
    {
        active = 0;
    }

    //--------------------------------------------------------------------
    // Draw control
    helper.DrawRectangle(bounds, {255, 255, 255, 200});
    helper.DrawRectangleLines(bounds, 2.0f, check_border_color);

    const float arrow_width = bounds.size.y - check_padding * 2.0f;

    const Rectangle arrow_rect{bounds.top_left + check_padding, {arrow_width, arrow_width}};

    const Vector2f a = arrow_rect.top_left;
    const Vector2f b = a + Vector2f{0, arrow_rect.size.y};
    const Vector2f c = a + Vector2f{arrow_rect.size.x, arrow_rect.size.y / 2.0f};
    helper.DrawTriangle(a, b, c, check_color);

    const Vector2f text_position = a + Vector2f{arrow_rect.size.x + check_padding, 0};

    if (!font)
    {
        font = &helper.GetFont();
    }

    const std::string text = fmt::format("[{}/{}] {}", active + 1, texts.size(), texts.at(active));
    constexpr float spacing = 2.f;
    helper.DrawText(*font, text, text_position, check_text_color, static_cast<float>(font->GetBaseSize()), spacing);

    return active;
}

}  // namespace simulation::tool::ui
