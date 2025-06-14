#pragma once

#include "data/consumable/consumable_instance_data.h"
#include "ecs/component.h"
#include "utility/vector_helper.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * ConsumableComponent
 *
 * This class keeps track of the attached consumables on entity
 * --------------------------------------------------------------------------------------------------------
 */
class ConsumableComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<ConsumableComponent>(*this);
    }

    size_t GetEquippedConsumablesSize() const
    {
        return equipped_consumables_.size();
    }

    const std::vector<ConsumableInstanceData>& GetEquippedConsumables() const
    {
        return equipped_consumables_;
    }

    void AddConsumable(const ConsumableInstanceData& consumable_data)
    {
        equipped_consumables_.push_back(consumable_data);
    }

    void RemoveConsumable(const ConsumableInstanceData& consumable_data)
    {
        VectorHelper::EraseValue(equipped_consumables_, consumable_data);
    }

    void ClearConsumable()
    {
        equipped_consumables_.clear();
    }

    bool HasConsumable(const ConsumableInstanceData& consumable_data) const
    {
        return VectorHelper::ContainsValue(equipped_consumables_, consumable_data);
    }
    bool HasConsumableTypeID(const ConsumableTypeID& type_id) const
    {
        return VectorHelper::ContainsValue(equipped_consumables_, type_id);
    }

private:
    std::vector<ConsumableInstanceData> equipped_consumables_;
};
}  // namespace simulation
