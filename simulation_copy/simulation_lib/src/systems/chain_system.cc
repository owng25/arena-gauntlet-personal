#include "systems/chain_system.h"

#include "components/abilities_component.h"
#include "components/chain_component.h"
#include "components/filtering_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"
#include "utility/entity_helper.h"

namespace simulation
{
void ChainSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kAbilityDeactivated>(this, &Self::OnAbilityDeactivated);
    world_->SubscribeMethodToEvent<EventType::kSkillNoTargets>(this, &Self::OnSkillNoTargets);
    world_->SubscribeMethodToEvent<EventType::kFocusNeverDeactivated>(this, &Self::OnFocusNeverDeactivated);
}

void ChainSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }
}

void ChainSystem::OnAbilityDeactivated(const event_data::AbilityDeactivated& data)
{
    // Get the chain entity
    ChainFindResult find_result;
    GetChainEntity("ChainSystem::OnAbilityDeactivated", data.sender_id, &find_result);
    if (!find_result.chain_entity)
    {
        return;
    }
    TryToBounceChain(find_result);
}

void ChainSystem::TryToBounceChain(const ChainFindResult& find_result)
{
    static constexpr std::string_view method_name = "ChainSystem::TryToBounceChain";
    const auto& old_chain_entity = *find_result.chain_entity;
    const EntityID old_chain_id = old_chain_entity.GetID();
    const auto& old_abilities_component = old_chain_entity.Get<AbilitiesComponent>();

    // Still the chain has some active abilities
    if (old_abilities_component.HasActiveAbility())
    {
        LogDebug(old_chain_id, "{} - Still has active ability. Waiting for it to Finish.", method_name);
        return;
    }

    // NOTE: This needs to handle special cases
    // Because the chain can be deployment using a projectile for example:
    // 2. Chain OnAbilityDeactivated fires
    //   - By this time this is called a projectile should have already spawned
    //   - Can't call this method because there exists a child projectile still alive
    // 3. Projectile - OnAbilityDeactivated fires
    //   - Can call this method, because the the projectile is the only child of the chain
    //   - Spawns new chains
    //   - Old chain gets destroyed
    if (find_result.is_sender_a_chain)
    {
        // Direct chain event
        if (world_->HasAnyChildEntities(old_chain_id))
        {
            LogDebug(old_chain_id, "{} - Still has other child entities active. Waiting for them to die.", method_name);
            return;
        }
    }
    else if (find_result.is_parent_a_chain)
    {
        // Most likely the sender_id is a projectile an the parent is the chain
        // NOTE: old_chain_id == find_result.parent_id
        const std::unordered_set<EntityID> chain_children = world_->GetChildEntities(old_chain_id);

        // Does it contain only the sender_id (which is not a chain)
        assert(!EntityHelper::IsAChain(*world_, find_result.sender_id));
        if (!(chain_children.count(find_result.sender_id) > 0 && chain_children.size() == 1))
        {
            LogDebug(
                old_chain_id,
                "{} - Still has some other child entities active. Waiting for the rest to die.",
                method_name);
            return;
        }
    }

    // Init data for new chain
    const auto& old_chain_component = old_chain_entity.Get<ChainComponent>();

    // Sender data
    bool old_destroyed = false;
    const auto& sender_combat_unit_entity = world_->GetByID(old_chain_component.GetCombatUnitSenderID());
    if (!sender_combat_unit_entity.IsActive())
    {
        // Destroy old chain
        LogDebug(
            old_chain_id,
            "{} - sender_id = {} is not longer active. Destroying chain",
            method_name,
            old_chain_component.GetCombatUnitSenderID());
        world_->BuildAndEmitEvent<EventType::kMarkChainAsDestroyed>(old_chain_id);
        old_destroyed = true;
    }

    // Chain Data
    const auto& old_filtered_component = old_chain_entity.Get<FilteringComponent>();
    ChainData new_chain_data = old_chain_component.GetComponentData();
    new_chain_data.sender_id = old_chain_entity.GetID();

    // Decrease chain number
    new_chain_data.chain_number--;

    // Assign old targets from the previous chain
    new_chain_data.old_target_entities = old_filtered_component.GetOldTargetEntities();

    // Spawn new chains
    if (new_chain_data.chain_number >= 1)
    {
        LogDebug(old_chain_id, "{} - Spawn new chains", method_name);
        EntityFactory::SpawnChainPropagation(*world_, sender_combat_unit_entity.GetTeam(), new_chain_data);
    }

    if (!old_destroyed)
    {
        LogDebug(old_chain_id, "{} - Destroying OLD chain", method_name);
        world_->BuildAndEmitEvent<EventType::kMarkChainAsDestroyed>(old_chain_id);
    }
}

void ChainSystem::GetChainEntity(
    const std::string_view method_context,
    const EntityID sender_id,
    ChainFindResult* out_result) const
{
    assert(out_result);
    out_result->sender_id = sender_id;
    if (!world_->HasEntity(sender_id))
    {
        return;
    }

    const auto& sender_entity = world_->GetByID(sender_id);
    out_result->parent_id = sender_entity.GetParentID();

    out_result->is_sender_a_chain = EntityHelper::IsAChain(*world_, sender_id);
    out_result->is_parent_a_chain = EntityHelper::IsAChain(*world_, out_result->parent_id);

    // Only care about if the sender_id is a chain or the parent of sender_id is a chain
    if (!out_result->is_sender_a_chain && !out_result->is_parent_a_chain)
    {
        return;
    }

    // Get the chain entity
    if (out_result->is_sender_a_chain)
    {
        LogDebug(sender_id, "{} - Is a chain", method_context);
        out_result->chain_entity = world_->GetByIDPtr(sender_id);
    }
    else if (out_result->is_parent_a_chain)
    {
        LogDebug(sender_id, "{} - parent_id = {} Is a chain", method_context, out_result->is_parent_a_chain);
        out_result->chain_entity = world_->GetByIDPtr(out_result->parent_id);
    }
    assert(out_result->chain_entity != nullptr);
}

void ChainSystem::OnSkillNoTargets(const event_data::Skill& data)
{
    const EntityID id = data.sender_id;

    // We are only interested in chain
    if (!EntityHelper::IsAChain(*world_, id))
    {
        return;
    }

    // Destroy chain as we don't have any more targets
    world_->BuildAndEmitEvent<EventType::kMarkChainAsDestroyed>(id);
}

void ChainSystem::OnFocusNeverDeactivated(const event_data::Focus& data)
{
    const EntityID id = data.entity_id;

    // We are only interested in chain
    if (!EntityHelper::IsAChain(*world_, id))
    {
        return;
    }

    // Get the chain entity
    ChainFindResult find_result;
    find_result.chain_entity = world_->GetByIDPtr(id);
    find_result.is_sender_a_chain = true;
    find_result.sender_id = id;

    // Try to bounce to another target
    TryToBounceChain(find_result);
}

}  // namespace simulation
