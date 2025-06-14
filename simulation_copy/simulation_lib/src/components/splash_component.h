#pragma once

#include "data/constants.h"
#include "data/splash_data.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * SplashComponent
 *
 * This class holds the details of a splash spawned in the world
 * --------------------------------------------------------------------------------------------------------
 */
class SplashComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<SplashComponent>(*this);
    }

    // Sets all the data from SplashData
    void SetComponentData(const SplashData& data)
    {
        data_ = data;
    }
    const SplashData& GetComponentData() const
    {
        return data_;
    }

    // From which entity is this splash spawned
    void SetSenderID(const EntityID sender_id)
    {
        data_.sender_id = sender_id;
    }
    EntityID GetSenderID() const
    {
        return data_.sender_id;
    }

private:
    // Splash specific data
    SplashData data_{};
};

}  // namespace simulation
