#include "draw_features.h"

#include "components/aura_component.h"
#include "components/beam_component.h"
#include "components/combat_synergy_component.h"
#include "components/combat_unit_component.h"
#include "components/displacement_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/projectile_component.h"
#include "components/stats_component.h"
#include "components/zone_component.h"
#include "constants.h"
#include "data/containers/game_data_container.h"
#include "draw_helper.h"
#include "ecs/event.h"
#include "ecs/world.h"
#include "input.h"
#include "ui.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/hyper_helper.h"
#include "utility/string.h"

namespace simulation::tool
{

void BaseDrawFeature::DrawCheckBox(const Rectangle& rect)
{
    SetEnabled(ui::CheckBox(*draw_helper_, rect, feature_name_, IsEnabled()));
}

const Color& BaseDrawFeature::GetRandomColor(EntityID entity_id) const
{
    // Try find assigned colors
    auto selected_color = assigned_colors_.find(entity_id);
    if (selected_color == assigned_colors_.end())
    {
        // Generate new color for entity_id
        Color fill_color = GetDrawHelper().GenerateRandomColor();
        fill_color.a = 50;
        const auto& insert_result = assigned_colors_.insert({entity_id, fill_color});
        assert(insert_result.second);
        selected_color = insert_result.first;
    }

    return selected_color->second;
}

size_t BaseDrawFeature::GetRandomColorIndex(EntityID entity_id) const
{
    const auto selected_color = assigned_colors_.find(entity_id);
    if (selected_color == assigned_colors_.end())
    {
        return 0;
    }

    return static_cast<size_t>(std::distance(assigned_colors_.begin(), selected_color));
}

void DrawCombatUnits::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            if (EntityHelper::IsACombatUnit(entity))
            {
                GetDrawHelper().DrawCombatUnit(entity);
            }
        });
}

void DebugDisplacement::Initialize()
{
    GetWorld().SubscribeMethodToEvent<EventType::kDisplacementStarted>(this, &Self::OnDisplacementStarted);
    GetWorld().SubscribeMethodToEvent<EventType::kDisplacementStopped>(this, &Self::OnDisplacementStopped);
}

void DebugDisplacement::Draw()
{
    for (auto [entity_id, debug_data] : active_displacements_)
    {
        DrawDisplacement(entity_id, debug_data);
    }
}

void DebugDisplacement::OnDisplacementStarted(const event_data::Displacement& displacement_data)
{
    const EntityID receiver_id = displacement_data.receiver_id;
    const Entity& receiver = GetWorld().GetByID(receiver_id);
    const auto& position_comp = receiver.Get<PositionComponent>();
    const auto& displacement_comp = receiver.Get<DisplacementComponent>();

    DisplacementDebugData debug_data;
    debug_data.sender = displacement_data.sender_id;
    debug_data.started_pos = position_comp.GetPosition();
    debug_data.target_pos = displacement_comp.GetDisplacementTargetPosition();
    debug_data.type = displacement_comp.GetDisplacementType();

    active_displacements_.emplace(receiver_id, debug_data);
}

void DebugDisplacement::OnDisplacementStopped(const event_data::Displacement& displacement_data)
{
    active_displacements_.erase(displacement_data.receiver_id);
}

void DebugDisplacement::DrawDisplacement(EntityID receiver, const DisplacementDebugData& data) const
{
    const Entity& receiver_entity = GetWorld().GetByID(receiver);
    const auto& position_comp = receiver_entity.Get<PositionComponent>();
    const int receiver_radius = position_comp.GetRadius();

    GetDrawHelper().DrawFillHexes(data.started_pos, receiver_radius, constants::color::Lime);
    GetDrawHelper().DrawFillHexes(data.target_pos, receiver_radius, constants::color::Lime);
    GetDrawHelper().DrawHexGroupOutline(position_comp.GetPosition(), receiver_radius, constants::color::Green, 2.f);
    GetDrawHelper().DrawArrow(data.started_pos, data.target_pos, 4, constants::color::DarkGreen);
    GetDrawHelper().DrawText(
        fmt::format("{}", data.type),
        GetDrawHelper().CovertHexToScreen(data.target_pos),
        constants::color::White);
}

void DebugFocus::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            const auto focus_component = entity.Get<FocusComponent>();
            const auto& position_component = entity.Get<PositionComponent>();
            if (focus_component.IsFocusSet())
            {
                GetDrawHelper().DrawArrowPos(
                    position_component,
                    focus_component.GetFocus()->Get<PositionComponent>(),
                    2.0f,
                    GetDrawHelper().GetEntityColorSchema(entity).hex_fill);
            }

            const std::unordered_set<EntityID>& unreachable_entity_set = focus_component.GetUnreachable();
            if (!unreachable_entity_set.empty())
            {
                const Vector2f text_position = GetDrawHelper().CovertHexToScreen(position_component.GetPosition());
                const std::string string = fmt::format("Unreachable: {}", unreachable_entity_set);
                GetDrawHelper().DrawText(string, text_position + Vector2f{10, 10}, constants::color::Black);
            }
        });
}

void DebugAbility::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            DrawAbilitiesProgress(entity);
        });
}

void DebugAbility::DrawAbilitiesProgress(const Entity& entity) const
{
    const auto& ability_component = entity.Get<AbilitiesComponent>();
    if (!ability_component.HasActiveAbility())
    {
        return;
    }

    const auto& position_component = entity.Get<PositionComponent>();
    const Vector2f entity_screen_pos = GetDrawHelper().CovertHexToScreen(position_component.GetPosition());

    // Ability progress
    const AbilityStatePtr& ability_state = ability_component.GetActiveAbility();
    const int current_ability_time_ms = ability_state->GetCurrentAbilityTimeMs();
    const int total_duration_ms = ability_state->total_duration_ms;
    const float ability_progress = static_cast<float>(current_ability_time_ms) / static_cast<float>(total_duration_ms);

    // Draw progress bar
    constexpr Color bg_color = {232, 196, 196, 255};

    Rectangle progress_bar_rect{entity_screen_pos + Vector2f{-20.f, 15.f}, {40.0f, 8.0f}};
    Color fill_color = {198, 151, 73, 255};
    Color border_color = {101, 100, 124, 255};

    if (ability_state->ability_type == AbilityType::kOmega)
    {
        progress_bar_rect = Rectangle{entity_screen_pos + Vector2f{-30.0f, 15.0f}, {60.0f, 10.0f}};
        fill_color = Color{196, 30, 104, 255};
        border_color = {255, 50, 50, 255};
    }
    else if (ability_state->ability_type == AbilityType::kInnate)
    {
        fill_color = Color{96, 190, 104, 255};
        border_color = {30, 255, 30, 255};
    }

    constexpr float border = 2.0f;
    ui::ProgressBarBorder(progress_bar_rect, ability_progress, fill_color, bg_color, border, border_color);
}

void DrawZone::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            if (!EntityHelper::IsAZone(entity))
            {
                return;
            }

            Draw(entity);
        });
}

void DrawZone::Draw(const Entity& entity) const
{
    const auto& zone_component = entity.Get<ZoneComponent>();
    const ZoneEffectShape shape = zone_component.GetShape();

    // Find assigned colors
    const Color& selected_color = GetRandomColor(entity.GetID());

    switch (shape)
    {
    case ZoneEffectShape::kHexagon:
        DrawHexagonZone(entity, zone_component, selected_color);
        break;
    case ZoneEffectShape::kTriangle:
        DrawTriangleZone(entity, zone_component, selected_color);
        break;
    case ZoneEffectShape::kRectangle:
        DrawRectangleZone(entity, zone_component, selected_color);
        break;
    default:
        GetWorld()
            .Log("DebugZone", LogLevel::kWarn, "- zone effect shape is not supported in visualization: {}", shape);
    }

    const auto& position_component = entity.Get<PositionComponent>();
    const std::string message =
        fmt::format("ZoneID {}, Context {}", entity.GetID(), zone_component.GetSourceContext().ability_name);
    Vector2f screen_pos = GetDrawHelper().CovertHexToScreen(position_component.GetPosition());
    screen_pos.y += 20.0f * static_cast<float>(GetRandomColorIndex(entity.GetID()));
    GetDrawHelper().DrawText(message, screen_pos, constants::color::Black);
}

void DrawZone::DrawHexagonZone(const Entity& entity, const ZoneComponent& zone_component, const Color& color) const
{
    const auto& position_component = entity.Get<PositionComponent>();
    GetDrawHelper().DrawFillHexes(position_component.GetPosition(), zone_component.GetRadiusUnits(), color);
}

void DrawZone::DrawTriangleZone(const Entity& entity, const ZoneComponent& zone_component, const Color& color) const
{
    const auto& position_component = entity.Get<PositionComponent>();

    // Angle the zone is facing relative to its position
    const int direction_degrees =
        Math::AngleLimitTo360(zone_component.GetSpawnDirectionDegrees() + zone_component.GetDirectionDegrees());
    const IVector2D world_source = GetWorld().ToWorldPosition(position_component.GetPosition());
    const int world_radius = GetWorld().ToWorldPosition(zone_component.GetRadiusUnits());

    // Calculate vertices for triangle ABC with A at source
    IVector2D triangle_a, triangle_b, triangle_c;
    world_source.GetTriangleVertices(direction_degrees, world_radius, &triangle_a, &triangle_b, &triangle_c);

    const Vector2f v1 = GetDrawHelper().WorldToScreen(triangle_a);
    const Vector2f v2 = GetDrawHelper().WorldToScreen(triangle_b);
    const Vector2f v3 = GetDrawHelper().WorldToScreen(triangle_c);

    GetDrawHelper().DrawTriangle(v2, v1, v3, color);
    GetDrawHelper().DrawTriangleLines(v2, v1, v3, DrawHelper::ColorScale(color, 0.3f));
}

void DrawZone::DrawRectangleZone(const Entity& entity, const ZoneComponent& zone_component, const Color& color) const
{
    const auto& position_component = entity.Get<PositionComponent>();

    const int width = zone_component.GetWidthUnits();
    const int height = zone_component.GetHeightUnits();

    // NOTE: it's interpretation of what is implemented in collision check
    const HexGridPosition center = position_component.GetPosition();
    const HexGridPosition a{center.q - width / 2, center.r - height / 2};
    const HexGridPosition b{center.q - width / 2, center.r + height / 2};
    const HexGridPosition c{center.q + width / 2, center.r + height / 2};
    const HexGridPosition d{center.q + width / 2, center.r - height / 2};

    const auto v1 = GetDrawHelper().CovertHexToScreen(a);
    const auto v2 = GetDrawHelper().CovertHexToScreen(b);
    const auto v3 = GetDrawHelper().CovertHexToScreen(c);
    const auto v4 = GetDrawHelper().CovertHexToScreen(d);

    // Draw using two triangle
    GetDrawHelper().DrawTriangle(v1, v2, v3, color);
    GetDrawHelper().DrawTriangle(v1, v3, v4, color);

    // Draw using lines
    constexpr float line_thick = 2.0f;
    const Color line_color = DrawHelper::ColorScale(color, 0.3f);
    GetDrawHelper().DrawLine(v1, v2, line_thick, line_color);
    GetDrawHelper().DrawLine(v2, v3, line_thick, line_color);
    GetDrawHelper().DrawLine(v3, v4, line_thick, line_color);
    GetDrawHelper().DrawLine(v4, v1, line_thick, line_color);
}

void DebugNavigation::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            if (!entity.Has<MovementComponent>() || !entity.Has<PositionComponent>())
            {
                return;
            }

            const auto& movement_component = entity.Get<MovementComponent>();
            if (movement_component.GetMovementType() == MovementComponent::MovementType::kNavigation)
            {
                DrawNavigationInfo(entity);
            }

            const auto& position_component = entity.Get<PositionComponent>();
            if (position_component.HasReservedPosition())
            {
                DrawReservedPosition(entity);
            }
        });
}

void DebugNavigation::DrawNavigationInfo(const Entity& entity) const
{
    const auto& movement_component = entity.Get<MovementComponent>();

    if (!movement_component.HasWaypoints())
    {
        return;
    }

    // Find assigned colors
    const Color& selected_color = GetRandomColor(entity.GetID());

    const auto& position_component = entity.Get<PositionComponent>();
    HexGridPosition current_position = position_component.GetPosition();

    const std::list<HexGridPosition>& waypoints = movement_component.GetVisualisationWaypoints();
    for (const auto& waypoint : waypoints)
    {
        GetDrawHelper().DrawFillHexes(waypoint, position_component.GetRadius(), selected_color);

        GetDrawHelper().DrawArrow(current_position, waypoint, 1.0, selected_color);

        current_position = waypoint;
    }
}

void DebugNavigation::DrawReservedPosition(const Entity& entity) const
{
    const auto& position_component = entity.Get<PositionComponent>();

    constexpr Color reserved_color = {255, 109, 174, 150};
    GetDrawHelper().DrawFillHexes(
        position_component.GetReservedPosition(),
        position_component.GetRadius(),
        reserved_color);
}

void DebugPathfinding::Initialize()
{
    GetWorld().SubscribeMethodToEvent<EventType::kPathfindingDebugData>(this, &Self::OnDebugPathfinding);
}

void DebugPathfinding::Draw()
{
    if (!events_.empty())
    {
        DrawSelectedEvent();
    }
}

void DebugPathfinding::DrawUI()
{
    if (!events_.empty())
    {
        DrawEventSelection();
    }
}

void DebugPathfinding::OnDebugPathfinding(const event_data::PathfindingDebugData& data)
{
    if (events_time_step_ != GetWorld().GetTimeStepCount())
    {
        events_time_step_ = GetWorld().GetTimeStepCount();

        events_.clear();
        event_texts_.clear();
    }

    events_.push_back(data);

    const HexGridConfig& grid_config = GetWorld().GetGridConfig();
    const HexGridPosition start_coords = grid_config.GetCoordinates(data.start_node);
    const HexGridPosition end_coords = grid_config.GetCoordinates(data.target_node);
    event_texts_.push_back(fmt::format("from {} to {}", start_coords, end_coords));
}

void DebugPathfinding::DrawEventSelection()
{
    constexpr float selector_width = 320.0f;
    auto& draw_helper = GetDrawHelper();
    const Vector2f screen_size = draw_helper.GetScreenSize();
    const Rectangle rect{{screen_size.x / 2.0f - selector_width / 2.0f, 50.0f}, {selector_width, 20.0f}};
    selected_event_ = ui::ComboBox(draw_helper, rect, event_texts_, selected_event_);
}

void DebugPathfinding::DrawSelectedEvent() const
{
    if (selected_event_ >= events_.size())
    {
        return;
    }

    const HexGridConfig& grid_config = GetWorld().GetGridConfig();

    const event_data::PathfindingDebugData& event_data = events_.at(selected_event_);

    DrawObstacles(event_data.source_radius, event_data.obstacles);

    // Fill all visited nodes
    for (size_t i = 0; i < event_data.path_graph.size(); i++)
    {
        const AStarGraphNode& node = event_data.path_graph[i];
        if (node.goal != 0)
        {
            const HexGridPosition node_position = grid_config.GetCoordinates(i);
            GetDrawHelper().DrawFillHex(node_position, constants::color::Green);
        }
    }

    // Draw all relations
    for (size_t i = 0; i < event_data.path_graph.size(); i++)
    {
        const AStarGraphNode& node = event_data.path_graph[i];
        if (node.goal != 0)
        {
            DrawNodeParentLine(i, node.parent_index, constants::color::Black, 1.0f);
        }
    }

    // Draw path if we it was found
    const bool path_found = event_data.reached_node != kInvalidIndex;
    if (path_found)
    {
        size_t current_node = event_data.reached_node;
        while (current_node != event_data.start_node)
        {
            const AStarGraphNode& current_node_data = event_data.path_graph.at(current_node);
            DrawNodeParentLine(current_node, current_node_data.parent_index, constants::color::Black, 3.0f);
            current_node = current_node_data.parent_index;
        }
    }

    const HexGridPosition start_coords = grid_config.GetCoordinates(event_data.start_node);
    const HexGridPosition target_coords = grid_config.GetCoordinates(event_data.target_node);
    const HexGridPosition reached_coords = grid_config.GetCoordinates(event_data.reached_node);

    GetDrawHelper().DrawFillHex(start_coords, constants::color::Red);
    GetDrawHelper().DrawFillHex(target_coords, constants::color::Red);
    GetDrawHelper().DrawFillHex(reached_coords, constants::color::Blue);
    GetDrawHelper().DrawArrow(start_coords, target_coords, 1.0, constants::color::Red);
}

void DebugPathfinding::DrawObstacles(int source_radius, const ObstaclesMapType& obstacles) const
{
    const HexGridConfig& grid_config = GetWorld().GetGridConfig();
    for (size_t index = 0; index < grid_config.GetGridSize(); index++)
    {
        HexGridPosition hex_pos = grid_config.GetCoordinates(index);

        // Check obstacles and bounds
        if (obstacles.at(index) > 0 || !grid_config.IsHexagonInGridLimits(hex_pos, source_radius))
        {
            GetDrawHelper().DrawFillHex(hex_pos, constants::color::Gray, 3.0f);
        }
    }
}

void DebugPathfinding::DrawNodeParentLine(size_t node, size_t parent, const Color& color, float thick) const
{
    if (node == kInvalidIndex || parent == kInvalidIndex)
    {
        return;
    }

    const HexGridConfig& grid_config = GetWorld().GetGridConfig();

    const HexGridPosition node_position = grid_config.GetCoordinates(node);
    const Vector2f screen_node_position = GetDrawHelper().CovertHexToScreen(node_position);

    const HexGridPosition parent_node_position = grid_config.GetCoordinates(parent);
    const Vector2f screen_parent_node_position = GetDrawHelper().CovertHexToScreen(parent_node_position);

    GetDrawHelper().DrawLine(screen_node_position, screen_parent_node_position, thick, color);
}

void DrawEventsLog::Initialize()
{
    GetWorld().SubscribeMethodToEvent<EventType::kOnDamage>(this, &Self::OnDamageEvent);
    GetWorld().SubscribeMethodToEvent<EventType::kOnHealthGain>(this, &Self::OnHealthGainEvent);
}

void DrawEventsLog::Draw()
{
    const auto& draw_helper = GetDrawHelper();
    const Vector2f screen_size = draw_helper.GetScreenSize();
    const Vector2f bottom_left = {100.0f, screen_size.y - 20.0f};

    {
        const Rectangle expand_check_bounds{{bottom_left.x - 80.0f, bottom_left.y}, Vector2f{15.0f}};
        is_expanded_ = ui::CheckBox(draw_helper, expand_check_bounds, "Expand", is_expanded_);
    }

    const size_t draw_items = is_expanded_ ? kMaxQueueLength : 5;
    const float draw_items_f = static_cast<float>(draw_items);

    const auto& font = draw_helper.GetFont(true);

    constexpr float margin = 2.0f;
    const float text_height = DrawHelper::MeasureTextSize(font, "A", kFontSize).y;
    constexpr float text_spacing = 1.f;
    const float row_height = text_height + text_spacing;
    const Rectangle damage_console_rect{
        .top_left = bottom_left - (Vector2f{0, row_height * (draw_items_f - 1)} + margin),
        .size = {screen_size.x - 2.0f * bottom_left.x + margin * 2.0f, row_height * draw_items_f + margin * 2.0f}};

    DrawHelper::DrawRectangle(damage_console_rect, constants::color::Black);

    for (size_t i = 0; i < events_queue_.size() && i < draw_items; ++i)
    {
        const Vector2f pos = {bottom_left.x, bottom_left.y - row_height * static_cast<float>(i)};
        const size_t event_index = (events_queue_.size() - i - 1);
        const auto& event_id = events_queue_[event_index];

        if (damage_events_.contains(event_id))
        {
            DrawEventDebugData(pos, damage_events_[event_id]);
            continue;
        }

        if (heal_events_.contains(event_id))
        {
            DrawEventDebugData(pos, heal_events_[event_id]);
            continue;
        }
    }
}

void DrawEventsLog::PopOverflowingEvents()
{
    while (GetTotalEventsCount() > kMaxQueueLength)
    {
        const size_t event_id = events_queue_.front();
        events_queue_.pop_front();

        damage_events_.erase(event_id);
        heal_events_.erase(event_id);
    }
}

void DrawEventsLog::OnDamageEvent(const event_data::AppliedDamage& event)
{
    const size_t event_id = next_event_id_++;
    damage_events_[event_id] = FillEventDebugData(event);
    events_queue_.push_back(event_id);
    PopOverflowingEvents();
}

void DrawEventsLog::OnHealthGainEvent(const event_data::HealthGain& event)
{
    const size_t event_id = next_event_id_++;
    heal_events_[event_id] = FillEventDebugData(event);
    events_queue_.push_back(event_id);
    PopOverflowingEvents();
}

DrawEventsLog::DamageDebugData DrawEventsLog::FillEventDebugData(const event_data::AppliedDamage& event) const
{
    DamageDebugData debug_data;
    debug_data.time_step = GetWorld().GetTimeStepCount();
    debug_data.event = event;

    if (EntityHelper::IsASynergy(GetWorld(), event.sender_id))
    {
        debug_data.sender_name = fmt::format("{}(ID: {})", event.source_context.combat_synergy, event.sender_id);
    }
    else
    {
        const auto& sender_combat_unit_entity = GetWorld().GetByID(event.combat_unit_sender_id);
        const auto& sender_combat_unit_component = sender_combat_unit_entity.Get<CombatUnitComponent>();
        debug_data.sender_name = fmt::format(
            "{}{}(ID: {})",
            sender_combat_unit_component.GetTypeID().line_name,
            sender_combat_unit_component.GetTypeID().stage,
            debug_data.event.combat_unit_sender_id);
    }

    debug_data.receiver_name = "Non Unit";
    const auto& receiver_entity = GetWorld().GetByID(debug_data.event.receiver_id);
    if (receiver_entity.Has<CombatUnitComponent>())
    {
        const auto& receiver_combat_unit_component = receiver_entity.Get<CombatUnitComponent>();
        debug_data.receiver_name = fmt::format(
            "{}{}",
            receiver_combat_unit_component.GetTypeID().line_name,
            receiver_combat_unit_component.GetTypeID().stage);
    }

    return debug_data;
}

DrawEventsLog::HealDebugData DrawEventsLog::FillEventDebugData(const event_data::HealthGain& event) const
{
    HealDebugData debug_data;
    debug_data.time_step = GetWorld().GetTimeStepCount();
    debug_data.event = event;

    if (EntityHelper::IsASynergy(GetWorld(), event.caused_by_id))
    {
        debug_data.sender_name = fmt::format("{}(ID: {})", event.source_context.combat_synergy, event.caused_by_id);
    }
    else
    {
        const auto& sender_combat_unit_entity = GetWorld().GetByID(event.caused_by_combat_unit_id);
        const auto& sender_combat_unit_component = sender_combat_unit_entity.Get<CombatUnitComponent>();
        debug_data.sender_name = fmt::format(
            "{}{}(ID: {})",
            sender_combat_unit_component.GetTypeID().line_name,
            sender_combat_unit_component.GetTypeID().stage,
            debug_data.event.caused_by_combat_unit_id);
    }

    debug_data.receiver_name = "Non Unit";
    const auto& receiver_entity = GetWorld().GetByID(debug_data.event.entity_id);
    if (receiver_entity.Has<CombatUnitComponent>())
    {
        const auto& receiver_combat_unit_component = receiver_entity.Get<CombatUnitComponent>();
        debug_data.receiver_name = fmt::format(
            "{}{}",
            receiver_combat_unit_component.GetTypeID().line_name,
            receiver_combat_unit_component.GetTypeID().stage);
    }

    return debug_data;
}

void DrawEventsLog::DrawEventDebugData(const Vector2f& position, const DamageDebugData& data) const
{
    auto& font = GetDrawHelper().GetFont(true);
    auto draw_text = [&, cursor = position]<typename... Args>(
                         float minimal_offset,
                         const Color& color,
                         fmt::format_string<Args...> format,
                         Args&&... args) mutable
    {
        buffer_.clear();
        String::FormatTo(buffer_, format, std::forward<Args>(args)...);
        DrawHelper::DrawText(font, buffer_, cursor, color, kFontSize, 1);
        const Vector2f text_size = DrawHelper::MeasureTextSize(font, buffer_, kFontSize);
        cursor.x += std::max(minimal_offset, text_size.x);
    };

    const Color sender_color = GetDrawHelper().GetEntityColorSchema(data.event.sender_id).damage_info;
    const FixedPoint stat_scale = GetWorld().GetStatsConstantsScale();
    draw_text(50, constants::color::White, "{:>4} ", data.time_step);
    draw_text(150, constants::color::Red, "{} {} ", data.event.damage_value / stat_scale, data.event.damage_type);
    draw_text(370, sender_color, "{} > {}(ID: {}) ", data.sender_name, data.receiver_name, data.event.receiver_id);
    draw_text(425, constants::color::White, "{}", data.event.source_context);
}

void DrawEventsLog::DrawEventDebugData(const Vector2f& position, const HealDebugData& data) const
{
    auto& font = GetDrawHelper().GetFont(true);
    auto draw_text = [&, cursor = position]<typename... Args>(
                         float minimal_offset,
                         const Color& color,
                         fmt::format_string<Args...> format,
                         Args&&... args) mutable
    {
        buffer_.clear();
        String::FormatTo(buffer_, format, std::forward<Args>(args)...);
        DrawHelper::DrawText(font, buffer_, cursor, color, kFontSize, 1);
        const Vector2f text_size = DrawHelper::MeasureTextSize(font, buffer_, kFontSize);
        cursor.x += std::max(minimal_offset, text_size.x);
    };

    const Color sender_color = GetDrawHelper().GetEntityColorSchema(data.event.caused_by_id).damage_info;
    const FixedPoint stat_scale = GetWorld().GetStatsConstantsScale();
    draw_text(50, constants::color::White, "{:>4}", data.time_step);
    draw_text(150, constants::color::Green, "{} {} ", data.event.gain / stat_scale, data.event.health_gain_type);
    draw_text(370, sender_color, "{} > {}(ID: {}) ", data.sender_name, data.receiver_name, data.event.entity_id);
    draw_text(425, constants::color::White, "{}", data.event.source_context);
}

void DrawLiveStats::TooltipForSelectedEntity(const EntityID entity_id, std::string* text)
{
    auto& draw_helper = GetDrawHelper();
    const StatsData stats_data = GetWorld().GetLiveStats(entity_id);
    const Entity& unit_entity = GetWorld().GetByID(entity_id);

    const AttachedEffectBuffsState* buffs_state = nullptr;
    if (const auto* attached_effects_component = unit_entity.GetPtr<AttachedEffectsComponent>())
    {
        buffs_state = &attached_effects_component->GetBuffsState();
    }

    String::AppendLineAndFormat(*text, "Shield: {}", GetWorld().GetShieldTotal(unit_entity));

    auto print_effects = [&](const std::string_view prefix,
                             const StatType stat,
                             const std::unordered_map<StatType, std::vector<AttachedEffectStatePtr>>& effect_map)
    {
        if (effect_map.contains(stat))
        {
            for (const auto& effect_state : effect_map.at(stat))
            {
                String::AppendLineAndFormat(*text, "    {}{} from ", prefix, effect_state->GetCapturedValue());
                GetWorld().BuildLogPrefixFor(effect_state->sender_id, text);
            }
        }
    };

    auto print_stat = [&](const StatType stat)
    {
        String::AppendLineAndFormat(*text, "{}: {}", stat, stats_data.Get(stat));
        if (buffs_state && (buffs_state->buffs.contains(stat) || buffs_state->debuffs.contains(stat)))
        {
            if (const StatsComponent* stats_component = unit_entity.GetPtr<StatsComponent>())
            {
                const FixedPoint base = stats_component->GetBaseStats().Get(stat);
                if (base != 0_fp)
                {
                    String::AppendLineAndFormat(*text, "    {} base", base);
                }
            }

            print_effects("+", stat, buffs_state->buffs);
            print_effects("-", stat, buffs_state->debuffs);
        }
    };

    if (Input::IsKeyDown(KeyboardKey::ShiftLeft))
    {
        for (const auto stat_type : EnumSet<StatType>::MakeFull())
        {
            print_stat(stat_type);
        }
    }
    else
    {
        for (const auto stat_type : draw_helper.GetDrawSettings().GetEnabledStats())
        {
            print_stat(stat_type);
        }
    }

    if (auto position_component = unit_entity.GetPtr<PositionComponent>())
    {
        String::AppendLineAndFormat(*text, "Radius: {}", position_component->GetRadius());
    }
}

void DrawPlaneChange::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            // Only for combat units
            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

            // Gets all the active plane changes
            const auto& active_plane_changes = attached_effects_component.GetPlaneChanges();
            if (active_plane_changes.empty())
            {
                return;
            }

            constexpr Color bg_color = {202, 240, 248, 230};
            for (const auto& [plane_change_type, effects] : active_plane_changes)
            {
                GetDrawHelper().DrawAttachedEffect(
                    entity,
                    "PlaneChange",
                    Enum::EffectPlaneChangeToString(plane_change_type),
                    effects,
                    bg_color);
            }
        });
}

void DrawPositiveStates::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            // Only for combat units
            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

            // Draw all active positive states
            constexpr Color bg_color = {255, 229, 236, 230};
            for (const auto& [effect_type, effects] : attached_effects_component.GetPositiveStates())
            {
                GetDrawHelper().DrawAttachedEffect(
                    entity,
                    "PositiveState",
                    Enum::EffectPositiveStateToString(effect_type),
                    effects,
                    bg_color);
            }
        });
}

void DrawNegativeStates::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            // Only for combat units
            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

            // Gets all the active negative states
            const auto& active_negative_states = attached_effects_component.GetNegativeStates();
            if (active_negative_states.empty())
            {
                return;
            }

            constexpr Color bg_color = {255, 229, 236, 230};
            for (const auto& [negative_state_type, effects] : active_negative_states)
            {
                GetDrawHelper().DrawAttachedEffect(
                    entity,
                    "NegativeState",
                    Enum::EffectNegativeStateToString(negative_state_type),
                    effects,
                    bg_color);
            }
        });
}

void DrawConditionEffects::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            // Only for combat units
            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

            // Gets all the active negative states
            auto& active_conditions = attached_effects_component.GetActiveConditions();
            if (active_conditions.empty())
            {
                return;
            }

            constexpr Color bg_color = {215, 249, 206, 230};
            for (const auto& [condition_type, effects] : active_conditions)
            {
                GetDrawHelper().DrawAttachedEffect(
                    entity,
                    "Condition",
                    Enum::EffectConditionTypeToString(condition_type),
                    effects,
                    bg_color);
            }
        });
}

void DrawSynergy::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            // Only for combat units
            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            if (!entity.Has<CombatSynergyComponent>())
            {
                return;
            }

            const PositionComponent& position_component = entity.Get<PositionComponent>();
            Vector2f screen_position = GetDrawHelper().CovertHexToScreen(position_component.GetPosition());
            screen_position.x -= GetDrawHelper().ConvertEntityRadiusToScreen(position_component.GetRadius());
            screen_position.y -= GetDrawHelper().ConvertEntityRadiusToScreen(position_component.GetRadius());
            DrawSynergies(screen_position, entity);
        });
}

void DrawSynergy::DrawSynergies(const Vector2f& position, const simulation::Entity& entity)
{
    const SynergiesHelper& synergies_helper = GetWorld().GetSynergiesHelper();
    const CombatSynergyComponent& synergy_component = entity.Get<CombatSynergyComponent>();
    constexpr Color text_color = {36, 55, 99, 255};
    constexpr Color bg_color = {175, 229, 226, 210};
    constexpr Vector2f bg_size = {120, 33};

    std::string synergy_text;

    const EnumSet<CombatAffinity> entity_combat_affinities = synergies_helper.GetAllCombatAffinitiesOfEntity(entity);
    for (const CombatAffinity combat_affinity : entity_combat_affinities)
    {
        fmt::format_to(std::back_inserter(synergy_text), "Affinity: {}\n", combat_affinity);
    }

    if (!entity_combat_affinities.IsEmpty())
    {
        fmt::format_to(
            std::back_inserter(synergy_text),
            "Dominant affinity: {}\n",
            synergy_component.GetDominantCombatAffinity());
    }

    const EnumSet<CombatClass> entity_combat_classes = synergies_helper.GetAllCombatClassesOfEntity(entity);
    for (const CombatClass combat_class : entity_combat_classes)
    {
        fmt::format_to(std::back_inserter(synergy_text), "Class: {}\n", combat_class);
    }

    if (!entity_combat_classes.IsEmpty())
    {
        fmt::format_to(
            std::back_inserter(synergy_text),
            "Dominant class: {}\n",
            Enum::CombatClassToString(synergy_component.GetDominantCombatClass()));
    }

    GetDrawHelper().DrawBorderedText(synergy_text, position, bg_size, text_color, bg_color);
}

void DrawHyper::Draw()
{
    const Vector2f mouse_position = Input::GetMousePosition();
    const IVector2D world_position = GetDrawHelper().CovertFromScreen(mouse_position);
    const HexGridPosition hex_pos = GetWorld().FromWorldPosition(world_position);

    EntityID selected_unit_id = kInvalidEntityID;
    if (GetWorld().GetGridConfig().IsInMapRectangleLimits(hex_pos))
    {
        selected_unit_id = GetDrawHelper().GetSelectedUnit(hex_pos);
    }

    if (selected_unit_id == kInvalidEntityID)
    {
        DrawAll();
        return;
    }

    const Entity& selected_unit = GetWorld().GetByID(selected_unit_id);
    DrawSelectedState(selected_unit);
}

void DrawHyper::DrawSelectedState(const Entity& selected_unit)
{
    const auto& combat_synergy_component = selected_unit.Get<CombatSynergyComponent>();
    const CombatAffinity dominant_combat_affinity = combat_synergy_component.GetDominantCombatAffinity();
    const HyperConfig& hyper_config = GetWorld().GetHyperConfig();
    const HyperData& hyper_data = GetWorld().GetGameDataContainer().GetHyperData();

    int combat_affinity_effectiveness = 0;

    GetWorld().SafeWalkAllEnemiesOfEntity(
        selected_unit,
        [&](const Entity& opponent_entity)
        {
            // Check if in range for hyper
            if (!HyperHelper::AreInHyperRange(selected_unit, opponent_entity, hyper_config))
            {
                return;
            }

            // Get opponent dominant combat affinity
            const auto& opponent_combat_synergy_component = opponent_entity.Get<CombatSynergyComponent>();
            const CombatAffinity opponent_dominant_combat_affinity =
                opponent_combat_synergy_component.GetDominantCombatAffinity();

            const int opponent_effectiveness =
                hyper_data.GetCombatAffinityEffectiveness(dominant_combat_affinity, opponent_dominant_combat_affinity);
            combat_affinity_effectiveness += opponent_effectiveness;

            const auto& opponent_position_component = opponent_entity.Get<PositionComponent>();

            const Color bg_color = GetColorForOpponentEffectiveness(opponent_effectiveness);
            GetDrawHelper().DrawHexGroup(
                opponent_position_component.GetPosition(),
                opponent_position_component.GetRadius(),
                bg_color);
            DrawOpponentEffectiveness(opponent_position_component, opponent_effectiveness);
        });

    DrawUnitHyperZone(selected_unit, constants::color::Yellow);
    DrawUnitEffectiveness(selected_unit, combat_affinity_effectiveness);
}

void DrawHyper::DrawAll()
{
    for (const auto& current_entity : GetWorld().GetAll())
    {
        // Don't add to hyper if it's fainted
        if (!current_entity->IsActive())
        {
            continue;
        }

        // Check if combat unit
        if (!EntityHelper::IsACombatUnit(*current_entity))
        {
            continue;
        }

        const int combat_affinity_effectiveness = HyperHelper::CalculateHyperGrowthEffectiveness(*current_entity);

        DrawUnitEffectiveness(*current_entity, combat_affinity_effectiveness);
    }
}

void DrawHyper::DrawOpponentEffectiveness(const PositionComponent& position_component, const int effectiveness)
{
    constexpr Color text_color = {36, 55, 99, 255};
    constexpr Color bg_color = {245, 139, 136, 255};
    const float screen_radius = GetDrawHelper().ConvertEntityRadiusToScreen(position_component.GetRadius());
    const Vector2f bg_size = {screen_radius, 30.0f};

    Vector2f screen_position = GetDrawHelper().CovertHexToScreen(position_component.GetPosition());
    screen_position.x -= bg_size.x / 2.0f;
    screen_position.y -= bg_size.y / 2.0f;

    GetDrawHelper()
        .DrawBorderedText(std::to_string(effectiveness), screen_position, bg_size, text_color, bg_color, 4.0f, 20.0f);
}

void DrawHyper::DrawUnitEffectiveness(const Entity& entity, const int affinity_effectiveness)
{
    FixedPoint effectiveness = FixedPoint::FromInt(affinity_effectiveness);
    effectiveness *= FixedPoint::FromInt(kPrecisionFactor);

    const auto& stats_component = entity.Get<StatsComponent>();
    const auto& position_component = entity.Get<PositionComponent>();

    const HyperConfig& hyper_config = GetWorld().GetHyperConfig();

    const FixedPoint position_effectiveness = hyper_config.sub_hyper_scale_percentage.AsPercentageOf(effectiveness);
    const FixedPoint current_sub_hyper = stats_component.GetCurrentSubHyper();

    constexpr Color text_color = {36, 55, 99, 255};
    Color bg_color = {175, 229, 226, 255};
    if (affinity_effectiveness > 0)
    {
        bg_color = {75, 229, 26, 255};
    }
    else if (affinity_effectiveness < 0)
    {
        bg_color = {225, 49, 46, 255};
    }

    const float screen_radius = GetDrawHelper().ConvertEntityRadiusToScreen(position_component.GetRadius());
    const Vector2f bg_size = {screen_radius * 2.0f, 60.0f};

    Vector2f screen_position = GetDrawHelper().CovertHexToScreen(position_component.GetPosition());
    screen_position.x -= screen_radius;
    screen_position.y -= screen_radius;

    const std::string text = fmt::format(
        "Effectiveness {}\nGain {}\nCurrent {}\nMax {}",
        affinity_effectiveness,
        position_effectiveness.AsInt(),
        current_sub_hyper.AsInt(),
        kMaxSubHyper);
    GetDrawHelper().DrawBorderedText(text, screen_position, bg_size, text_color, bg_color);
}

void DrawHyper::DrawUnitHyperZone(const Entity& entity, const Color& bg_color)
{
    const auto& position_component = entity.Get<PositionComponent>();
    const HyperConfig& hyper_config = GetWorld().GetHyperConfig();

    // NOTE: incrementing visualization by 1 to show overlap
    const int radius = position_component.GetRadius() + hyper_config.enemies_range_units + 1;

    GetDrawHelper().DrawBorderHexes(position_component.GetPosition(), radius, bg_color);
}

Color DrawHyper::GetColorForOpponentEffectiveness(const int effectiveness)
{
    switch (effectiveness)
    {
    case 2:
        return Color{0, 191, 233, 255};
    case 1:
    case 0:
        return Color{250, 255, 0, 255};
    case -1:
        return Color{250, 0, 0, 255};
    case -2:
        return Color{250, 0, 255, 255};

    default:
        assert(false);
        break;
    }

    return constants::color::White;
}

void DrawProjectiles::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            // Only for projectiles
            if (!EntityHelper::IsAProjectile(entity))
            {
                return;
            }

            const auto& projectile_component = entity.Get<ProjectileComponent>();
            const EntityID receiver_id = projectile_component.GetReceiverID();
            if (!GetWorld().HasEntity(receiver_id))
            {
                return;
            }

            const auto& position_component = entity.Get<PositionComponent>();

            const auto color = GetRandomColor(entity.GetID());

            const HexGridPosition current_position = position_component.GetPosition();
            const HexGridPosition target_position =
                GetWorld().GetByID(receiver_id).Get<PositionComponent>().GetPosition();

            GetDrawHelper().DrawHexGroup(current_position, position_component.GetRadius(), color);
            GetDrawHelper().DrawArrow(current_position, target_position, 1.0, color);
        });
}

void DrawBeams::Draw()
{
    GetWorld().SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            // Only for projectiles
            if (!EntityHelper::IsABeam(entity))
            {
                return;
            }

            const auto& beam_component = entity.Get<BeamComponent>();
            const EntityID receiver_id = beam_component.GetReceiverID();
            if (!GetWorld().HasEntity(receiver_id))
            {
                return;
            }

            const auto color = GetRandomColor(entity.GetID());

            const HexGridPosition beam_position = entity.Get<PositionComponent>().GetPosition();
            const HexGridPosition target_position =
                GetWorld().GetByID(receiver_id).Get<PositionComponent>().GetPosition();

            const int width_units = simulation::Math::SubUnitsToUnits(beam_component.GetWidthSubUnits());
            GetDrawHelper().DrawLine(beam_position, target_position, width_units, color);
        });
}

void DrawCenterLine::Draw()
{
    const auto& grid_config = GetWorld().GetGridConfig();
    const int middle_line_width = GetWorld().GetMiddleLineWidth();

    auto line_color = constants::color::Red;
    line_color.a = 130;

    const int width_extent = grid_config.GetRectangleWidthExtent();
    const auto draw_r_line = [&](const int r)
    {
        for (int q = -width_extent; q <= width_extent; ++q)
        {
            GetDrawHelper().DrawFillHex({q, r}, line_color);
        }
    };

    draw_r_line(0);
    for (int r = 1; r <= middle_line_width; ++r)
    {
        draw_r_line(r);
        draw_r_line(-r);
    }
}

void DrawChains::Initialize()
{
    auto& world = GetWorld();
    event_handlers_ids_.push_back(world.SubscribeMethodToEvent<EventType::kChainCreated>(this, &Self::OnNewChain));
    event_handlers_ids_.push_back(world.SubscribeMethodToEvent<EventType::kChainDestroyed>(this, &Self::OnChainRemove));
}

DrawChains::~DrawChains()
{
    auto& world = GetWorld();
    for (const EventHandleID& id : event_handlers_ids_)
    {
        world.UnsubscribeFromEvent(id);
    }
}

void DrawChains::OnNewChain(const simulation::event_data::ChainCreated& chain_created_event)
{
    auto& world = GetWorld();
    auto get_position = [&](const EntityID& entity_id)
    {
        const auto entity_ptr = world.GetByIDPtr(entity_id);
        if (!(entity_ptr && entity_ptr->Has<PositionComponent>()))
        {
            return kInvalidHexHexGridPosition;
        }

        return entity_ptr->Get<PositionComponent>().GetPosition();
    };

    const auto entity_id = chain_created_event.entity_id;
    assert(id_to_origin_.count(entity_id) == 0);
    if (EntityHelper::IsAChain(world, chain_created_event.sender_id))
    {
        assert(id_to_origin_.count(chain_created_event.sender_id) != 0);
        id_to_origin_[entity_id] = id_to_origin_[chain_created_event.sender_id];
    }
    else
    {
        assert(id_to_origin_.count(entity_id) == 0);
        id_to_origin_[entity_id] = entity_id;

        auto color = GetRandomColor(entity_id);
        color.a = 255;
        origin_to_color_[entity_id] = color;
    }

    assert(id_to_segment_.count(entity_id) == 0);
    auto& segment = id_to_segment_[entity_id];
    segment.source_position = get_position(chain_created_event.sender_id);
    segment.destination_position = get_position(chain_created_event.receiver_id);
}

void DrawChains::OnChainRemove(const simulation::event_data::ChainDestroyed& chain_destroyed_event)
{
    assert(id_to_segment_.count(chain_destroyed_event.entity_id) != 0);
    auto& segment = id_to_segment_[chain_destroyed_event.entity_id];
    segment.destroyed_on_time_step = GetWorld().GetTimeStepCount();
}

void DrawChains::Draw()
{
    const auto& world = GetWorld();
    const int time_step = world.GetTimeStepCount();

    const auto forget_origin_if_unused = [&](const EntityID origin_id)
    {
        for (auto& [key, value] : id_to_origin_)
        {
            if (value == origin_id)
            {
                return;
            }
        }

        origin_to_color_.erase(origin_id);
    };

    // Draw chains and delete inactive segments
    for (auto it = id_to_segment_.begin(); it != id_to_segment_.end();)
    {
        const auto& [entity_id, segment] = *it;
        const EntityID origin_entity_id = id_to_origin_[entity_id];
        assert(origin_to_color_.count(origin_entity_id));
        Color chain_color = origin_to_color_[origin_entity_id];

        // Draw active chain segment
        const int destroy_time_step = segment.destroyed_on_time_step;
        const auto delta_time_step = destroy_time_step < 0 ? 0 : time_step - destroy_time_step;
        if (delta_time_step < kChainSegmentLifetimeAfterDestroy)
        {
            chain_color.a = 255 - static_cast<uint8_t>(
                                      255.f * static_cast<float>(delta_time_step) / kChainSegmentLifetimeAfterDestroy);
            GetDrawHelper()
                .DrawLine(segment.source_position, segment.destination_position, kChainLineThickness, chain_color);
            ++it;
        }
        else
        {
            // Or delete it and renew iterator
            id_to_origin_.erase(entity_id);
            it = id_to_segment_.erase(it);
            forget_origin_if_unused(origin_entity_id);
        }
    }
}

void DrawAuras::Draw()
{
    for (const std::shared_ptr<Entity>& entity_ptr : GetWorld().GetAll())
    {
        if (!entity_ptr || !entity_ptr->IsActive() || !EntityHelper::IsAnAura(*entity_ptr))
        {
            continue;
        }

        const Entity& aura = *entity_ptr;
        const auto& aura_component = aura.Get<AuraComponent>();
        const AuraData& aura_data = aura_component.GetComponentData();

        if (!GetWorld().HasEntity(aura_data.receiver_id))
        {
            continue;
        }

        const Entity& receiver = GetWorld().GetByID(aura_data.receiver_id);
        const HexGridPosition aura_position = receiver.Get<PositionComponent>().GetPosition();

        GetDrawHelper().DrawBorderHexes(aura_position, aura_data.effect.radius_units, GetRandomColor(aura.GetID()));
    }
}

void DrawAuras::TooltipForSelectedEntity(const EntityID entity_id, std::string* text)
{
    size_t num_auras = 0;

    for (const std::shared_ptr<Entity>& entity_ptr : GetWorld().GetAll())
    {
        if (!entity_ptr || !entity_ptr->IsActive() || !EntityHelper::IsAnAura(*entity_ptr))
        {
            continue;
        }

        const Entity& aura = *entity_ptr;
        const auto& aura_component = aura.Get<AuraComponent>();
        const AuraData& aura_data = aura_component.GetComponentData();

        if (aura_data.receiver_id == entity_id)
        {
            if (num_auras++ == 0)
            {
                String::AppendLineAndFormat(*text, "Attached auras: {}", aura.GetID());
            }
            else
            {
                String::FormatTo(*text, ", {}", aura.GetID());
            }
        }

        // GetDrawHelper().DrawFillHexes(position_component.GetPosition(), zone_component.GetRadiusUnits(), color);
    }
}

}  // namespace simulation::tool
