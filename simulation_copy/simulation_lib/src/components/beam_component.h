#pragma once

#include "data/beam_data.h"
#include "data/constants.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * BeamComponent
 *
 * This class holds the details of a beam spawned in the world
 * --------------------------------------------------------------------------------------------------------
 */
class BeamComponent : public Component
{
public:
    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<BeamComponent>(*this);
    }

    // Sets the data from BeamData
    void SetComponentData(const BeamData& data)
    {
        data_ = data;
        // Don't keep pointer to the skill data here
        data_.skill_data.reset();
    }

    // From which entity is this beam spawned
    void SetSenderID(const EntityID sender_id)
    {
        data_.sender_id = sender_id;
    }
    EntityID GetSenderID() const
    {
        return data_.sender_id;
    }

    // To which entity is this beam aiming
    void SetReceiverID(const EntityID receiver_id)
    {
        data_.receiver_id = receiver_id;
    }
    EntityID GetReceiverID() const
    {
        return data_.receiver_id;
    }

    // How often should the beam activate
    void SetFrequencyMs(const int frequency_ms)
    {
        data_.frequency_ms = frequency_ms;
    }
    int GetFrequencyMs() const
    {
        return data_.frequency_ms;
    }

    // Beam width
    void SetWidthSubUnits(const int width_sub_units)
    {
        data_.width_sub_units = width_sub_units;
    }
    int GetWidthSubUnits() const
    {
        return data_.width_sub_units;
    }

    // Beam length in world sub units
    void SetWorldLengthSubUnits(const int length_sub_units)
    {
        data_.length_world_sub_units = length_sub_units;
    }
    int GetWorldLengthSubUnits() const
    {
        return data_.length_world_sub_units;
    }

    // Beam direction in degrees from origin
    void SetDirectionDegrees(const int direction_degrees)
    {
        data_.direction_degrees = direction_degrees;
    }
    int GetDirectionDegrees() const
    {
        return data_.direction_degrees;
    }

    // Whether to apply effect only once
    void SetApplyOnce(const bool apply_once)
    {
        data_.apply_once = apply_once;
    }
    bool IsApplyOnce() const
    {
        return data_.apply_once;
    }

    // Whether the beam is homing
    void SetHoming(const bool homing)
    {
        data_.is_homing = homing;
    }
    bool IsHoming() const
    {
        return data_.is_homing;
    }

    // Whether the beam is blockable
    void SetBlockable(const bool blockable)
    {
        data_.is_blockable = blockable;
    }
    bool IsBlockable() const
    {
        return data_.is_blockable;
    }

    void SetIsBlockedByEntityID(const EntityID entity_id)
    {
        blocked_by_entity_id_ = entity_id;
    }
    bool IsBlockedByAnEntityID() const
    {
        return blocked_by_entity_id_ != kInvalidEntityID;
    }
    EntityID GetBlockedByEntityID() const
    {
        return blocked_by_entity_id_;
    }

    // Who the beam is blocked by
    void SetBlockAllegiance(const AllegianceType block_allegiance)
    {
        data_.block_allegiance = block_allegiance;
    }
    AllegianceType GetBlockAllegiance() const
    {
        return data_.block_allegiance;
    }

    // TimeStep count
    int GetAndIncrementTimeStepCount()
    {
        return time_step_count_++;
    }

    const SourceContextData& GetSourceContext() const
    {
        return data_.source_context;
    }

private:
    // Beam specific data
    BeamData data_{};

    // Beam is blocked by this entity id
    EntityID blocked_by_entity_id_ = kInvalidEntityID;

    // Keep track of time step count
    int time_step_count_ = 0;
};

}  // namespace simulation
