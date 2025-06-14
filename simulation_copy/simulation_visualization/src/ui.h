#pragma once

#include <string>
#include <vector>

#include "color.h"
#include "rectangle.h"

namespace simulation::tool
{
class DrawHelper;
class Font;

namespace ui
{

// Check Box control, returns true when active
bool CheckBox(
    const DrawHelper& helper,
    const Rectangle& bounds,
    std::string_view text,
    bool is_checked,
    const Font* font = nullptr);

// Draw progress bar
void ProgressBar(const Rectangle& rect, float progress, const Color& fill_color, const Color& bg_color);

// Draw progress bar with border
void ProgressBarBorder(
    const Rectangle& rect,
    float progress,
    const Color& fill_color,
    const Color& bg_color,
    float border_width,
    const Color& border_color);

// Combo Box control, returns selected item codepointIndex
std::size_t ComboBox(
    const DrawHelper& helper,
    const Rectangle& bounds,
    const std::vector<std::string>& texts,
    std::size_t active,
    const Font* font = nullptr);

}  // namespace ui
}  // namespace simulation::tool
