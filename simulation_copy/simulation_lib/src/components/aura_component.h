#pragma once

#include "data/aura_data.hpp"
#include "ecs/component.h"

namespace simulation
{

class AuraComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<AuraComponent>(*this);
    }

    // Sets the data from ShieldData
    void SetComponentData(AuraData data)
    {
        data_ = std::move(data);
    }

    const AuraData& GetComponentData() const
    {
        return data_;
    }

    void SetCreatedAtTimeStep(int time_step)
    {
        created_at_time_step_ = time_step;
    }

    int GetCreatedAtTimeStep() const
    {
        return created_at_time_step_;
    }

private:
    AuraData data_{};
    int created_at_time_step_ = 0;
};

}  // namespace simulation
