#include "systems/zone_system.h"

#include <cstdint>

#include "components/filtering_component.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "components/zone_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"
#include "utility/intersection_helper.h"
#include "utility/math.h"
#include "utility/time.h"

namespace simulation
{

void ZoneSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kZoneActivated>(this, &Self::OnZoneActivated);
}

void ZoneSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Get the entity ID
    const EntityID entity_id = entity.GetID();

    // We are only interested in zones
    if (!EntityHelper::IsAZone(*world_, entity_id))
    {
        return;
    }
    auto& zone_component = entity.Get<ZoneComponent>();

    // Get and increment the time step count
    const int current_time_step_count = zone_component.GetAndIncrementTimeStepCount();

    // Check frequency and activate skill
    const int time_step_frequency = Time::MsToTimeSteps(zone_component.GetFrequencyMs());
    if (time_step_frequency == 0)
    {
        LogWarn(entity_id, "ZoneSystem::TimeStep - frequency shouldn't be 0!");
    }

    if (time_step_frequency && current_time_step_count % time_step_frequency == 0)
    {
        const int activation_index = current_time_step_count / time_step_frequency;
        if (!zone_component.ShouldSkipActivation(activation_index))
        {
            LogDebug(entity_id, "ZoneSystem::TimeStep - Time for the zone to activate");
            world_->BuildAndEmitEvent<EventType::kZoneActivated>(
                entity_id,
                zone_component.GetSenderID(),
                entity.Get<PositionComponent>().GetPosition());
        }
    }

    // Grow the zone
    const int old_zone_radius = zone_component.GetRadiusSubUnits();
    zone_component.IncreaseRadiusSubUnits(zone_component.GetGrowthRateSubUnitsPerStep());

    // Emit zone radius changed event only if values changes
    if (const int new_zone_radius = zone_component.GetRadiusSubUnits(); new_zone_radius != old_zone_radius)
    {
        event_data::ZoneRadiusChanged event{};
        event.entity_id = entity_id;
        event.old_radius_sub_units = old_zone_radius;
        event.new_radius_sub_units = new_zone_radius;
        world_->EmitEvent<EventType::kZoneRadiusChanged>(event);
    }

    if (ShouldDestroyZone(entity))
    {
        LogDebug(entity_id, "ZoneSystem::TimeStep send MarkZoneAsDestroyed event.");
        world_->BuildAndEmitEvent<EventType::kMarkZoneAsDestroyed>(entity_id);
    }
}

void ZoneSystem::PostTimeStep(const Entity& entity)
{
    System::PostTimeStep(entity);

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Get the entity ID
    const EntityID entity_id = entity.GetID();

    // We are only interested in zones
    if (!EntityHelper::IsAZone(*world_, entity_id))
    {
        return;
    }
    const auto& zone_component = entity.Get<ZoneComponent>();
    if (zone_component.IsApplyOnce() || !zone_component.IsGlobalOverlap())
    {
        return;
    }

    // Clear global collisions for this global collision id if setup for non apply once zones
    world_->ResetGlobalCollision(entity.GetTeam(), zone_component.GetGlobalCollisionID());
}

bool ZoneSystem::ShouldDestroyZone(const Entity& zone_entity) const
{
    ILLUVIUM_PROFILE_FUNCTION();

    const auto& zone_component = zone_entity.Get<ZoneComponent>();
    const EntityID entity_id = zone_entity.GetID();
    const EntityID combat_unit_id = zone_component.GetOriginalSenderID();

    if (zone_component.IsChanneled())
    {
        const auto& combat_unit = world_->GetByID(combat_unit_id);
        const auto& abilities_component = combat_unit.Get<AbilitiesComponent>();

        if (const auto current_ability = abilities_component.GetActiveAbility())
        {
            if (current_ability->IsFinished())
            {
                LogDebug(entity_id, "ZoneSystem::TimeStep - Ability is finished, destroy channeled zone");
                return true;
            }

            const SkillStateType current_ability_skill_state = current_ability->GetCurrentSkillState().state;
            if (current_ability_skill_state != SkillStateType::kChanneling)
            {
                LogDebug(entity_id, "ZoneSystem::TimeStep - Skill is not channeling, destroy channeled zone");
                return true;
            }
        }
        else
        {
            LogDebug(entity_id, "ZoneSystem::TimeStep - Not active ability, destroy channeled zone");
            return true;
        }
    }

    // Check whether zone should fade away
    const auto& position_component = zone_entity.Get<PositionComponent>();
    if (!world_->GetGridConfig().IsInMapRectangleLimitsWithMargin(
            position_component.GetPosition(),
            position_component.GetRadius()))
    {
        LogDebug(entity_id, "ZoneSystem::TimeStep - Zone has left the map");
        return true;
    }

    if (zone_component.CanDestroyWithSender() && !world_->IsCombatUnitAlive(combat_unit_id))
    {
        return true;
    }

    return false;
}

void ZoneSystem::SummarizeZoneEffectTypes(
    const Entity& zone,
    const AbilityType ability_type,
    bool* out_beneficial,
    bool* out_detrimental) const
{
    ILLUVIUM_PROFILE_FUNCTION();

    const AbilitiesComponent* abilities_component = zone.GetPtr<AbilitiesComponent>();
    if (!abilities_component)
    {
        return;
    }

    const auto& abilities = abilities_component->GetAbilities(ability_type);
    for (const std::shared_ptr<const AbilityData>& ability_data : abilities.data.abilities)
    {
        for (const std::shared_ptr<SkillData>& skill_data : ability_data->skills)
        {
            for (const EffectData& effect_data : skill_data->effect_package.effects)
            {
                if (effect_data.IsTypeDetrimental())
                {
                    *out_detrimental = true;
                }

                if (effect_data.IsTypeBeneficial())
                {
                    *out_beneficial = true;
                }

                if (*out_detrimental && *out_beneficial)
                {
                    return;
                }
            }
        }
    }
}

void ZoneSystem::OnZoneActivated(const event_data::ZoneActivated& data)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // We are only interested in zones
    if (!EntityHelper::IsAZone(*world_, data.entity_id))
    {
        return;
    }

    const EntityID zone_id = data.entity_id;
    const auto& zone = world_->GetByID(zone_id);
    auto& zone_component = zone.Get<ZoneComponent>();
    auto& filtering_component = zone.Get<FilteringComponent>();

    //
    // Check if we have collision with anything else
    //
    const auto& position_component = zone.Get<PositionComponent>();

    const ZoneEffectShape shape = zone_component.GetShape();

    // Save some resources by only doing this once for all entities
    TriangleZoneIntersectionCache triangle_intersection_cache;
    if (shape == ZoneEffectShape::kTriangle)
    {
        // Angle the zone is facing relative to its position
        const int direction_degrees =
            Math::AngleLimitTo360(zone_component.GetSpawnDirectionDegrees() + zone_component.GetDirectionDegrees());

        triangle_intersection_cache = IntersectionHelper::MakeTriangleIntersectionCache(
            *world_,
            position_component.GetPosition(),
            direction_degrees,
            zone_component.GetRadiusUnits());
    }

    bool with_beneficial_effects = false;
    bool with_detrimental_effects = false;
    SummarizeZoneEffectTypes(zone, AbilityType::kAttack, &with_beneficial_effects, &with_detrimental_effects);

    std::vector<EntityID> collisions;
    for (const auto& other : world_->GetAll())
    {
        // Ignore collision with:
        // - self
        const EntityID other_id = other->GetID();
        if (zone_id == other_id)
        {
            continue;
        }

        // Avoid activating abilities on irrelevant targets
        const bool is_relevant_target = zone.IsAlliedWith(*other) ? with_beneficial_effects : with_detrimental_effects;
        if (!is_relevant_target)
        {
            continue;
        }

        // Ignore entities that are not taking space
        if (!EntityHelper::IsCollidable(*other))
        {
            continue;
        }

        // Already collided with this entity, ignore if apply once set
        if (zone_component.IsApplyOnce())
        {
            if (HasZoneCollidedWith(zone_component, filtering_component, other_id))
            {
                continue;
            }
        }

        // Filter component should filter out targets that should be ignored
        if (filtering_component.GetIgnoredReceiverIDs().count(other_id) > 0)
        {
            continue;
        }

        // Found another entity of interest
        // Get position component
        const auto& other_position_component = other->Get<PositionComponent>();

        // Found another entity of interest
        switch (shape)
        {
        case ZoneEffectShape::kHexagon:
        {
            if (IntersectionHelper::DoesHexZoneIntersectEntity(
                    zone_component.GetRadiusUnits(),
                    position_component.GetPosition(),
                    other_position_component.GetPosition()))
            {
                collisions.emplace_back(other_id);
            }
            break;
        }
        case ZoneEffectShape::kRectangle:
        {
            if (IntersectionHelper::DoesRectangleZoneIntersectEntity(
                    position_component.GetPosition().ToSubUnits(),
                    other_position_component.GetPosition().ToSubUnits(),
                    zone_component.GetWidthSubUnits(),
                    zone_component.GetHeightSubUnits()))
            {
                collisions.emplace_back(other_id);
            }
            break;
        }
        case ZoneEffectShape::kTriangle:
        {
            // Check if sum of other triangles add up to triangle ABC
            if (IntersectionHelper::DoesTriangleZoneIntersectEntity(
                    triangle_intersection_cache,
                    *world_,
                    other_position_component.GetPosition()))
            {
                collisions.emplace_back(other_id);
            }
            break;
        }
        default:
            break;
        }
    }

    for (auto& collided_with : collisions)
    {
        // Found a collision
        LogDebug(zone_id, "ZoneSystem::OnZoneActivated Found a collision with entity = {}", collided_with);

        if (zone_component.IsGlobalOverlap() && HasZoneCollidedWith(zone_component, filtering_component, collided_with))
        {
            continue;
        }

        // Mark as collided
        AddZoneCollidedWith(zone_component, filtering_component, collided_with);

        auto& focus_component = zone.Get<FocusComponent>();

        // Set the new focus with our collided with
        focus_component.SetFocus(world_->GetByIDPtr(collided_with));

        // Choose any ability and activate it in the same time step
        constexpr bool can_do_attack = true;
        constexpr bool can_do_omega = true;
        world_->BuildAndEmitEvent<EventType::kActivateAnyAbility>(zone_id, can_do_attack, can_do_omega);

        // Reset the focus
        focus_component.ResetFocus();
    }
}

bool ZoneSystem::HasZoneCollidedWith(
    const ZoneComponent& zone_component,
    const FilteringComponent& filtering_component,
    const EntityID entity_id) const
{
    ILLUVIUM_PROFILE_FUNCTION();
    if (zone_component.IsGlobalOverlap())
    {
        const Team team = zone_component.GetOwnerEntity()->GetTeam();
        return world_->HasGlobalCollision(team, zone_component.GetGlobalCollisionID(), entity_id);
    }

    return filtering_component.HasCollidedWith(entity_id);
}

void ZoneSystem::AddZoneCollidedWith(
    const ZoneComponent& zone_component,
    FilteringComponent& filtering_component,
    const EntityID entity_id) const
{
    ILLUVIUM_PROFILE_FUNCTION();
    if (zone_component.IsGlobalOverlap())
    {
        const Team team = zone_component.GetOwnerEntity()->GetTeam();
        world_->AddGlobalCollision(team, zone_component.GetGlobalCollisionID(), entity_id);
    }
    else
    {
        filtering_component.AddCollidedWith(entity_id);
    }
}

}  // namespace simulation
