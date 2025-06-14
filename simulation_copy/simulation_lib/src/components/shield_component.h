#pragma once

#include "data/constants.h"
#include "data/shield_data.h"
#include "data/skill_data.h"
#include "ecs/component.h"
#include "ecs/event.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * ShieldComponent
 *
 * This class holds the details of a shield spawned in the world
 * --------------------------------------------------------------------------------------------------------
 */
class ShieldComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<ShieldComponent>(*this);
    }

    // Sets the data from ShieldData
    void SetComponentData(const ShieldData& data)
    {
        data_ = data;
    }

    // From which entity is this shield spawned
    void SetSenderID(const EntityID sender_id)
    {
        data_.sender_id = sender_id;
    }
    EntityID GetSenderID() const
    {
        return data_.sender_id;
    }

    // The receiver of this shield
    void SetReceiverID(const EntityID receiver_id)
    {
        data_.receiver_id = receiver_id;
    }
    EntityID GetReceiverID() const
    {
        return data_.receiver_id;
    }

    // What is the current shield value
    void SetValue(const FixedPoint value)
    {
        data_.value = value;
    }
    FixedPoint GetValue() const
    {
        return data_.value;
    }

    // Get event skills
    std::shared_ptr<SkillData> GetSkillForEvent(const EventType event_type)
    {
        if (data_.event_skills.count(event_type) > 0)
        {
            return data_.event_skills[event_type];
        }

        return nullptr;
    }

    // Get attached effects
    const std::vector<EffectData>& GetAttachedEffects() const
    {
        return data_.effects;
    }

    const SourceContextData& GetSourceContextData() const
    {
        return data_.source_context;
    }

private:
    // Shield specific data
    ShieldData data_{};
};

}  // namespace simulation
