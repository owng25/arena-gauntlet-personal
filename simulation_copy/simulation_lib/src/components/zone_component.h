#pragma once

#include "data/constants.h"
#include "data/zone_data.h"
#include "ecs/component.h"
#include "utility/time.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * ZoneComponent
 *
 * This class holds the details of a zone spawned in the world
 * --------------------------------------------------------------------------------------------------------
 */
class ZoneComponent : public Component
{
public:
    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<ZoneComponent>(*this);
    }
    void Init() override
    {
        time_step_count_ = 0;
    }

    // Sets the data from ZoneData
    void SetComponentData(const ZoneData& data)
    {
        data_ = data;
        // Don't keep pointer to the skill data here
        data_.skill_data.reset();
    }

    // From which entity is this zone spawned
    void SetSenderID(const EntityID sender_id)
    {
        data_.sender_id = sender_id;
    }
    EntityID GetSenderID() const
    {
        return data_.sender_id;
    }

    // The original entity the zone is spawned from, i.e. splash
    // Used when we may need to know if the zone is spawned from something other than an Illuvial
    void SetOriginalSenderID(const EntityID sender_id)
    {
        data_.original_sender_id = sender_id;
    }
    EntityID GetOriginalSenderID() const
    {
        return data_.original_sender_id;
    }

    // How often should the zone activate
    void SetFrequencyMs(const int frequency_ms)
    {
        data_.frequency_ms = frequency_ms;
    }
    int GetFrequencyMs() const
    {
        return data_.frequency_ms;
    }

    // Skipped activations
    void GetActivationsToSkip(const std::vector<int>& activations_to_skip)
    {
        data_.skip_activations = activations_to_skip;
    }

    const std::vector<int>& GetActivationsToSkip() const
    {
        return data_.skip_activations;
    }

    bool ShouldSkipActivation(const int index) const
    {
        for (const int activation_to_skip : GetActivationsToSkip())
        {
            if (activation_to_skip == index)
            {
                return true;
            }
        }

        return false;
    }

    // Zone shape
    void SetShape(const ZoneEffectShape shape)
    {
        data_.shape = shape;
    }
    ZoneEffectShape GetShape() const
    {
        return data_.shape;
    }

    // Zone radius
    void SetRadiusSubUnits(const int radius_sub_units)
    {
        data_.radius_sub_units = radius_sub_units;
    }
    int GetRadiusSubUnits() const
    {
        return data_.radius_sub_units;
    }
    int GetRadiusUnits() const
    {
        return Math::SubUnitsToUnits(GetRadiusSubUnits());
    }
    int GetMaxRadiusSubUnits() const
    {
        return data_.max_radius_sub_units;
    }
    void IncreaseRadiusSubUnits(const int value_sub_units)
    {
        data_.radius_sub_units += value_sub_units;
        // Only increase radius if we are not at max and if there is a max radius
        if (data_.max_radius_sub_units != 0 && data_.radius_sub_units > data_.max_radius_sub_units)
        {
            data_.radius_sub_units = data_.max_radius_sub_units;
        }
    }

    // Zone width
    void SetWidthSubUnits(const int width_sub_units)
    {
        data_.width_sub_units = width_sub_units;
    }
    int GetWidthSubUnits() const
    {
        return data_.width_sub_units;
    }
    int GetWidthUnits() const
    {
        return Math::SubUnitsToUnits(GetWidthSubUnits());
    }

    // Zone height
    void SetHeightSubUnits(const int height_sub_units)
    {
        data_.height_sub_units = height_sub_units;
    }
    int GetHeightSubUnits() const
    {
        return data_.height_sub_units;
    }
    int GetHeightUnits() const
    {
        return Math::SubUnitsToUnits(GetHeightSubUnits());
    }

    // Zone spawn direction in degrees from origin, for directional zones
    void SetSpawnDirectionDegrees(const int spawn_direction_degrees)
    {
        data_.spawn_direction_degrees = spawn_direction_degrees;
    }
    int GetSpawnDirectionDegrees() const
    {
        return data_.spawn_direction_degrees;
    }

    // Zone direction in degrees from spawn direction, for directional zones
    void SetDirectionDegrees(const int direction_degrees)
    {
        data_.direction_degrees = direction_degrees;
    }
    int GetDirectionDegrees() const
    {
        return data_.direction_degrees;
    }

    // Growth rate in subunits per step
    void SetGrowthRateSubUnitsPerStep(const int growth_rate_sub_units_per_step)
    {
        data_.growth_rate_sub_units_per_time_step = growth_rate_sub_units_per_step;
    }
    int GetGrowthRateSubUnitsPerStep() const
    {
        return data_.growth_rate_sub_units_per_time_step;
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

    bool IsGlobalOverlap() const
    {
        return data_.global_collision_id != kInvalidGlobalCollisionID;
    }

    int GetGlobalCollisionID() const
    {
        return data_.global_collision_id;
    }

    // TimeStep count
    int GetAndIncrementTimeStepCount()
    {
        return time_step_count_++;
    }

    // Duration of the zone
    int GetDurationTimeSteps() const
    {
        return Time::MsToTimeSteps(data_.duration_ms);
    }

    // Check if zone is channeled
    bool IsChanneled() const
    {
        return data_.is_channeled;
    }

    // Check if zone is channeled
    const SourceContextData& GetSourceContext() const
    {
        return data_.source_context;
    }

    bool CanDestroyWithSender() const
    {
        return data_.destroy_with_sender;
    }

private:
    // Zone specific data
    ZoneData data_{};

    // Keep track of time step count
    int time_step_count_ = 0;
};

}  // namespace simulation
