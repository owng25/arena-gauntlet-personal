#pragma once

#include "data/constants.h"
#include "ecs/entity.h"
#include "utility/logger.h"
#include "utility/logger_consumer.h"

namespace simulation
{
class World;
/* -------------------------------------------------------------------------------------------------------
 * System
 *
 * This class forms the basis of a system in the ECS system.
 * --------------------------------------------------------------------------------------------------------
 */
class System : public LoggerConsumer
{
public:
    // System destructor
    ~System() override {}

    // System initialisation function
    virtual void Init(World* world);

    // Update function before the battle started event
    virtual void PreBattleStarted(const Entity&) {}

    // System update function, called before the first time step and after battle started event
    virtual void InitialTimeStep(const Entity&) {}

    // System update function, called every time step for each entity
    virtual void TimeStep(const Entity&) = 0;

    // System update function, called every time step for each entity
    virtual void PostTimeStep(const Entity&) {}

    // System update function, called at the end of time step
    // for every system (after per-entity PostTimeStep)
    virtual void PostSystemTimeStep() {}

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;
    int GetTimeStepCount() const override;

protected:
    // Owner world of this system
    World* world_ = nullptr;
};

}  // namespace simulation
