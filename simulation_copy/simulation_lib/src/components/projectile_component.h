#pragma once

#include "data/constants.h"
#include "data/projectile_data.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * ProjectileComponent
 *
 * This class holds the details of a projectile spawned in the world
 * --------------------------------------------------------------------------------------------------------
 */
class ProjectileComponent : public Component
{
public:
    // Component initialisation
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<ProjectileComponent>(*this);
    }

    // Sets the data from ProjectileData
    void SetComponentData(const ProjectileData& data)
    {
        data_ = data;
        // Don't keep pointer to the skill data here
        data_.skill_data.reset();
    }

    // From which entity is this projectile spawned
    void SetSenderID(const EntityID sender_id)
    {
        data_.sender_id = sender_id;
    }
    EntityID GetSenderID() const
    {
        return data_.sender_id;
    }

    // To which entity is this projectile targeted
    void SetReceiverID(const EntityID receiver_id)
    {
        data_.receiver_id = receiver_id;
    }
    EntityID GetReceiverID() const
    {
        return data_.receiver_id;
    }

    // Can we go through other combat units?
    void SetIsBlockable(const bool is_blockable)
    {
        data_.is_blockable = is_blockable;
    }
    bool IsBlockable() const
    {
        return data_.is_blockable;
    }

    // Do we follow the target
    void SetIsHoming(const bool is_homing)
    {
        data_.is_homing = is_homing;
    }
    bool IsHoming() const
    {
        return data_.is_homing;
    }

    // This only has an effect if blockable is set to false.
    bool ApplyToAll() const
    {
        return data_.apply_to_all;
    }
    void SetApplyToAll(const bool apply_to_all)
    {
        data_.apply_to_all = apply_to_all;
    }

    // Checks if this projectile should check collision with everything at every time step
    bool CanCheckForCollisions() const
    {
        // Check collision if this projectile can be blocked by something else
        const bool is_blockable = IsBlockable();
        if (!is_blockable)
        {
            // Apply to all is only relevant if is_blockable = false
            return ApplyToAll();
        }

        return is_blockable;
    }

    // Check guidance for ability to deploy to airborne target
    bool CanDeployToAirborne() const
    {
        return data_.deployment_guidance.Contains(GuidanceType::kAirborne);
    }

    // Check guidance for ability to deploy to underground target
    bool CanDeployToUnderground() const
    {
        return data_.deployment_guidance.Contains(GuidanceType::kUnderground);
    }

    // Determines if the projectile can proceed after hitting the target entity
    bool ContinueAfterTarget() const
    {
        return data_.continue_after_target;
    }
    void SetContinueAfterTarget(const bool value)
    {
        data_.continue_after_target = value;
    }

    EntityID GetBlockedBy() const
    {
        return blocked_by_;
    }

    void SetBlockedBy(const EntityID entity_id)
    {
        blocked_by_ = entity_id;
    }

private:
    // Projectile specific data
    ProjectileData data_{};

    // Entity id that blocked this projectile
    EntityID blocked_by_ = kInvalidEntityID;
};

}  // namespace simulation
