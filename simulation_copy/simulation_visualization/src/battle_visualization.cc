#include "battle_visualization.h"

#include <chrono>
#include <utility>

#include "constants.h"
#include "draw_features.h"
#include "ecs/world.h"
#include "input.h"
#include "utility/grid_helper.h"

namespace simulation::tool
{

BattleVisualization::BattleVisualization(std::shared_ptr<World> world, bool exit_when_finished, bool space_to_time_step)
    : world_(std::move(world)),
      draw_helper_(world_),
      exit_when_finished_(exit_when_finished),
      space_to_time_step_(space_to_time_step)
{
    world_->SubscribeMethodToEvent<EventType::kTimeStepped>(this, &BattleVisualization::OnTimeStep);
    world_->SubscribeMethodToEvent<EventType::kBattleStarted>(this, &BattleVisualization::OnBattleStarted);

    auto add_feature =
        [this](std::unique_ptr<BaseDrawFeature> feature, const std::string_view name, const bool is_enabled = true)
    {
        feature->world_ = world_;
        feature->draw_helper_ = &draw_helper_;
        feature->feature_name_ = name;
        feature->is_enabled_ = is_enabled;
        feature->Initialize();
        draw_features_.emplace_back(std::move(feature));
    };

    // Order of drawing features
    add_feature(std::make_unique<DrawCenterLine>(), "Center Line");
    add_feature(std::make_unique<DebugDisplacement>(), "Displacement");
    add_feature(std::make_unique<DebugNavigation>(), "Navigation");
    add_feature(std::make_unique<DebugPathfinding>(), "Pathfinding", false);
    add_feature(std::make_unique<DrawCombatUnits>(), "Combat Units");
    add_feature(std::make_unique<DrawZone>(), "Zone", false);
    add_feature(std::make_unique<DrawBeams>(), "Beam");
    add_feature(std::make_unique<DrawChains>(), "Chain");
    add_feature(std::make_unique<DebugFocus>(), "Focus");
    add_feature(std::make_unique<DebugAbility>(), "Ability");
    add_feature(std::make_unique<DrawEventsLog>(), "EventsLog");
    add_feature(std::make_unique<DrawLiveStats>(), "LiveStats");
    add_feature(std::make_unique<DrawPlaneChange>(), "PlaneChange");
    add_feature(std::make_unique<DrawPositiveStates>(), "PositiveStates");
    add_feature(std::make_unique<DrawNegativeStates>(), "NegativeState");
    add_feature(std::make_unique<DrawConditionEffects>(), "ConditionEffects");
    add_feature(std::make_unique<DrawProjectiles>(), "Projectiles");
    add_feature(std::make_unique<DrawSynergy>(), "Synergy", false);
    add_feature(std::make_unique<DrawHyper>(), "Hyper", false);
    add_feature(std::make_unique<DrawAuras>(), "Auras", false);

    if (GetDrawHelper().GetDrawSettings().HasFeaturesSet())
    {
        for (const auto& feature : draw_features_)
        {
            feature->SetEnabled(GetDrawHelper().GetDrawSettings().IsFeatureEnabled(*feature));
        }
    }
}

BattleVisualization::~BattleVisualization()
{
    draw_helper_.GetDrawSettingsMutable().SetDrawFeaturesState(draw_features_);

    // Do not try to close windows when it's closed already
    if (!draw_helper_.IsWindowReady())
    {
        return;
    }

    if (!exit_when_finished_)
    {
        // Ask for exit before finishing
        while (!draw_helper_.WindowShouldClose())  // Detect window close button or ESC key
        {
            DrawWorld();
        }
    }

    draw_helper_.CloseWindow();
}

static Rectangle ComputeWorldCoordinatesRectangle(
    const World& world,
    const Vector2f camera,
    const Vector2f& screen_size,
    const float zoom)
{
    const HexGridConfig& grid_config = world.GetGridConfig();
    const auto min_world_position = Vector2f(world.ToWorldPosition(grid_config.GetMinHexGridPosition()));
    const auto max_world_position = Vector2f(world.ToWorldPosition(grid_config.GetMaxHexGridPosition()));
    const Vector2f aspects = Vector2f(screen_size.Max()) / screen_size;
    const Vector2f range = aspects.SwappedXY() * (max_world_position - min_world_position) / zoom;
    return Rectangle{camera - range / 2.f, range};
}

void BattleVisualization::OnTimeStep(const Event&)
{
    // Do not try to draw when windows is closed
    if (!draw_helper_.IsWindowReady())
    {
        return;
    }

    using namespace std::chrono;  // NOLINT

    // save begin time
    const steady_clock::time_point start_time = steady_clock::now();

    // measured in full screens per seconds (i.e. will be different depending on current zoom value)
    constexpr float camera_pan_rate = .5f;
    constexpr float camera_mouse_zoom_rate = 0.25f;
    constexpr float camera_keyboard_zoom_rate = 1.f;

    // Wait for Space key / Window Close input / Time for step drawing
    while (true)
    {
        const auto frame_start_time = high_resolution_clock::now();
        DrawWorld();
        const auto frame_duration = high_resolution_clock::now() - frame_start_time;
        const float frame_duration_seconds = duration_cast<duration<float>>(frame_duration).count();

        // zoom in
        if (Input::IsKeyDown(KeyboardKey::E))
        {
            zoom_ += frame_duration_seconds * camera_keyboard_zoom_rate;
        }

        zoom_ += Input::GetMouseWheelMove() * camera_mouse_zoom_rate;
        zoom_ = std::clamp(zoom_, 0.5f, 10.f);

        // zoom out
        if (Input::IsKeyDown(KeyboardKey::Q))
        {
            zoom_ -= frame_duration_seconds * camera_keyboard_zoom_rate;
        }

        // Pan
        {
            // Don't want to compute that on every frame because it will be rarely used
            std::optional<Rectangle> world_coord_range;
            auto get_coord_range = [&]() -> const Rectangle&
            {
                if (!world_coord_range)
                {
                    world_coord_range =
                        ComputeWorldCoordinatesRectangle(*world_, camera_, draw_helper_.GetScreenSize(), zoom_);
                }
                return *world_coord_range;
            };

            if (Input::IsKeyDown(KeyboardKey::W))
            {
                camera_.y -= frame_duration_seconds * camera_pan_rate * get_coord_range().Height();
            }

            if (Input::IsKeyDown(KeyboardKey::S))
            {
                camera_.y += frame_duration_seconds * camera_pan_rate * get_coord_range().Height();
            }

            if (Input::IsKeyDown(KeyboardKey::D))
            {
                camera_.x += frame_duration_seconds * camera_pan_rate * get_coord_range().Width();
            }

            if (Input::IsKeyDown(KeyboardKey::A))
            {
                camera_.x -= frame_duration_seconds * camera_pan_rate * get_coord_range().Width();
            }
        }

        // Allow to close window early
        if (draw_helper_.WindowShouldClose())
        {
            draw_helper_.CloseWindow();
            break;
        }

        if (Input::IsKeyPressed(KeyboardKey::Enter))
        {
            // Exit/Enter stepping state by using 'Enter'
            space_to_time_step_ = !space_to_time_step_;
            break;
        }

        if (space_to_time_step_)
        {
            if (Input::IsKeyPressed(KeyboardKey::Space))
            {
                // Exit loop drawing time step by space key
                break;
            }
        }
        else
        {
            if (GetDrawHelper().GetDrawSettings().GetPlaySpeedMs() <
                duration_cast<milliseconds>(steady_clock::now() - start_time).count())
            {
                break;
            }
        }
    }
}

void BattleVisualization::OnBattleStarted(const Event&)
{
    EnumMap<Team, std::vector<EntityID>> owners_combat_units_map{};

    for (const auto& entity : world_->GetAll())
    {
        owners_combat_units_map.GetOrAdd(entity->GetTeam()).push_back(entity->GetID());
    }

    const SynergiesStateContainer& synergies_state_container = world_->GetSynergiesStateContainer();
    for (const auto& [team, _] : owners_combat_units_map)
    {
        const auto& synergies_vector = synergies_state_container.GetTeamAllSynergies(team, true);
        std::string synergy_text;
        for (const SynergyOrder& synergy_order : synergies_vector)
        {
            fmt::format_to(
                std::back_inserter(synergy_text),
                "{}: {}\n",
                synergy_order.combat_synergy,
                synergy_order.synergy_stacks);
        }

        teams_synergies.emplace(team, synergy_text);
    }
}

void BattleVisualization::DrawWorld()
{
    draw_helper_.PreDrawFrame();

    draw_helper_.BeginFrame(
        ComputeWorldCoordinatesRectangle(*world_, camera_, draw_helper_.GetScreenSize(), zoom_),
        zoom_);
    draw_helper_.Clear(constants::color::RayWhite);

    DrawGrid();
    DrawFeaturesWorldPass();
    DrawMouseSelection();
    DrawInfo();
    DrawFeaturesUIPass();

    draw_helper_.EndFrame();
}

void BattleVisualization::DrawGrid() const
{
    const HexGridConfig& grid = world_->GetGridConfig();
    for (size_t index = 0; index < grid.GetGridSize(); index++)
    {
        HexGridPosition hex_pos = grid.GetCoordinates(index);
        GetDrawHelper().DrawFillHex(hex_pos, constants::color::LightGray, 1.0f);
    }
}

void BattleVisualization::DrawMouseSelection() const
{
    const Vector2f mouse_position = Input::GetMousePosition();
    const IVector2D world_position = GetDrawHelper().CovertFromScreen(mouse_position);
    const HexGridPosition hex_pos = world_->FromWorldPosition(world_position);
    if (!world_->GetGridConfig().IsInMapRectangleLimits(hex_pos))
    {
        return;
    }

    auto& draw_helper = GetDrawHelper();
    draw_helper.DrawFillHex(hex_pos, constants::color::DarkPurple);

    const float font_size = draw_helper.GetDefaultFontSize();
    draw_helper.DrawRectangle(mouse_position, Vector2f{100, font_size}, constants::color::RayWhite);
    const std::string hex_pos_str = fmt::format("{}", hex_pos);
    draw_helper.DrawText(
        hex_pos_str,
        Vector2f{mouse_position.x + 10.0f, mouse_position.y},
        constants::color::DarkGreen,
        font_size);
}

void BattleVisualization::DrawFeaturesWorldPass() const
{
    for (auto& feature : draw_features_)
    {
        if (feature->IsEnabled())
        {
            feature->Draw();
        }
    }
}

struct TextWithBackground
{
    void Draw(const Font& font, const std::string& text, const Vector2f& screen_size, const Vector2f& position) const
    {
        const Vector2f text_size = DrawHelper::MeasureTextSize(font, text, font_size);
        const Vector2f bg_size = text_size + margin * 2;

        // Try to adjust position to avoid clipping
        Vector2f bg_pos = position;
        bg_pos.x = std::min(bg_pos.x, screen_size.x - bg_size.x);
        bg_pos.x = std::max(bg_pos.x, 0.f);
        bg_pos.y = std::min(bg_pos.y, screen_size.y - bg_size.y);
        bg_pos.y = std::max(bg_pos.y, 0.f);

        DrawHelper::DrawRectangle(bg_pos, bg_size, bg_color);
        DrawHelper::DrawText(font, text, bg_pos + margin, fg_color, font_size, spacing);
    }

    Color fg_color{255, 255, 255, 255};
    Color bg_color{};
    Vector2f margin{};
    float font_size = 14.f;
    float spacing = 1.f;
};

void BattleVisualization::DrawFeaturesUIPass() const
{
    float padding = 10.0f;
    constexpr float height = 20.0f;
    Rectangle background_rect{};
    background_rect.size.x = 160.f;
    background_rect.size.y = height * static_cast<float>(draw_features_.size()) + 2 * padding;
    DrawHelper::DrawRectangle(background_rect, Color{255, 255, 255, 200});
    for (auto& feature : draw_features_)
    {
        feature->DrawCheckBox(Rectangle{{10, padding}, Vector2f(18.f)});
        padding += height;
    }

    for (auto& feature : draw_features_)
    {
        if (feature->IsEnabled())
        {
            feature->DrawUI();
        }
    }

    const Font& font = GetDrawHelper().GetFont(true);

    // Tooltip for selected unit
    const Vector2f mouse_position = Input::GetMousePosition();
    const IVector2D world_position = GetDrawHelper().CovertFromScreen(mouse_position);
    const HexGridPosition hex_pos = world_->FromWorldPosition(world_position);
    const Vector2f& screen_size = GetDrawHelper().GetScreenSize();
    if (world_->GetGridConfig().IsInMapRectangleLimits(hex_pos))
    {
        const EntityID selected_unit_id = GetDrawHelper().GetSelectedUnit(hex_pos);
        if (selected_unit_id != kInvalidEntityID)
        {
            std::string text;

            // Gather tooltip text from all features
            for (auto& feature : draw_features_)
            {
                if (feature->IsEnabled())
                {
                    feature->TooltipForSelectedEntity(selected_unit_id, &text);
                }
            }

            if (!text.empty())
            {
                constexpr TextWithBackground text_with_background{
                    .fg_color = constants::color::Black,
                    .bg_color = {255, 235, 183, 230},
                    .margin = {5, 5},
                    .font_size = 20.f,
                };

                text_with_background.Draw(font, text, screen_size, mouse_position + Vector2f{0.f, 15.f});
            }
        }
    }
}

void BattleVisualization::DrawInfo() const
{
    auto& draw_helper = GetDrawHelper();
    draw_helper.DrawText(
        fmt::format("Step number {}", world_->GetTimeStepCount()),
        Vector2f{GetDrawHelper().GetScreenWidth() / 2.0f, 20.0f},
        constants::color::Red,
        24);

    constexpr int offset = 100;
    constexpr int size = 14;
    constexpr int spacing = 4;

    const float text_left = draw_helper.GetScreenWidth() - offset;
    const float color_box = text_left - size - spacing;

    draw_helper.DrawRectangle(
        {color_box - spacing, size - spacing},
        {offset + size + spacing * 2, (size + spacing) * 2 + spacing},
        constants::color::DarkGray);

    float top = size;
    for (const Team team : EnumSet<Team>::MakeFull())
    {
        const Color color = draw_helper.GetTeamColorSchema(team).hex_fill;
        draw_helper.DrawText(Enum::TeamToString(team), Vector2f{text_left, top}, color);
        draw_helper.DrawRectangle({color_box, top}, {size, size}, color);
        top += size + spacing;
    }

    // Draw world config data
    top += 10.0f;

    constexpr Color text_color = {36, 55, 99, 255};
    const std::string random_seed_text = fmt::format("RandomSeed: {}", world_->GetBattleConfig().random_seed);
    draw_helper.DrawText(random_seed_text, {color_box, top}, text_color);

    DrawSynergyInfo();
}

void BattleVisualization::DrawSynergyInfo() const
{
    constexpr float h_offset = 5.0f;
    constexpr float v_offset = 120.0f;
    constexpr float height = 200.0f;
    constexpr float width = 120.0f;

    constexpr Color text_color = {36, 55, 99, 255};
    constexpr Color bg_color = {175, 229, 226, 210};
    constexpr Vector2f bg_size = {width, height};

    for (const auto& [team, synergy_text] : teams_synergies)
    {
        Vector2f position = {h_offset, GetDrawHelper().GetScreenHeight() - height - v_offset};

        if (team == Team::kRed)
        {
            position.x = position.x + GetDrawHelper().GetScreenWidth() - width - 2.0f * h_offset;
        }

        GetDrawHelper().DrawBorderedText(synergy_text, position, bg_size, text_color, bg_color);
    }
}

}  // namespace simulation::tool
