#include "draw_helper.h"

#include "components/combat_unit_component.h"
#include "components/position_component.h"
#include "constants.h"
#include "raylib_cpp.h"
#include "ui.h"
#include "utility/entity_helper.h"

static constexpr float kGridCellSize = 10.0f;

template <typename T>
auto RayConvert(T&& arg)
{
    using CT = std::decay_t<T>;
    if constexpr (std::is_floating_point_v<CT> || std::is_integral_v<CT> || std::is_pointer_v<CT>)
    {
        return arg;
    }
    else
    {
        return ToRay(std::forward<T>(arg));
    }
}

template <typename Function, typename... Args>
void CallRay(Function function, Args&&... args)
{
    function(RayConvert(std::forward<Args>(args))...);
}

template <typename Function, typename... Args>
auto CallRayR(Function function, Args&&... args)
{
    return simulation::tool::FromRay(function(RayConvert(std::forward<Args>(args))...));
}

namespace simulation::tool
{

std::unique_ptr<Font> Font::TryLoadAnyTTF()
{
    namespace fs = std::filesystem;
    const auto executable_dir = FileHelper::GetExecutableDirectory();
    for (const fs::directory_entry& dir_entry : fs::directory_iterator(executable_dir))
    {
        if (!dir_entry.is_regular_file())
        {
            continue;
        }

        const fs::path& path = dir_entry.path();
        if (!path.has_extension())
        {
            continue;
        }

        const auto ext = path.extension().string();
        if (ext != ".ttf")
        {
            continue;
        }

        auto font = std::make_unique<RayFont>();
        font->font_ = ::LoadFont(path.string().data());
        return font;
    }

    return nullptr;
}

std::unique_ptr<Font> Font::CreateDefaultFont()
{
    auto font = std::make_unique<RayFont>();
    font->font_ = ::GetFontDefault();
    return font;
}

DrawHelper::DrawHelper(std::shared_ptr<World> world) : world_(std::move(world))
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(draw_settings.GetScreenWidth(), draw_settings.GetScreenHeight(), "BattleVisualization");
    font_ = Font::CreateDefaultFont();
    ttf_ = Font::TryLoadAnyTTF();

    SetTraceLogLevel(LOG_ERROR);
    SetTargetFPS(60);
}

DrawHelper::~DrawHelper()
{
    draw_settings.SetScreenWidth(::GetScreenWidth());
    draw_settings.SetScreenHeight(::GetScreenHeight());
}

void DrawHelper::DrawFillHex(const HexGridPosition& position, const Color& color, float padding) const
{
    const IVector2D hex_world_pos = world_->ToWorldPosition(position);
    RayFillHex(WorldToScreen(hex_world_pos), padding, color);
}

void DrawHelper::DrawLine(const HexGridPosition& from, const HexGridPosition& to, const int width, const Color& color)
    const
{
    DrawLine(from, to, static_cast<float>(world_->ToWorldPosition(width)), color);
}

void DrawHelper::DrawLine(
    const simulation::HexGridPosition& from,
    const simulation::HexGridPosition& to,
    const float width,
    const Color& color) const
{
    const IVector2D from_world_position = world_->ToWorldPosition(from);
    const Vector2f from_screen_position = WorldToScreen(from_world_position);
    const IVector2D to_world_position = world_->ToWorldPosition(to);
    const Vector2f to_screen_position = WorldToScreen(to_world_position);
    DrawLine(from_screen_position, to_screen_position, width * zoom_, color);
}

void DrawHelper::DrawTriangle(const Vector2f& a, const Vector2f& b, const Vector2f& c, const Color& color)
{
    CallRay(::DrawTriangle, a, b, c, color);
}

void DrawHelper::DrawTriangleLines(const Vector2f& a, const Vector2f& b, const Vector2f& c, const Color& color)
{
    CallRay(::DrawTriangleLines, a, b, c, color);
}

void DrawHelper::DrawRectangle(const Rectangle& r, const Color& c)
{
    CallRay(::DrawRectangleRec, r, c);
}

void DrawHelper::DrawRectangle(const Vector2f& position, const Vector2f& size, const Color& color)
{
    CallRay(::DrawRectangleV, position, size, color);
}

void DrawHelper::DrawRectangleLines(const Rectangle& rec, float thick, const Color& color)
{
    CallRay(::DrawRectangleLinesEx, rec, thick, color);
}

void DrawHelper::DrawLine(const Vector2f& from, const Vector2f& to, float thick, const Color& color)
{
    CallRay(::DrawLineEx, from, to, thick, color);
}

void DrawHelper::DrawArrow(const HexGridPosition& from, const HexGridPosition& to, float thick, const Color& color)
    const
{
    const IVector2D from_world_position = world_->ToWorldPosition(from);
    const Vector2f from_screen_position = WorldToScreen(from_world_position);
    const IVector2D to_world_position = world_->ToWorldPosition(to);
    const Vector2f to_screen_position = WorldToScreen(to_world_position);
    DrawLine(from_screen_position, to_screen_position, thick, color);

    constexpr IVector2D direction_left = {1, 0};
    const IVector2D arrow_direction = to_world_position - from_world_position;
    const int angle = direction_left.AngleToPosition(arrow_direction);
    IVector2D triangle_a, triangle_b, triangle_c;
    to_world_position.GetTriangleVertices(angle, -50, &triangle_a, &triangle_b, &triangle_c);

    const Vector2f triangle_a_screen = WorldToScreen(triangle_a);
    const Vector2f triangle_b_screen = WorldToScreen(triangle_b);
    const Vector2f triangle_c_screen = WorldToScreen(triangle_c);
    DrawTriangle(triangle_a_screen, triangle_c_screen, triangle_b_screen, color);
}

void DrawHelper::DrawArrowPos(
    const PositionComponent& from,
    const PositionComponent& to,
    float thick,
    const Color& color) const
{
    IVector2D from_world_position = world_->ToWorldPosition(from.GetPosition());
    Vector2f from_screen_position = WorldToScreen(from_world_position);

    IVector2D to_world_position = world_->ToWorldPosition(to.GetPosition());
    Vector2f to_screen_position = WorldToScreen(to_world_position);

    // Calculate direction vector
    const Vector2f direction = (to_screen_position - from_screen_position).Normalized();

    // Calculate radius offsets
    const float from_screen_radius = ConvertEntityRadiusToScreen(from.GetRadius());
    const float to_screen_radius = ConvertEntityRadiusToScreen(to.GetRadius());
    const Vector2f from_position_offset = direction * from_screen_radius;
    const Vector2f to_position_offset = -direction * to_screen_radius;

    from_screen_position = from_screen_position + from_position_offset;
    to_screen_position = to_screen_position + to_position_offset;

    DrawLine(from_screen_position, to_screen_position, thick, color);

    // Update world with offsets
    from_world_position = CovertFromScreen(from_screen_position);
    to_world_position = CovertFromScreen(to_screen_position);

    // Calculate triangle
    constexpr IVector2D direction_left = {1, 0};
    const IVector2D arrow_direction = to_world_position - from_world_position;
    const int angle = direction_left.AngleToPosition(arrow_direction);
    IVector2D triangle_a, triangle_b, triangle_c;
    to_world_position.GetTriangleVertices(angle, -50, &triangle_a, &triangle_b, &triangle_c);

    const Vector2f triangle_a_screen = WorldToScreen(triangle_a);
    const Vector2f triangle_b_screen = WorldToScreen(triangle_b);
    const Vector2f triangle_c_screen = WorldToScreen(triangle_c);
    DrawTriangle(triangle_a_screen, triangle_c_screen, triangle_b_screen, color);
}

void DrawHelper::DrawFillHexes(const HexGridPosition& position, int radius, const Color& color) const
{
    const std::vector<HexGridPosition> hexes = world_->GetGridHelper().GetSpiralRingsPositions(position, radius);
    for (const HexGridPosition& hex : hexes)
    {
        const IVector2D hex_world_pos = world_->ToWorldPosition(hex);
        RayFillHex(WorldToScreen(hex_world_pos), -0.5f, color);
    }
}

void DrawHelper::DrawBorderHexes(const HexGridPosition& position, int radius, const Color& color) const
{
    const std::vector<HexGridPosition> hexes = world_->GetGridHelper().GetSingleRingPositions(position, radius);
    for (const HexGridPosition& hex : hexes)
    {
        const IVector2D hex_world_pos = world_->ToWorldPosition(hex);
        RayFillHex(WorldToScreen(hex_world_pos), -0.5f, color);
    }
}

void DrawHelper::DrawFillHexesLayered(const HexGridPosition& position, int radius, const Color& color, int layer_id)
{
    std::unordered_set<size_t>& layer_hexes = drawn_layer_hexes[layer_id];
    const std::vector<HexGridPosition> hexes = world_->GetGridHelper().GetSpiralRingsPositions(position, radius);
    const HexGridConfig& grid_config = world_->GetGridConfig();
    for (const HexGridPosition& hex : hexes)
    {
        const size_t hex_index = grid_config.GetGridIndex(hex);
        if (layer_hexes.count(hex_index) > 0)
        {
            continue;
        }
        const IVector2D hex_world_pos = world_->ToWorldPosition(hex);
        RayFillHex(WorldToScreen(hex_world_pos), -0.5f, color);
        layer_hexes.insert(hex_index);
    }
}

void DrawHelper::DrawBorderHexesLayered(const HexGridPosition& position, int radius, const Color& color, int layer_id)
{
    std::unordered_set<size_t>& layer_hexes = drawn_layer_hexes[layer_id];
    const std::vector<HexGridPosition> hexes = world_->GetGridHelper().GetSingleRingPositions(position, radius);
    const HexGridConfig& grid_config = world_->GetGridConfig();
    for (const HexGridPosition& hex : hexes)
    {
        const size_t hex_index = grid_config.GetGridIndex(hex);
        if (layer_hexes.count(hex_index) > 0)
        {
            continue;
        }
        const IVector2D hex_world_pos = world_->ToWorldPosition(hex);
        RayFillHex(WorldToScreen(hex_world_pos), -0.5f, color);
        layer_hexes.insert(hex_index);
    }
}

void DrawHelper::DrawText(
    const Font& font,
    const std::string_view text,
    const Vector2f& pos,
    const Color& color,
    const float size,
    const float spacing)
{
    CallRay(::DrawTextEx, font, text.data(), pos, size, spacing, color);
}

void DrawHelper::DrawText(std::string_view text, const Vector2f& pos, const Color& color, float size, float spacing)
    const
{
    if (size <= .0f)
    {
        size = GetDefaultFontSize();
    }

    DrawText(GetFont(), text, pos, color, size, spacing);
}

Vector2f DrawHelper::MeasureTextSize(const Font& font, std::string_view text, float size, float spacing)
{
    if (size <= .0f)
    {
        size = static_cast<float>(font.GetBaseSize());
    }

    return CallRayR(::MeasureTextEx, font, text.data(), size, spacing);
}

void DrawHelper::DrawBorderedText(
    std::string_view text,
    const Vector2f& position,
    const Vector2f& bg_size,
    const Color& text_color,
    const Color& bg_color,
    float padding,
    float font_size) const
{
    DrawRectangle(position, bg_size, bg_color);
    const Vector2f text_position = Vector2f{padding + position.x, padding + position.y};
    DrawText(text, text_position, text_color, font_size);
}

void DrawHelper::DrawCombatUnit(const Entity& entity) const
{
    // Draw position
    const auto& position_component = entity.Get<PositionComponent>();
    const auto& position = position_component.GetPosition();
    const auto& radius = position_component.GetRadius();

    constexpr float outline = 4.0f;
    const auto& team_color_schema = GetEntityColorSchema(entity);
    DrawHexGroup(position, radius, team_color_schema.hex_fill, outline, team_color_schema.hex_border);

    // Draw some text info
    const Vector2f screen_pos = CovertHexToScreen(position);
    const auto text_pos = screen_pos - zoom_ * 20.f;

    const auto& combat_unit_component = entity.Get<CombatUnitComponent>();
    const std::string message = fmt::format("ID {}\n{}", entity.GetID(), combat_unit_component.GetTypeID().line_name);
    DrawText(message, text_pos, constants::color::DarkBrown, GetDefaultFontSize() * zoom_);

    const auto bar_size = Vector2f{40.f, 4.f} * zoom_;

    // Bars progress
    const auto& stats_data = world_->GetLiveStats(entity.GetID());
    const float health_progress =
        stats_data.Get(StatType::kCurrentHealth).AsFloat() / stats_data.Get(StatType::kMaxHealth).AsFloat();
    const float energy_progress =
        stats_data.Get(StatType::kCurrentEnergy).AsFloat() / stats_data.Get(StatType::kEnergyCost).AsFloat();

    // Health bar
    const Rectangle health_bar_rect{screen_pos + Vector2f{-20.f, 5.f} * zoom_, bar_size};
    ui::ProgressBar(health_bar_rect, health_progress, constants::color::Red, PINK);

    // Energy bar
    const Rectangle energy_bar_rect{health_bar_rect.top_left + Vector2f{0.0f, bar_size.y + zoom_}, bar_size};
    ui::ProgressBar(energy_bar_rect, energy_progress, constants::color::DarkBlue, constants::color::Blue);
}

const constants::color::TeamColorSchema& DrawHelper::GetEntityColorSchema(const simulation::EntityID entity_id) const
{
    const Team team = world_->GetEntityTeam(entity_id);
    return GetTeamColorSchema(team);
}

const constants::color::TeamColorSchema& DrawHelper::GetEntityColorSchema(const simulation::Entity& entity)
{
    const Team team = entity.GetTeam();
    return GetTeamColorSchema(team);
}

const constants::color::TeamColorSchema& DrawHelper::GetTeamColorSchema(const simulation::Team team)
{
    switch (team)
    {
    case Team::kRed:
        return constants::color::kRedTeamColorSchema;
    case Team::kBlue:
        return constants::color::kBlueTeamColorSchema;

    default:
        assert(false);
        return constants::color::kUnknownColorSchema;
    }
}

void DrawHelper::DrawAttachedEffect(
    const Entity& entity,
    const std::string_view effect_type_name,
    const std::string_view effect_sub_type_name,
    const std::vector<AttachedEffectStatePtr>& effects,
    const Color& bg_color)
{
    if (effects.empty()) return;

    constexpr Color text_color = {36, 55, 99, 255};
    constexpr Color progress_bg_color = {245, 202, 195, 255};
    constexpr Color progress_fill_color = {242, 132, 130, 255};
    constexpr float progress_height = 10.f;
    constexpr float text_to_progress_spacing = 3.f;
    constexpr float space_between_effects = 3.f;
    const float font_size = GetDefaultFontSize();
    constexpr Vector2f bg_padding(5);

    const auto& position_component = entity.Get<PositionComponent>();
    Vector2f screen_position = CovertHexToScreen(position_component.GetPosition());
    screen_position.y += 20.0f + 45.0f * static_cast<float>(GetEventsCount(entity.GetID()));
    screen_position.x -= ConvertEntityRadiusToScreen(position_component.GetRadius());

    std::vector<std::tuple<std::string, Vector2f>> strings_and_sizes;

    Vector2f bg_size(0, bg_padding.y * 2);
    for (const auto& effect : effects)
    {
        auto& [text, render_size] = strings_and_sizes.emplace_back();

        std::string_view format = "{}: {}\n [{}] Duration: {}/{}";
        if (effect->duration_time_steps == kTimeInfinite)
        {
            format = "{}: {}\n [{}] Duration: {}";
        }
        else
        {
            // Add the height of progress bar rectangles to background rect
            bg_size.y += progress_height;

            // Add the spacing between between text and progress bar
            bg_size.y += text_to_progress_spacing;
        }

        text = fmt::format(
            fmt::runtime(format),
            effect_type_name,
            effect_sub_type_name,
            effect->current_stacks_count,
            effect->current_time_steps,
            effect->duration_time_steps);

        render_size = MeasureTextSize(text, font_size);

        bg_size.x = std::max(render_size.x + 2 * bg_padding.x, bg_size.x);
        bg_size.y += render_size.y;
    }

    // Add the space between effects
    bg_size.y += static_cast<float>(effects.size() - 1) * space_between_effects;

    // Draw background for all entries
    DrawRectangle(screen_position, bg_size, bg_color);

    screen_position += bg_padding;

    for (size_t index = 0; index != strings_and_sizes.size(); ++index)
    {
        if (index != 0)
        {
            screen_position.y += space_between_effects;
        }

        const auto& [text, render_size] = strings_and_sizes[index];
        const auto& effect = effects[index];

        DrawText(text, Vector2f{screen_position.x, screen_position.y}, text_color, font_size);
        screen_position.y += render_size.y + text_to_progress_spacing;

        if (effect->duration_time_steps != kTimeInfinite)
        {
            const float progress =
                static_cast<float>(effect->current_time_steps) / static_cast<float>(effect->duration_time_steps);

            const Rectangle progress_rect = {screen_position, {bg_size.x - 2 * bg_padding.x, progress_height}};
            ui::ProgressBar(progress_rect, progress, progress_fill_color, progress_bg_color);
        }

        screen_position.y += progress_height;
    }

    AddEvents(entity.GetID(), static_cast<int>(effects.size()));
}

Vector2f DrawHelper::CovertHexToScreen(const HexGridPosition& position) const
{
    const IVector2D world_position = world_->ToWorldPosition(position);
    return WorldToScreen(world_position);
}

void DrawHelper::Clear(const Color& color) const
{
    CallRay(::ClearBackground, color);
}

Color DrawHelper::GenerateRandomColor()
{
    constexpr int same_part = 100;
    constexpr int chanel_part = (250 - 100) / 2;
    const int rand_chanel = rand() % 3;                                          // NOLINT
    const auto same_rand_part = static_cast<unsigned char>(rand() % same_part);  // NOLINT
    const unsigned char r = static_cast<unsigned char>(2 - rand_chanel) * chanel_part + same_rand_part;
    const unsigned char g = static_cast<unsigned char>(std::abs(1 - rand_chanel)) * chanel_part + same_rand_part;
    const unsigned char b = static_cast<unsigned char>(rand_chanel * chanel_part) + same_rand_part;

    return {r, g, b, 255};
}

Color DrawHelper::ColorScale(const Color& color, float scale)
{
    Color result;
    result.r = static_cast<unsigned char>(std::clamp(static_cast<float>(color.r) * scale, 0.0f, 255.0f));
    result.g = static_cast<unsigned char>(std::clamp(static_cast<float>(color.g) * scale, 0.0f, 255.0f));
    result.b = static_cast<unsigned char>(std::clamp(static_cast<float>(color.b) * scale, 0.0f, 255.0f));
    result.a = color.a;
    return result;
}

float DrawHelper::GetDefaultFontSize() const
{
    return static_cast<float>(GetFontDefault().baseSize);
}

void DrawHelper::AddEvents(EntityID entity, int count)
{
    active_effects.insert_or_assign(entity, count + GetEventsCount(entity));
}

int DrawHelper::GetEventsCount(EntityID entity) const
{
    if (active_effects.count(entity) == 0)
    {
        return 0;
    }

    return active_effects.at(entity);
}

Vector2f DrawHelper::WorldToScreen(const Vector2f& position) const
{
    return GetScreenSize() * (position - world_rect_.top_left) / world_rect_.size;
}

IVector2D DrawHelper::CovertFromScreen(const Vector2f& position) const
{
    const auto norm_screen_pos = position / GetScreenSize();
    const auto world_pos = world_rect_.top_left + norm_screen_pos * world_rect_.size;
    return IVector2D{static_cast<int>(world_pos.x), static_cast<int>(world_pos.y)};
}

float DrawHelper::ConvertEntityRadiusToScreen(int radius) const
{
    return WorldToScreen(world_->ToWorldPosition({radius, 0})).x - WorldToScreen(world_->ToWorldPosition({0, 0})).x;
}

EntityID DrawHelper::GetSelectedUnit(const HexGridPosition& mouse_position) const
{
    const std::vector<std::shared_ptr<Entity>>& all_entities = world_->GetAll();
    for (const std::shared_ptr<Entity>& entity_ptr : all_entities)
    {
        if (!entity_ptr->IsActive())
        {
            continue;
        }

        if (!EntityHelper::IsACombatUnit(*entity_ptr))
        {
            continue;
        }

        const PositionComponent& position_component = entity_ptr->Get<PositionComponent>();

        if (position_component.IsPositionsIntersecting(mouse_position, 0))
        {
            return entity_ptr->GetID();
        }
    }

    return kInvalidEntityID;
}
void DrawHelper::CloseWindow() const
{
    ::CloseWindow();
}

bool DrawHelper::IsWindowReady() const
{
    return ::IsWindowReady();
}

bool DrawHelper::WindowShouldClose() const
{
    return ::WindowShouldClose();
}

float DrawHelper::GetScreenWidth() const
{
    return static_cast<float>(::GetScreenWidth());
}

float DrawHelper::GetScreenHeight() const
{
    return static_cast<float>(::GetScreenHeight());
}

void DrawHelper::BeginFrame(const Rectangle& world_rect, const float zoom)
{
    world_rect_ = world_rect;
    zoom_ = zoom;
    ::BeginDrawing();
}

void DrawHelper::EndFrame()
{
    ::EndDrawing();
}

void DrawHelper::PreDrawFrame()
{
    active_effects.clear();

    for (auto& [_, layer_hexes] : drawn_layer_hexes)
    {
        layer_hexes.clear();
    }
}

void DrawHelper::RayFillHex(const Vector2f& position, float padding, const Color& color) const
{
    constexpr int hex_side_count = 6;
    constexpr int hex_rotation = 0;

    float radius = (GetScreenSize() * kGridCellSize / world_rect_.size).Min();
    radius -= padding;
    DrawPoly(ToRay(position), hex_side_count, radius, hex_rotation, ToRay(color));
}

void DrawHelper::DrawHexGroup(
    const simulation::HexGridPosition& group_center,
    int group_radius,
    const Color& fill_color,
    const float border_width,
    const Color& border_color) const
{
    const bool draw_border = border_width > 0;
    const bool draw_body = fill_color.a > 0;

    if (!(draw_border || draw_body))
    {
        return;
    }

    const float hex_radius = (GetScreenSize() * kGridCellSize / world_rect_.size).Min();

    if (group_radius == 0)
    {
        constexpr int hex_side_count = 6;
        constexpr float entity_hex_rotation = 30;
        const auto screen_center = CovertHexToScreen(group_center);
        CallRay(::DrawPoly, screen_center, hex_side_count, hex_radius, entity_hex_rotation, fill_color);

        if (draw_border)
        {
            CallRay(
                ::DrawPolyLinesEx,
                screen_center,
                hex_side_count,
                hex_radius - border_width / 2.f,
                entity_hex_rotation,
                border_width,
                border_color);
        }

        return;
    }

    static constexpr size_t kTopPoint = 0;
    static constexpr size_t kTopLeftPoint = 1;
    static constexpr size_t kBottomLeftPoint = 2;
    static constexpr size_t kBottomPoint = 3;
    static constexpr size_t kBottomRightPoint = 4;
    static constexpr size_t kTopRightPoint = 5;

    const float sqrt_3 = std::sqrt(3.f);
    const float hex_width = hex_radius * sqrt_3;
    const float dx = hex_width / 2;

    const auto hex_points_offsets = [&]()
    {
        const float dy = hex_radius / 2;
        std::array<Vector2f, 6> result;
        result[kTopPoint] = Vector2f{0, -hex_radius};
        result[kTopLeftPoint] = Vector2f{-dx, -dy};
        result[kTopRightPoint] = Vector2f{dx, -dy};
        result[kBottomLeftPoint] = Vector2f{-dx, dy};
        result[kBottomPoint] = Vector2f{0, hex_radius};
        result[kBottomRightPoint] = Vector2f{dx, dy};
        return result;
    }();

    auto hex_pt = [&hex_points_offsets](const Vector2f& offset, size_t index)
    {
        return offset + hex_points_offsets[index];
    };

    auto draw_triangle = [&](const Vector2f& a, const Vector2f& b, const Vector2f& c)
    {
        if (draw_body)
        {
            CallRay(::DrawTriangle, a, b, c, fill_color);
        }
    };

    auto draw_line = [&](const Vector2f& a, const Vector2f b)
    {
        if (draw_border)
        {
            CallRay(::DrawLineEx, a, b, border_width, border_color);
        }
    };

    // Make an array of hexagons corners positions that will be used more than once
    const auto corner = [&]()
    {
        const auto ring_right = CovertHexToScreen(group_center + kHexGridNeighboursOffsets[0] * group_radius);
        const auto ring_top_right = CovertHexToScreen(group_center + kHexGridNeighboursOffsets[1] * group_radius);
        const auto ring_top_left = CovertHexToScreen(group_center + kHexGridNeighboursOffsets[2] * group_radius);
        const auto ring_left = CovertHexToScreen(group_center + kHexGridNeighboursOffsets[3] * group_radius);
        const auto ring_bot_left = CovertHexToScreen(group_center + kHexGridNeighboursOffsets[4] * group_radius);
        const auto ring_bot_right = CovertHexToScreen(group_center + kHexGridNeighboursOffsets[5] * group_radius);
        return std::array<Vector2f, 12>{
            hex_pt(ring_right, kBottomRightPoint),
            hex_pt(ring_right, kTopRightPoint),
            hex_pt(ring_top_right, kTopRightPoint),
            hex_pt(ring_top_right, kTopPoint),
            hex_pt(ring_top_left, kTopPoint),
            hex_pt(ring_top_left, kTopLeftPoint),
            hex_pt(ring_left, kTopLeftPoint),
            hex_pt(ring_left, kBottomLeftPoint),
            hex_pt(ring_bot_left, kBottomLeftPoint),
            hex_pt(ring_bot_left, kBottomPoint),
            hex_pt(ring_bot_right, kBottomPoint),
            hex_pt(ring_bot_right, kBottomRightPoint),
        };
    }();

    // Generate points of big hexagon body
    const auto draw_points = [&]()
    {
        // An intersection of two lines assuming there is always an intersection
        auto intersection = [](const Vector2f& start_pos_1,
                               const Vector2f& end_pos_1,
                               const Vector2f& start_pos_2,
                               const Vector2f& end_pos_2)
        {
            const auto dir1 = end_pos_1 - start_pos_1;
            const auto dir2 = end_pos_2 - start_pos_2;
            const float div = dir2.y * dir1.x - dir2.x * dir1.y;
            const float fa = start_pos_2.x * end_pos_2.y - start_pos_2.y * end_pos_2.x;
            const float fb = start_pos_1.x * end_pos_1.y - start_pos_1.y * end_pos_1.x;
            const float xi = (dir1.x * fa - dir2.x * fb);
            const float yi = (dir1.y * fa - dir2.y * fb);
            return Vector2f{xi, yi} / div;
        };

        std::array<Vector2f, 6> r;
        for (size_t i = 0; i != 6; ++i)
        {
            const size_t a = i * 2;
            const size_t b = ((i + 1) * 2) % 12;
            r[i] = intersection(corner[a], corner[(a + 3) % 12], corner[b], corner[(b + 3) % 12]);
        }
        return r;
    }();

    // Draw big hexagon
    draw_triangle(draw_points[1], draw_points[2], draw_points[3]);
    draw_triangle(draw_points[0], draw_points[1], draw_points[3]);
    draw_triangle(draw_points[0], draw_points[3], draw_points[4]);
    draw_triangle(draw_points[5], draw_points[0], draw_points[4]);

    // Next draw precise sawtooths around the big hexagon

    if (fill_color.a == 255 && !draw_border)
    {
        // Non-transparent ones we can draw by drawing single ring hexagons over the big hexagon
        auto hex = group_center + kHexGridNeighboursOffsets[4] * group_radius;
        for (size_t i = 0; i < kHexGridNeighboursOffsets.size(); i++)
        {
            for (int j = 0; j < group_radius; j++)
            {
                CallRay(::DrawPoly, CovertHexToScreen(hex), 6, hex_radius, 0.f, fill_color);
                hex += kHexGridNeighboursOffsets[i];
            }
        }

        return;
    }

    // Semi-transparency require more work, otherwise there would be overlapping regions
    const auto jd = 1.1f * 2 * hex_radius / sqrt_3;
    const auto draw_side = [&](const size_t dpi0,
                               const size_t dpi1,
                               const size_t ci0,
                               const size_t ci1,
                               const size_t ni0,
                               const size_t ni1,
                               const size_t sti)
    {
        auto v = draw_points[dpi0] - draw_points[dpi1];
        const float dist = v.Length();
        v /= dist;

        draw_triangle(corner[ci0], draw_points[dpi0] - v * jd, draw_points[dpi0]);
        draw_triangle(corner[ci1], draw_points[dpi1], draw_points[dpi1] + v * jd);
        draw_line(corner[ci0], draw_points[dpi0] - v * jd);
        draw_line(corner[ci1], draw_points[dpi1] + v * jd);

        // Corner
        {
            const size_t a = ((dpi0 + 1) * 2) % 12;
            draw_triangle(draw_points[dpi0], corner[a], corner[a + 1]);
            draw_line(corner[a], corner[(a + 1) % 12]);
        }

        const auto corner_hex = group_center + kHexGridNeighboursOffsets[ni0] * group_radius;
        const float step = (dist - 2 * jd) / static_cast<float>(group_radius - 1);
        for (int i = 0; i != group_radius - 1; ++i)
        {
            const auto hex = corner_hex + kHexGridNeighboursOffsets[ni1] * (i + 1);
            const auto a = hex_pt(CovertHexToScreen(hex), sti);
            const auto b = draw_points[dpi1] + v * (jd + step * static_cast<float>(i + 0));
            const auto c = draw_points[dpi1] + v * (jd + step * static_cast<float>(i + 1));
            draw_triangle(a, b, c);
            draw_line(a, b);
            draw_line(a, c);
        }
    };

    for (size_t side_index = 0; side_index != 6; ++side_index)
    {
        const size_t dpi0 = side_index;
        const size_t dpi1 = (side_index + 1) % 6;
        const size_t ci0 = (3 + side_index * 2) % 12;
        const size_t ci1 = (4 + side_index * 2) % 12;
        const size_t ni0 = (2 + side_index) % 6;
        const size_t ni1 = (0 + side_index) % 6;
        const size_t sti = side_index;
        draw_side(dpi0, dpi1, ci0, ci1, ni0, ni1, sti);
    }
}

void DrawHelper::RayEntityBorderHex(const Vector2f& position, float radius, float line_thick, const Color& outline)
{
    constexpr int hex_side_count = 6;
    constexpr int entity_hex_rotation = 30;
    DrawPolyLinesEx(ToRay(position), hex_side_count, radius, entity_hex_rotation, line_thick, ToRay(outline));
}

}  // namespace simulation::tool
