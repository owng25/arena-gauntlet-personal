#pragma once

#include "data/augment/augment_instance_data.h"
#include "ecs/component.h"
#include "utility/vector_helper.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * AugmentComponent
 *
 * This class keeps track of the attached augments on entity
 * --------------------------------------------------------------------------------------------------------
 */
class AugmentComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<AugmentComponent>(*this);
    }

    size_t GetEquippedAugmentsSize() const
    {
        return equipped_augments_.size();
    }

    const std::vector<AugmentInstanceData>& GetEquippedAugments() const
    {
        return equipped_augments_;
    }

    void AddAugment(const AugmentInstanceData& augment_data)
    {
        equipped_augments_.push_back(augment_data);
    }

    bool RemoveAugment(const AugmentInstanceData& augment_data)
    {
        const size_t prev_size = equipped_augments_.size();
        VectorHelper::EraseValue(equipped_augments_, augment_data);
        return prev_size != equipped_augments_.size();
    }

    void ClearAugments()
    {
        equipped_augments_.clear();
    }

    bool HasAugment(const AugmentInstanceData& augment_data) const
    {
        return VectorHelper::ContainsValue(equipped_augments_, augment_data);
    }
    bool HasAugmentTypeID(const AugmentTypeID& type_id) const
    {
        return VectorHelper::ContainsValue(equipped_augments_, type_id);
    }

private:
    std::vector<AugmentInstanceData> equipped_augments_;
};
}  // namespace simulation
