#pragma once

#include <memory>

#include "color.h"
#include "constants.h"
#include "draw_settings.h"
#include "ecs/world.h"
#include "rectangle.h"
#include "vector2f.h"

namespace simulation::tool
{

class Font
{
public:
    static std::unique_ptr<Font> CreateDefaultFont();
    static std::unique_ptr<Font> TryLoadAnyTTF();
    virtual int GetBaseSize() const = 0;

    virtual ~Font() = default;
};

/* -------------------------------------------------------------------------------------------------------
 * DrawHelper
 *
 * This class contains all data necessary for simulation visualization and some reusable drawing function.
 * --------------------------------------------------------------------------------------------------------
 */
class DrawHelper
{
public:
    explicit DrawHelper(std::shared_ptr<World> world);
    ~DrawHelper();

    // Draw grid hexes
    void DrawFillHex(const HexGridPosition& position, const Color& color, float padding = 0.0f) const;
    void DrawFillHexes(const HexGridPosition& position, int radius, const Color& color) const;
    void DrawBorderHexes(const HexGridPosition& position, int radius, const Color& color) const;
    void DrawFillHexesLayered(const HexGridPosition& position, int radius, const Color& color, int layer_id);
    void DrawBorderHexesLayered(const HexGridPosition& position, int radius, const Color& color, int layer_id);

    void DrawLine(const HexGridPosition& from, const HexGridPosition& to, const int width, const Color& color) const;
    void DrawLine(const HexGridPosition& from, const HexGridPosition& to, const float width, const Color& color) const;

    static void DrawTriangle(const Vector2f& a, const Vector2f& b, const Vector2f& c, const Color& color);
    static void DrawTriangleLines(const Vector2f& a, const Vector2f& b, const Vector2f& c, const Color& color);
    static void DrawRectangle(const Rectangle& r, const Color& c);
    static void DrawRectangle(const Vector2f& position, const Vector2f& size, const Color& color);
    static void DrawRectangleLines(const Rectangle& rec, float thick, const Color& color);

    static void DrawLine(const Vector2f& from, const Vector2f& to, float thick, const Color& color);

    void Clear(const Color& color) const;

    void DrawArrow(const HexGridPosition& from, const HexGridPosition& to, float thick, const Color& color) const;

    void DrawArrowPos(const PositionComponent& from, const PositionComponent& to, float thick, const Color& color)
        const;

    static void DrawText(
        const Font& font,
        const std::string_view text,
        const Vector2f& pos,
        const Color& color,
        const float size = -1.0f,
        const float spacing = GetDefaultSpacing());

    void DrawText(
        std::string_view text,
        const Vector2f& pos,
        const Color& color,
        float size = -1.0f,
        float spacing = GetDefaultSpacing()) const;
    static Vector2f
    MeasureTextSize(const Font& font, std::string_view text, float size = -1.0f, float spacing = GetDefaultSpacing());
    Vector2f MeasureTextSize(const std::string_view text, const float size = -1.0f, float spacing = GetDefaultSpacing())
        const
    {
        return MeasureTextSize(GetFont(), text, size, spacing);
    }

    // Draw text using loaded font, default size use font base size
    void DrawBorderedText(
        std::string_view text,
        const Vector2f& pos,
        const Vector2f& bg_size,
        const Color& text_color,
        const Color& bg_color,
        float padding = 3.0f,
        float font_size = -1.0f) const;

    // Draw combat unit
    void DrawCombatUnit(const simulation::Entity& entity) const;

    const constants::color::TeamColorSchema& GetEntityColorSchema(const simulation::EntityID entity_id) const;
    static const constants::color::TeamColorSchema& GetEntityColorSchema(const simulation::Entity& entity);
    static const constants::color::TeamColorSchema& GetTeamColorSchema(const simulation::Team team);

    // Draw attached effects for give unit
    void DrawAttachedEffect(
        const Entity& entity,
        const std::string_view effect_type_name,
        const std::string_view effect_sub_type_name,
        const std::vector<AttachedEffectStatePtr>& effects,
        const Color& bg_color);

    // Covert
    Vector2f CovertHexToScreen(const HexGridPosition& position) const;
    Vector2f WorldToScreen(const Vector2f& position) const;
    Vector2f WorldToScreen(const IVector2D& position) const
    {
        return WorldToScreen(Vector2f{position});
    }
    IVector2D CovertFromScreen(const Vector2f& position) const;
    float ConvertEntityRadiusToScreen(int radius) const;

    // Getters
    const Font& GetFont(bool prefer_ttf = false) const
    {
        if (prefer_ttf && ttf_) return *ttf_;
        return *font_;
    }

    float GetDefaultFontSize() const;
    static constexpr float GetDefaultSpacing()
    {
        return 1.f;
    }

    void AddEvents(simulation::EntityID entity, int count);
    int GetEventsCount(simulation::EntityID entity) const;

    static Color GenerateRandomColor();

    static Color ColorScale(const Color& color, float scale);

    const DrawSettings& GetDrawSettings() const
    {
        return draw_settings;
    }

    DrawSettings& GetDrawSettingsMutable()
    {
        return draw_settings;
    }

    float GetScreenWidth() const;
    float GetScreenHeight() const;

    Vector2f GetScreenSize() const
    {
        return {GetScreenWidth(), GetScreenHeight()};
    }

    EntityID GetSelectedUnit(const HexGridPosition& mouse_position) const;

    void PreDrawFrame();

    void CloseWindow() const;
    bool IsWindowReady() const;
    bool WindowShouldClose() const;
    void BeginFrame(const Rectangle& world_rect, const float zoom);
    void EndFrame();

    void DrawHexGroupOutline(const HexGridPosition& position, int radius, const Color& color, const float width) const
    {
        DrawHexGroup(position, radius, Color{0, 0, 0, 0}, width, color);
    }

    void DrawHexGroup(
        const HexGridPosition& position,
        int radius,
        const Color& fill,
        const float border_width = -1.f,
        const Color& border_color = constants::color::Black) const;

private:
    void RayFillHex(const Vector2f& position, float padding, const Color& color) const;

    static void RayEntityBorderHex(const Vector2f& position, float radius, float line_thick, const Color& outline);

    std::shared_ptr<World> world_;
    std::unique_ptr<Font> font_;
    std::unique_ptr<Font> ttf_;

    std::map<EntityID, int> active_effects;
    std::unordered_map<int, std::unordered_set<size_t>> drawn_layer_hexes;
    Rectangle world_rect_{};
    float zoom_ = 1.f;

    DrawSettings draw_settings;
};

}  // namespace simulation::tool
