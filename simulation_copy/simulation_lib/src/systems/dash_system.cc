#include "systems/dash_system.h"

#include "components/dash_component.h"
#include "components/filtering_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"

namespace simulation
{

void DashSystem::Init(World* world)
{
    Super::Init(world);
    world->SubscribeMethodToEvent<EventType::kAbilityInterrupted>(this, &Self::OnInterrupt);
}

void DashSystem::OnInterrupt(const event_data::AbilityInterrupted& event)
{
    if (!world_->HasEntity(event.sender_id))
    {
        return;
    }

    const auto sender_ptr = world_->GetByIDPtr(event.sender_id);
    if (!sender_ptr || !EntityHelper::IsACombatUnit(*sender_ptr))
    {
        return;
    }

    constexpr auto has_dash_skill = [](const AbilityData& ability)
    {
        for (const auto& skill : ability.skills)
        {
            if (skill->deployment.type == SkillDeploymentType::kDash)
            {
                return true;
            }
        }

        return false;
    };

    const auto& sender_abilities = sender_ptr->Get<AbilitiesComponent>();
    const auto& abilities = sender_abilities.GetAbilities(event.ability_type).data;
    const auto& ability = abilities.abilities[event.ability_index];

    if (!has_dash_skill(*ability))
    {
        return;
    }

    for (const auto& dash_entity : world_->GetAll())
    {
        if (!dash_entity || !dash_entity->IsActive() || !EntityHelper::IsADash(*dash_entity))
        {
            continue;
        }

        const auto& dash = dash_entity->Get<DashComponent>();
        if (dash.GetSenderID() == event.sender_id)
        {
            interrupted_dashes_.emplace(dash_entity->GetID());
        }
    }
}

void DashSystem::TimeStep(const Entity& entity)
{
    const EntityID dash_id = entity.GetID();

    // We are only interested in dashes
    if (!EntityHelper::IsADash(*world_, dash_id))
    {
        return;
    }

    const auto& dash_entity = world_->GetByID(dash_id);

    if (!dash_entity.IsActive() || TryToDestroyIfInterrupted(dash_entity))
    {
        // Ignore not active
        return;
    }

    ProcessDashMove(dash_entity);
}

bool DashSystem::TryToDestroyIfInterrupted(const Entity& dash_entity)
{
    static constexpr std::string_view method_name = "DashSystem::TryToDestroyIfInterrupted";

    if (!interrupted_dashes_.count(dash_entity.GetID()))
    {
        return false;
    }

    auto& dash_component = dash_entity.Get<DashComponent>();
    auto sender_entity_ptr = world_->GetByIDPtr(dash_component.GetSenderID());
    if (sender_entity_ptr)
    {
        // Do not stop dash until free position is found
        auto& sender_position = sender_entity_ptr->Get<PositionComponent>();
        if (world_->GetGridHelper().IsHexagonPositionTaken(
                sender_position.GetPosition(),
                sender_position.GetRadius(),
                {dash_component.GetSenderID()}))
        {
            return false;
        }
    }

    LogDebug(dash_entity.GetID(), "{} - destroying dash as ability was interrupted", method_name);
    world_->BuildAndEmitEvent<EventType::kMarkDashAsDestroyed>(dash_entity.GetID());
    interrupted_dashes_.erase(dash_entity.GetID());

    return true;
}

void DashSystem::ProcessDashMove(const Entity& dash_entity)
{
    assert(EntityHelper::IsADash(dash_entity));

    auto& dash_component = dash_entity.Get<DashComponent>();
    auto& filtering_component = dash_entity.Get<FilteringComponent>();

    // Find new collisions
    std::vector<EntityID> new_collision;
    if (FindNewCollisions(dash_entity, new_collision))
    {
        for (const EntityID entity_id : new_collision)
        {
            ActivateDashAbilityOnEntity(dash_entity, entity_id);
        }
    }

    const EntityID sender_id = dash_component.GetSenderID();
    const auto& sender_entity = world_->GetByID(sender_id);
    const auto& sender_position_component = sender_entity.Get<PositionComponent>();
    const HexGridPosition& new_position = sender_position_component.GetPosition();

    // Emit move event for the sender
    world_->BuildAndEmitEvent<EventType::kMoved>(sender_id, new_position, std::list<HexGridPosition>{new_position});

    dash_component.SetLastPosition(new_position);

    // Target reach check
    const auto& movement_component = dash_entity.Get<MovementComponent>();
    if (sender_position_component.GetPosition() == movement_component.GetMovementTargetPosition())
    {
        // Apply effect to target entity in end position for not land behind dash, it can't collide
        const EntityID target_id = dash_component.GetReceiverID();

        if (!filtering_component.HasCollidedWith(target_id) && !dash_component.IsLandBehind())
        {
            filtering_component.AddCollidedWith(target_id);

            ActivateDashAbilityOnEntity(dash_entity, target_id);
        }

        LogDebug(dash_entity.GetID(), "DashSystem::TimeStep - destroying dash as it did reach the target");
        world_->BuildAndEmitEvent<EventType::kMarkDashAsDestroyed>(dash_entity.GetID());
    }
}

void DashSystem::ActivateDashAbilityOnEntity(const Entity& sender, EntityID receiver_id)
{
    auto& focus_component = sender.Get<FocusComponent>();
    focus_component.SetFocus(world_->GetByIDPtr(receiver_id));

    // Dash have only attack ability, so activate it
    constexpr bool can_do_attack = true;
    constexpr bool can_do_omega = false;
    world_->BuildAndEmitEvent<EventType::kActivateAnyAbility>(sender.GetID(), can_do_attack, can_do_omega);

    focus_component.ResetFocus();
}

bool DashSystem::FindNewCollisions(const Entity& dash_entity, std::vector<EntityID>& out_new_collision)
{
    out_new_collision.clear();

    auto& dash_component = dash_entity.Get<DashComponent>();
    auto& filtering_component = dash_entity.Get<FilteringComponent>();

    const EntityID sender_id = dash_component.GetSenderID();
    const auto& sender_entity = world_->GetByID(sender_id);
    const auto& sender_position_component = sender_entity.Get<PositionComponent>();
    const HexGridPosition& sender_current_position = sender_position_component.GetPosition();
    const HexGridPosition& sender_prev_position = dash_component.GetLastPosition();
    const int sender_size_units = sender_position_component.GetRadius();

    if (dash_component.ApplyToAll())
    {
        for (const auto& other : world_->GetAll())
        {
            // Ignore collision with:
            // - self
            // - source entity
            const EntityID other_id = other->GetID();
            if (dash_entity.GetID() == other_id || sender_id == other_id)
            {
                continue;
            }

            // Can the entity be collided with
            if (!EntityHelper::IsCollidable(*other))
            {
                continue;
            }

            // Already collided with this entity, ignore
            if (filtering_component.HasCollidedWith(other_id))
            {
                continue;
            }

            if (CheckCapsuleCollision(sender_prev_position, sender_current_position, sender_size_units, other_id))
            {
                filtering_component.AddCollidedWith(other_id);
                out_new_collision.emplace_back(other_id);
            }
        }
    }
    else
    {
        const EntityID target_id = dash_component.GetReceiverID();

        if (!filtering_component.HasOldTarget(target_id))
        {
            if (CheckCapsuleCollision(sender_prev_position, sender_current_position, sender_size_units, target_id))
            {
                filtering_component.AddCollidedWith(target_id);
                out_new_collision.emplace_back(target_id);
            }
        }
    }

    return !out_new_collision.empty();
}

bool DashSystem::CheckCapsuleCollision(
    const HexGridPosition& first_position,
    const HexGridPosition& second_position,
    const int radius,
    const EntityID other_entity_id) const
{
    const HexGridPosition direction = second_position - first_position;
    const int distance = direction.Length();

    const auto& other_entity = world_->GetByID(other_entity_id);
    const auto& other_position_component = other_entity.Get<PositionComponent>();

    // TODO(oleksandr) We might have to optimize it later, seems like there should be much optimal way to check capsule
    // shape collision for entity
    for (int i = 1; i <= distance; ++i)
    {
        const HexGridPosition position = first_position + (direction.ToNormalizedAndScaled() * i).ToUnits();

        // Found collision with other entity
        if (other_position_component.IsPositionsIntersecting(position, radius))
        {
            return true;
        }
    }

    return false;
}

}  // namespace simulation
