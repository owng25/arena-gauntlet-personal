#pragma once

#include <unordered_set>

#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * FilteringComponent
 *
 * This class holds details for entities that specific targeting requirements such that we
 * don't target the same entity twice, or that we must prioritize new targets intead of old ones.
 * --------------------------------------------------------------------------------------------------------
 */
class FilteringComponent : public Component
{
public:
    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<FilteringComponent>(*this);
    }
    void Init() override
    {
        old_target_entities_.clear();
    }

    void SetOnlyNewTargets(const bool value)
    {
        only_new_targets_ = value;
    }
    void SetPrioritizeNewTargets(const bool value)
    {
        prioritize_new_targets_ = value;
    }
    bool GetPrioritizeNewTargets() const
    {
        return prioritize_new_targets_;
    }
    bool GetOnlyNewTargets() const
    {
        return only_new_targets_;
    }

    // Add/Query old target
    void AddOldTarget(const EntityID id)
    {
        old_target_entities_.insert(id);
    }
    void AddOldTargets(const std::vector<EntityID>& target_ids)
    {
        for (const EntityID target_id : target_ids)
        {
            AddOldTarget(target_id);
        }
    }

    bool HasOldTarget(const EntityID id) const
    {
        return old_target_entities_.count(id) > 0;
    }

    // Getter/Setter directly on the set
    const std::unordered_set<EntityID>& GetOldTargetEntities() const
    {
        return old_target_entities_;
    }
    void SetOldTargetEntities(const std::unordered_set<EntityID>& old_target_entities)
    {
        old_target_entities_ = old_target_entities;
    }

    // Add an entity to the list of entities to be ignored
    void AddIgnoredEntityID(const EntityID entity_id)
    {
        ignored_receiver_ids_.insert(entity_id);
    }
    const std::unordered_set<EntityID>& GetIgnoredReceiverIDs() const
    {
        return ignored_receiver_ids_;
    }

    // Add an entity to the set of entities owner entity has collided with
    void AddCollidedWith(const EntityID entity_id)
    {
        collided_with_.insert(entity_id);
    }

    bool HasCollidedWith(const EntityID entity_id) const
    {
        return collided_with_.count(entity_id) != 0;
    }

private:
    // Keep a list of targets that we have already bounced to, and prioritise other ones.
    bool prioritize_new_targets_ = false;

    // Keep a list of targets that we have already bounced to, and never bounce to them again.
    bool only_new_targets_ = false;

    // Keep a list of targets that should be ignored
    std::unordered_set<EntityID> ignored_receiver_ids_;

    // Keep track of old entities we targeted
    std::unordered_set<EntityID> old_target_entities_;

    // Keep track of entities we collided with
    std::unordered_set<EntityID> collided_with_;
};

}  // namespace simulation
