#pragma once

#include <unordered_set>

#include "ecs/event.h"
#include "ecs/system.h"
#include "utility/gtest_friend.h"

namespace simulation
{

namespace event_data
{
struct ApplyShieldGain;
struct MarkCreated;
struct MarkDestroyed;
struct ShieldWasHit;
struct ShieldCreated;
struct ShieldDestroyed;
struct Shield;
}  // namespace event_data

/* -------------------------------------------------------------------------------------------------------
 * AttachedEntitySystem
 *
 * Handles the shields inside the world
 * --------------------------------------------------------------------------------------------------------
 */
class AttachedEntitySystem : public System
{
    typedef System Super;
    typedef AttachedEntitySystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSpawnable;
    }

private:
    void TimeStepMarkEntity(const Entity& mark_entity) const;
    void TimeStepShieldEntity(const Entity& shield_entity) const;

    // Listen to world events
    void OnShieldCreated(const event_data::ShieldCreated& data);
    void OnShieldDestroyed(const event_data::ShieldDestroyed& data);
    void OnShieldWasHit(const event_data::ShieldWasHit&);
    void OnOtherShieldEvent(const Event& event);
    void OnGainShieldEvent(const event_data::ApplyShieldGain& shield_gain);
    void OnMarkCreated(const event_data::MarkCreated& event);
    void OnMarkDestroyed(const event_data::MarkDestroyed& event);

    void TryToActivateAbilitiesForEventType(const Entity& shield_entity, const EventType event_type) const;

    void ApplyShieldGain(const EntityID receiver_id, const FixedPoint gain_amount, const int duration_time_ms);
    void ApplyStartingShield(const Entity& entity);

    // Keep track of entities in order to activate their starting shield
    std::unordered_set<EntityID> starting_shield_activated_;

    // Google test friends
#ifdef GOOGLE_UNIT_TEST
    // Shield system tests
    FRIEND_TEST(ShieldSystemTest, ApplyEffectToShieldsFullDestruction);
    FRIEND_TEST(ShieldSystemTest, ApplyEffectToShieldsPartialDestruction);
#endif  // GOOGLE_UNIT_TEST
};

}  // namespace simulation
