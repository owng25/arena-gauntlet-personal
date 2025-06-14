#include "aura_system.h"

#include "components/aura_component.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"

namespace simulation
{

bool AuraSystem::ShouldDestroyAura(const Entity& aura) const
{
    const auto& aura_component = aura.Get<AuraComponent>();
    const auto& aura_data = aura_component.GetComponentData();

    // The entitiy the aura was applied to does not exist
    const auto& receiver_ptr = world_->GetByIDPtr(aura_data.receiver_id);
    if (!receiver_ptr || !receiver_ptr->IsActive())
    {
        return true;
    }

    // Effect sender entity does not exist
    const auto& effect_sender_ptr = world_->GetByIDPtr(aura_data.effect_sender_id);
    if (!effect_sender_ptr || !effect_sender_ptr->IsActive())
    {
        return true;
    }

    // Aura is expired
    const int duration_time_steps = Time::MsToTimeSteps(aura_data.effect.lifetime.duration_time_ms);
    if (duration_time_steps != kTimeInfinite)
    {
        const int expiry_time_step = aura_component.GetCreatedAtTimeStep() + duration_time_steps;
        if (world_->GetTimeStepCount() > expiry_time_step)
        {
            return true;
        }
    }

    return false;
}

void AuraSystem::RemoveAuraEffects(const Entity& aura)
{
    const EntityID aura_id = aura.GetID();

    if (!applied_effects_.contains(aura_id))
    {
        return;
    }

    for (const auto& [receiver_id, effect_state_ptr] : applied_effects_[aura_id])
    {
        if (world_->HasEntity(receiver_id))
        {
            const Entity& receiver = world_->GetByID(receiver_id);
            world_->GetAttachedEffectsHelper().RemoveAttachedEffect(receiver, effect_state_ptr);
        }
    }

    applied_effects_.erase(aura.GetID());
}

void AuraSystem::DestroyAura(const Entity& aura)
{
    RemoveAuraEffects(aura);
    world_->EmitEvent<EventType::kMarkAuraAsDestroyed>(event_data::Marked{.entity_id = aura.GetID()});
}

void AuraSystem::RefreshAuraEffects(const Entity& aura)
{
    std::vector<EntityID>& new_targets = targeting_cache_;
    new_targets.clear();
    FindAuraTargets(*world_, aura, &new_targets);

    // List of effects already applied by this aura
    std::vector<EffectInfo>& effects_info = applied_effects_[aura.GetID()];

    // Remove all effects applied to entities that could not be targeted atm for whatever reason
    bool with_changes = false;
    const auto& attached_effects_helper = world_->GetAttachedEffectsHelper();
    for (const EffectInfo& effect_info : effects_info)
    {
        if (world_->HasEntity(effect_info.receiver) &&
            std::find(new_targets.begin(), new_targets.end(), effect_info.receiver) == new_targets.end())
        {
            attached_effects_helper.RemoveAttachedEffect(world_->GetByID(effect_info.receiver), effect_info.effect);
            with_changes = true;
        }
    }

    // Add new effects
    const auto& aura_data = aura.Get<AuraComponent>().GetComponentData();
    for (const EntityID effect_receiver_id : new_targets)
    {
        bool found_existing = false;

        for (auto& effect_info : effects_info)
        {
            if (effect_info.receiver == effect_receiver_id)
            {
                found_existing = true;
                break;
            }
        }

        if (found_existing)
        {
            continue;
        }

        LogDebug(aura.GetID(), "Aura applies effect to entity {}", effect_receiver_id);
        world_->EmitEvent<EventType::kTryToApplyEffect>(event_data::Effect{
            .sender_id = aura_data.effect_sender_id,
            .receiver_id = effect_receiver_id,
            .data = aura_data.effect,
            .state =
                EffectState{
                    .source_context = aura_data.source_context,
                    .sender_stats = world_->GetFullStats(aura_data.effect_sender_id),
                    .attached_from_entity_id = aura.GetID()},
        });
        with_changes = true;
    }

    if (with_changes)
    {
        // Since we don't have a way to obtain effect state on application the list of current effects must be updated
        effects_info.clear();

        auto scan_effects = [&](const EntityID receiver_id, const std::vector<AttachedEffectStatePtr>& states)
        {
            for (auto& effect_state : states)
            {
                if (effect_state->effect_state.attached_from_entity_id == aura.GetID())
                {
                    effects_info.push_back({.receiver = receiver_id, .effect = effect_state});
                }
            }
        };

        // Still can use the list of new targets as a hint
        for (const EntityID effect_receiver_id : new_targets)
        {
            if (!world_->HasEntity(effect_receiver_id)) continue;

            const auto& receiver = world_->GetByID(effect_receiver_id);

            if (!receiver.Has<AttachedEffectsComponent>()) continue;

            const auto& attached_effects_component = receiver.Get<AttachedEffectsComponent>();

            for (auto& [stat_type, buffs] : attached_effects_component.GetBuffsState().buffs)
            {
                scan_effects(effect_receiver_id, buffs);
            }

            for (auto& [stat_type, debuffs] : attached_effects_component.GetBuffsState().debuffs)
            {
                scan_effects(effect_receiver_id, debuffs);
            }
        }
    }
}

void AuraSystem::FindAuraTargets(World& world, const Entity& aura, std::vector<EntityID>* out_targets)
{
    AllegianceType allegiance = AllegianceType::kNone;
    const auto& aura_data = aura.Get<AuraComponent>().GetComponentData();

    // Determine targeting parameters based on type of effect
    auto& aura_receiver = world.GetByID(aura_data.receiver_id);
    if (aura_data.effect.type_id.type == EffectType::kBuff)
    {
        allegiance = AllegianceType::kAlly;
    }
    else if (aura_data.effect.type_id.type == EffectType::kDebuff)
    {
        allegiance = AllegianceType::kEnemy;
    }
    else
    {
        world.LogErr(aura.GetID(), "Unsupported effect in aura");
        return;
    }

    // If aura itself and aura receiver are in different teams, allegiance must be inverted
    if (aura_receiver.GetTeam() != aura.GetTeam())
    {
        allegiance = allegiance == AllegianceType::kAlly ? AllegianceType::kEnemy : AllegianceType::kAlly;
    }

    constexpr bool only_focusers = false;
    const bool targeting_self = allegiance == AllegianceType::kAlly;
    std::unordered_set<EntityID> ignored_targets{};

    // Find targets
    world.GetTargetingHelper().GetEntitiesWithinRange(
        aura_data.receiver_id,
        allegiance,
        aura_data.effect.radius_units,
        targeting_self,
        only_focusers,
        ignored_targets,
        out_targets);
}

void AuraSystem::TimeStep(const Entity& aura)
{
    if (aura.IsActive() && EntityHelper::IsAnAura(aura))
    {
        if (ShouldDestroyAura(aura))
        {
            DestroyAura(aura);
        }
        else
        {
            RefreshAuraEffects(aura);
        }
    }
}

}  // namespace simulation
