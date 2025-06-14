#pragma once

#include "data/mark_data.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * MarkComponent
 *
 * This class holds the details of a mark spawned in the world
 * --------------------------------------------------------------------------------------------------------
 */
class MarkComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<MarkComponent>(*this);
    }

    // Sets the data from MarkData
    void SetComponentData(const MarkData& data)
    {
        data_ = data;
    }

    // From which entity is this mark spawned
    void SetSenderID(const EntityID sender_id)
    {
        data_.sender_id = sender_id;
    }
    EntityID GetSenderID() const
    {
        return data_.sender_id;
    }

    // The receiver of this mark
    void SetReceiverID(const EntityID receiver_id)
    {
        data_.receiver_id = receiver_id;
    }
    EntityID GetReceiverID() const
    {
        return data_.receiver_id;
    }

    bool ShouldDestroyOnSenderDeath() const
    {
        return data_.should_destroy_on_sender_death;
    }

    // Get abilities
    const std::vector<std::shared_ptr<const AbilityData>>& GetAbilities() const
    {
        return data_.abilities_data;
    }

private:
    // Mark specific data
    MarkData data_{};
};

}  // namespace simulation
