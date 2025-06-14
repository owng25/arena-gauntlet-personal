#pragma once

#include <any>
#include <functional>
#include <memory>

namespace simulation
{

// Defines the type of event
enum class EventType : int
{
    // The empty event
    kNone = 0,

    // event_data::Created
    kCreated,

    // Used this to find the final position after a time step
    // event_data::Moved
    kMoved,

    // Specialized move event, only use this if you need to know multiple times
    // in the same time step how an entity moved.
    // event_data::Moved
    kMovedStep,

    // event_data::Blinked
    kBlinked,

    // event_data::StoppedMovement
    kStoppedMovement,

    // Notifies for a pathfinding attempt
    // event_data::FindPath
    kFindPath,

    // event_data::Deactivated
    kDeactivated,

    // event_data::Fainted
    kFainted,

    // event_data::AbilityActivated
    kAbilityActivated,

    // event_data::AbilityDeactivated
    kAbilityDeactivated,

    // Needed because we want to execute some events after all the AbilityDeactivated events are fired
    kAfterAbilityDeactivated,

    // event_data::AbilityInterrupted
    kAbilityInterrupted,

    // event_data::Skill
    kSkillWaiting,
    kSkillDeploying,
    kSkillChanneling,
    kSkillFinished,
    kSkillNoTargets,

    // Tells about targets updates during targeting frame
    kSkillTargetsUpdated,

    // event_data::EffectPackage
    kEffectPackageReceived,
    kEffectPackageMissed,
    kEffectPackageDodged,
    kEffectPackageBlocked,  // See PositiveState EffectPackageBlock

    // event_data::AppliedDamage
    kOnDamage,

    // event_data::StatChanged
    kHealthChanged,
    kEnergyChanged,
    kHyperChanged,

    // event_data::HealthGain
    kOnHealthGain,

    // event_data::ApplyHealthGain
    kApplyHealthGain,

    // event_data::ApplyShieldGain
    kApplyShieldGain,

    // event_data::ApplyEnergyGain
    kApplyEnergyGain,

    // event_data::Effect
    kOnEffectApplied,
    kTryToApplyEffect,
    kOnAttachedEffectApplied,
    kOnAttachedEffectRemoved,

    // event_data::ActivateAnyAbility
    kActivateAnyAbility,

    // event_data::CombatUnitCreated
    kCombatUnitCreated,

    // event_data::ChainCreated
    kChainCreated,

    // event_data::ChainBounced
    kChainBounced,

    // event_data::ChainDestroyed
    kChainDestroyed,

    // event_data::SplashCreated
    kSplashCreated,

    // event_data::SplashDestroyed
    kSplashDestroyed,

    // event_data::ProjectileCreated
    kProjectileCreated,

    // event_data::ProjectileBlocked
    kProjectileBlocked,

    // event_data::ProjectileDestroyed
    kProjectileDestroyed,

    // event_data::ZoneCreated
    kZoneCreated,

    // event_data::ZoneDestroyed
    kZoneDestroyed,

    // event_data::ZoneActivated
    kZoneActivated,

    // event_data::BeamCreated
    kBeamCreated,

    // event_data::BeamDestroyed
    kBeamDestroyed,

    // event_data::BeamUpdated
    kBeamUpdated,

    // event_data::BeamActivated
    kBeamActivated,

    // event_data::DashCreated
    kDashCreated,

    // event_data::DashDestroyed
    kDashDestroyed,

    // event_data::ShieldCreated
    kShieldCreated,

    // event_data::ShieldDestroyed
    kShieldDestroyed,

    // event_data::Shield
    kShieldWasHit,
    kShieldExpired,
    kShieldPendingDestroyed,

    // event_data::Displacement
    kDisplacementStarted,
    kDisplacementStopped,

    // event_data::MarkCreated
    kMarkCreated,

    // event_data::MarkDestroyed
    kMarkDestroyed,

    // event_data::Marked
    kMarkChainAsDestroyed,
    kMarkSplashAsDestroyed,
    kMarkProjectileAsDestroyed,
    kMarkZoneAsDestroyed,
    kMarkBeamAsDestroyed,
    kMarkDashAsDestroyed,
    kMarkAuraAsDestroyed,
    kMarkAttachedEntityAsDestroyed,

    // The entity with the retarget type of kNever has a target that is set but it is not active
    // event_data::Focus
    kFocusNeverDeactivated,

    // Focus was found for an entity
    // event_data::Focus
    kFocusFound,

    kFocusUnreachable,

    // event_data::CombatUnitStateChanged
    kCombatUnitStateChanged,

    // event_data::BattleStarted
    kBattleStarted,

    // event_data::BattleFinished
    kBattleFinished,

    // event_data::Hyperactive
    kGoneHyperactive,

    // event_data::OverloadStarted
    kOverloadStarted,

    // event_data::TimeStepped
    kTimeStepped,

    // EventData::PathfindingDebugData
    kPathfindingDebugData,

    // EventData::ZoneRadiusChanged
    kZoneRadiusChanged,

    // EventData::AuraCreated
    kAuraCreated,

    // EventData::AuraDestroyed
    kAuraDestroyed,

    // -new versions can be added above this line
    kNum
};

// Used if you want your event to be handled by a free floating function
// For example:
// // Free floating function definition
// void OnEventSomething(const Event& event) {}
//
// // Subscribe
// world_->SubscribeToEvent(EVENT_FUNCTION_SUBSCRIBER(EventType::EventX, OnEventSomething));
#define EVENT_FUNCTION_SUBSCRIBER(_event_type, _subscriber) _event_type, std::bind(&_subscriber, std::placeholders::_1)

class Event;
typedef std::function<void(const Event&)> EventCallback;
typedef std::shared_ptr<EventCallback> EventCallbackPtr;
typedef std::weak_ptr<EventCallback> EventCallbackWeakPtr;

// Handler for events that are subscribed
struct EventHandleID
{
    EventCallbackWeakPtr listener_weak_ptr;
    EventType type = EventType::kNone;
};

/* -------------------------------------------------------------------------------------------------------
 * Event
 *
 * This class forms the basis of any event in the ECS system
 * --------------------------------------------------------------------------------------------------------
 */
class Event final
{
public:
    // Event constructor always must have a type
    explicit Event(const EventType type) : type_(type) {}

    // Can not call the default constructor
    Event() = delete;

    // Copyable or Movable
    Event(const Event&) = default;
    Event& operator=(const Event&) = default;
    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    // Returns the type of this event
    EventType GetType() const
    {
        return type_;
    }
    int GetTypeAsInt() const
    {
        return static_cast<int>(type_);
    }

    // Sets the data for this event
    template <typename T>
    void Set(const T& data)
    {
        data_ = data;
    }

    // Gets the data for this event as T
    template <typename T>
    const T& Get() const
    {
        return std::any_cast<const T&>(data_);
    }

    // Checks if the Event type is in the valid range
    static bool IsEventTypeValid(const EventType event_type)
    {
        return IsEventTypeValid(static_cast<int>(event_type));
    }
    static bool IsEventTypeValid(const int event_type)
    {
        return event_type >= static_cast<int>(EventType::kNone) && event_type < static_cast<int>(EventType::kNum);
    }

    // Maximum number of event types in the game
    static constexpr int kMaxEvents = static_cast<int>(EventType::kNum);

private:
    // The type of this event
    EventType type_ = EventType::kNone;

    // The data for this event
    std::any data_{};
};  // class Event

}  // namespace simulation
