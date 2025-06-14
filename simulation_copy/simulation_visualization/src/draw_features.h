#pragma once

#include <memory>
#include <unordered_map>

#include "color.h"
#include "data/constants.h"
#include "data/effect_enums.h"
#include "ecs/event_types_data.h"
#include "rectangle.h"
#include "utility/hex_grid_position.h"
#include "vector2f.h"

namespace simulation
{
class CombatSynergyComponent;
class ZoneComponent;
class Entity;
class World;
}  // namespace simulation

namespace simulation::tool
{
class DrawHelper;
class BattleVisualization;

/* -------------------------------------------------------------------------------------------------------
 * BaseDrawFeature
 *
 * This class is base for all visualization drawing features.
 * It contains enabling/disabling functionality using checkbox.
 * --------------------------------------------------------------------------------------------------------
 */
class BaseDrawFeature
{
public:
    friend BattleVisualization;

    BaseDrawFeature() = default;
    virtual ~BaseDrawFeature() = default;

    virtual void Initialize() {}
    virtual void Draw() {}
    virtual void DrawUI() {}
    virtual void TooltipForSelectedEntity(const EntityID, std::string*) {}

    simulation::World& GetWorld() const
    {
        return *world_;
    }
    const DrawHelper& GetDrawHelper() const
    {
        return *draw_helper_;
    }

    DrawHelper& GetDrawHelper()
    {
        return *draw_helper_;
    }

    void SetEnabled(bool is_enabled)
    {
        is_enabled_ = is_enabled;
    }

    bool IsEnabled() const
    {
        return is_enabled_;
    }

    const std::string& GetName() const
    {
        return feature_name_;
    }

    void DrawCheckBox(const Rectangle& rect);

protected:
    const Color& GetRandomColor(simulation::EntityID entity_id) const;
    size_t GetRandomColorIndex(simulation::EntityID entity_id) const;

private:
    std::shared_ptr<simulation::World> world_;
    DrawHelper* draw_helper_ = nullptr;
    bool is_enabled_ = true;
    std::string feature_name_;
    mutable std::unordered_map<simulation::EntityID, Color> assigned_colors_;
};

class DrawCombatUnits : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DebugDisplacement : public BaseDrawFeature
{
public:
    using Super = BaseDrawFeature;
    using Self = DebugDisplacement;

    void Draw() override;
    void Initialize() override;

private:
    struct DisplacementDebugData
    {
        simulation::EntityID sender = simulation::kInvalidEntityID;
        simulation::EffectDisplacementType type = simulation::EffectDisplacementType::kNone;
        simulation::HexGridPosition started_pos;
        simulation::HexGridPosition target_pos;
    };

    void OnDisplacementStarted(const simulation::event_data::Displacement& displacement_data);
    void OnDisplacementStopped(const simulation::event_data::Displacement& displacement_data);

    void DrawDisplacement(simulation::EntityID receiver, const DisplacementDebugData& data) const;

    std::unordered_map<simulation::EntityID, DisplacementDebugData> active_displacements_;
};

class DebugFocus : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DebugAbility : public BaseDrawFeature
{
public:
    void Draw() override;

private:
    void DrawAbilitiesProgress(const simulation::Entity& entity) const;
};

class DrawZone : public BaseDrawFeature
{
public:
    void Draw() override;

private:
    void Draw(const simulation::Entity& entity) const;

    void DrawHexagonZone(
        const simulation::Entity& entity,
        const simulation::ZoneComponent& zone_component,
        const Color& color) const;
    void DrawTriangleZone(
        const simulation::Entity& entity,
        const simulation::ZoneComponent& zone_component,
        const Color& color) const;
    void DrawRectangleZone(
        const simulation::Entity& entity,
        const simulation::ZoneComponent& zone_component,
        const Color& color) const;
};

class DebugNavigation : public BaseDrawFeature
{
public:
    void Draw() override;

public:
    void DrawNavigationInfo(const simulation::Entity& entity) const;
    void DrawReservedPosition(const simulation::Entity& entity) const;
};

class DebugPathfinding : public BaseDrawFeature
{
public:
    using Super = BaseDrawFeature;
    using Self = DebugPathfinding;

    void Initialize() override;
    void Draw() override;
    void DrawUI() override;

private:
    void OnDebugPathfinding(const simulation::event_data::PathfindingDebugData& data);

    void DrawEventSelection();
    void DrawSelectedEvent() const;

    void DrawObstacles(int source_radius, const simulation::ObstaclesMapType& obstacles) const;
    void DrawNodeParentLine(size_t node, size_t parent, const Color& color, float thick) const;

    int events_time_step_ = 0;
    std::vector<simulation::event_data::PathfindingDebugData> events_;
    std::vector<std::string> event_texts_;
    size_t selected_event_ = 0;
};

class DrawEventsLog : public BaseDrawFeature
{
public:
    static constexpr float kFontSize = 20;

    using Super = BaseDrawFeature;
    using Self = DrawEventsLog;

    struct DamageDebugData
    {
        // original event data
        event_data::AppliedDamage event;
        std::string sender_name;
        std::string receiver_name;
        int time_step = -1;
    };

    struct HealDebugData
    {
        // original event data
        event_data::HealthGain event;
        std::string sender_name;
        std::string receiver_name;
        int time_step = -1;
    };

    void Initialize() override;
    void Draw() override;

private:
    size_t GetTotalEventsCount() const
    {
        return damage_events_.size() + heal_events_.size();
    }

    void PopOverflowingEvents();
    void OnDamageEvent(const event_data::AppliedDamage& event_data);
    void OnHealthGainEvent(const event_data::HealthGain& event_data);

    DamageDebugData FillEventDebugData(const event_data::AppliedDamage& event_data) const;
    HealDebugData FillEventDebugData(const event_data::HealthGain& event_data) const;

    void DrawEventDebugData(const Vector2f& position, const DamageDebugData& data) const;
    void DrawEventDebugData(const Vector2f& position, const HealDebugData& data) const;

    static constexpr size_t kMaxQueueLength = 30;
    bool is_expanded_ = false;

    static constexpr size_t kInvalidEventId = std::numeric_limits<size_t>::max();
    size_t next_event_id_ = 0;

    // contains ids of events. The can be later be found in tables below
    std::deque<size_t> events_queue_;

    // Key: id of event
    // Value: event data
    std::unordered_map<size_t, DamageDebugData> damage_events_;
    std::unordered_map<size_t, HealDebugData> heal_events_;

    mutable std::string buffer_;
};

class DrawLiveStats : public BaseDrawFeature
{
public:
    void TooltipForSelectedEntity(const EntityID entity_id, std::string* out_text) override;
};

class DrawPlaneChange : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DrawPositiveStates : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DrawNegativeStates : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DrawConditionEffects : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DrawSynergy : public BaseDrawFeature
{
public:
    void Draw() override;

private:
    void DrawSynergies(const Vector2f& position, const simulation::Entity& entity);
};

class DrawHyper : public BaseDrawFeature
{
public:
    void Draw() override;

private:
    static Color GetColorForOpponentEffectiveness(const int effectiveness);

    void DrawSelectedState(const simulation::Entity& selected_unit);
    void DrawAll();
    void DrawOpponentEffectiveness(const simulation::PositionComponent& position_component, const int effectiveness);
    void DrawUnitEffectiveness(const simulation::Entity& entity, const int effectiveness);

    void DrawUnitHyperZone(const simulation::Entity& entity, const Color& bg_color);
};

class DrawProjectiles : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DrawBeams : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DrawCenterLine : public BaseDrawFeature
{
public:
    void Draw() override;
};

class DrawChains : public BaseDrawFeature
{
public:
    using Super = BaseDrawFeature;
    using Self = DrawChains;

    ~DrawChains() override;

    struct ChainSegment
    {
        simulation::HexGridPosition source_position = simulation::kInvalidHexHexGridPosition;
        simulation::HexGridPosition destination_position = simulation::kInvalidHexHexGridPosition;
        int destroyed_on_time_step = -1;
    };

    void Initialize() override;
    void Draw() override;
    void OnNewChain(const simulation::event_data::ChainCreated& chain_created_event);
    void OnChainRemove(const simulation::event_data::ChainDestroyed& chain_destroyed_event);

protected:
    static constexpr int kChainSegmentLifetimeAfterDestroy = 10;
    static constexpr float kChainLineThickness = 10.f;

    // Save information about chain segments.
    // Key: chain entity id
    std::unordered_map<simulation::EntityID, ChainSegment> id_to_segment_;

    // Key: chain entity id
    // Value: entity id of the first entity in the chain (chain origin)
    std::unordered_map<simulation::EntityID, simulation::EntityID> id_to_origin_;

    // Saved color for the whole chain
    std::unordered_map<simulation::EntityID, Color> origin_to_color_;

    std::vector<simulation::EventHandleID> event_handlers_ids_;
};

class DrawAuras : public BaseDrawFeature
{
public:
    void Draw() override;
    void TooltipForSelectedEntity(const EntityID entity_id, std::string* text) override;
};

}  // namespace simulation::tool
