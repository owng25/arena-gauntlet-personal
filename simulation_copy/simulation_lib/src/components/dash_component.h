#pragma once

#include "data/constants.h"
#include "data/dash_data.h"
#include "ecs/component.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * DashComponent
 *
 * This class holds the details of an entity for a dashing combat unit
 * --------------------------------------------------------------------------------------------------------
 */
class DashComponent : public Component
{
public:
    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<DashComponent>(*this);
    }

    // Sets the data from DashData
    void SetComponentData(const DashData& data)
    {
        data_ = data;
        // Don't keep pointer to the skill data here
        data_.skill_data.reset();
    }

    // From which entity is this dash spawned
    void SetSenderID(const EntityID sender_id)
    {
        data_.sender_id = sender_id;
    }
    EntityID GetSenderID() const
    {
        return data_.sender_id;
    }

    // To which entity is this dash targeted
    void SetReceiverID(const EntityID receiver_id)
    {
        data_.receiver_id = receiver_id;
    }
    EntityID GetReceiverID() const
    {
        return data_.receiver_id;
    }

    bool ApplyToAll() const
    {
        return data_.apply_to_all;
    }

    bool IsLandBehind() const
    {
        return data_.land_behind;
    }

    void SetLastPosition(const HexGridPosition& last_position)
    {
        last_position_ = last_position;
    }

    HexGridPosition GetLastPosition() const
    {
        return last_position_;
    }

private:
    // Dash specific data
    DashData data_{};

    // Position from previous timestamp
    HexGridPosition last_position_ = kInvalidHexHexGridPosition;
};

}  // namespace simulation
