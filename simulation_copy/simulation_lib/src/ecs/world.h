#pragma once

#include <array>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "components/attached_effects_component.h"
#include "data/battle_result.h"
#include "data/combat_unit_data.h"
#include "data/constants.h"
#include "data/drone_augment/drone_augment_state.h"
#include "data/encounter_mod_instance_data.h"
#include "data/hyper_config.h"
#include "data/stats_data.h"
#include "data/world_effects_config.h"
#include "ecs/entity.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/event_types_map.h"
#include "ecs/system.h"
#include "hex_grid_config.h"
#include "profiling/illuvium_profiling.h"
#include "utility/attached_effects_helper.h"
#include "utility/augment_helper.h"
#include "utility/consumables_helper.h"
#include "utility/effect_package_helper.h"
#include "utility/equipment_helper.h"
#include "utility/grid_helper.h"
#include "utility/hex_grid_position.h"
#include "utility/ivector2d.h"
#include "utility/leveling_helper.h"
#include "utility/logger.h"
#include "utility/logger_consumer.h"
#include "utility/random_generator.h"
#include "utility/synergies_helper.h"
#include "utility/synergies_state_container.h"
#include "utility/targeting_helper.h"

namespace simulation
{
class DroneAugmentData;
}

namespace simulation
{

class GameDataContainer;

namespace event_data
{
struct BattleFinished;
struct Effect;
struct Fainted;
struct Marked;
}  // namespace event_data

// Configurable Params for leveling
class LevelingConfig
{
public:
    // Grow rate used for leveling
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44204724/Combat+Stat+Growth+by+Illuvial+Combat+Level
    FixedPoint leveling_grow_rate_percentage = 25_fp;

    // Should the leveling system be enabled?
    bool enable_leveling_system = true;
};

// Configurable Params for overload
class OverloadConfig
{
public:
    // When to start applying the overload damage to combat units
    int start_seconds_apply_overload_damage = 45;

    // How much the Damage Percentage will increase each frame
    FixedPoint increase_overload_damage_percentage = 5_fp;

    // Should the overload system be enabled?
    bool enable_overload_system = true;
};

// Configurable params for augments
class AugmentsConfig
{
public:
    // Max augments on illuvial
    size_t max_augments = 2;

    // Max drone augments per team
    size_t max_drone_augments = 3;
};

// Configurable params for consumables
class ConsumablesConfig
{
public:
    // Max consumables on illuvial
    size_t max_consumables = 2;
};

// Extra info for logs
class LogsConfig
{
public:
    // Should there be logs for when calculating current stats
    bool enable_calculate_live_stats = false;

    // Extra logs for innate abilities
    bool enable_innate_abilities = false;

    // Extra logs for attached effects
    bool enable_attached_effects = false;
};

class BattleConfig
{
public:
    // Config for the augments
    AugmentsConfig augments_config{};

    // Config for the consumables
    ConsumablesConfig consumables_config{};

    // Config for overload
    OverloadConfig overload_config{};

    // Config for leveling
    LevelingConfig leveling_config{};

    // Seed for the random number generator
    uint64_t random_seed = 0;

    // Grid height
    // Used for the row (r coordinate)
    // NOTE: In world scale kGridScale*151 = 1510 matches the previous hex grid size
    int grid_height = 75 * 2 + 1;

    // Grid width
    // Used for the column (q coordinate)
    int grid_width = grid_height;

    // Grid scale for internal representation of data
    // The scale is needed to avoid wasting resources in places where we iterate over sizes/spaces
    // This represents how many centimeters there are in one simulation grid unit
    // The number 10 was picked because multiples of 10 are easy to read
    // This makes 10cm in Unreal equal to one grid unit in the simulation
    int grid_scale = 10;

    // The number of rows from the center line of the arena.
    // NOTE: This counts from the middle row (r = 0) to the side.
    // Example: If this is 5 then this takes 11 rows
    int middle_line_width = 0;

    // If set to true, right before the battle starts, it will sort all the combat units by unique id
    // (ascending if random_seed is even and descending if random_seed is odd)
    bool sort_by_unique_id = true;

    // List of encounter mod instances for each team.
    // Key: Team.
    // Value: std::vector<EncounterModInstanceData>.
    std::unordered_map<Team, std::vector<EncounterModInstanceData>> teams_encounter_mods{};

    // If true, will require each unit to have enough space to contain any descendant from evolution/fusion tree when
    // placed.
    bool use_max_stage_placement_radius = false;

    // Max weapon amplifiers to equip on a weapon
    size_t max_weapon_amplifiers = 2;
};

// Configurable Params to construct the world
class WorldConfig
{
public:
    // Maximum attack speed allowed
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44270345/Attack+Speed#Max
    FixedPoint max_attack_speed = 250_fp;

    // The Scale factor for the world stats
    FixedPoint stats_constants_scale = 1_fp;

    // Amount of energy we gain for each attack ability
    FixedPoint base_energy_gain_per_attack = 10_fp;

    // Amount of effective damage combat unit required to receive
    // (after mitigation and shields) to gain 1 energy
    // Details: https://illuvium.atlassian.net/wiki/spaces/AB/pages/72321254/Energy+Calculation#Taking-Damage
    FixedPoint one_energy_gain_per_damage_taken = 30_fp;

    // The logger for our world
    std::shared_ptr<Logger> logger = nullptr;

    // Pointer to an array of obstacles
    std::shared_ptr<ObstaclesMapType> obstacles = nullptr;

    // Should the hyper system be enabled?
    bool enable_hyper_system = true;

    // Should we enable omega range
    bool enable_omega_range = true;

    // Fraction of grid size to use for pathfinding iterations
    // Lower is slower but allows more complex paths
    // Higher is faster but limits path complexity
    int pathfinding_iteration_divisor = 1;

    // How many time steps to tolerate not having a reachable focus
    int unreachable_focus_time_step_limit = 5;

    // Multiple of movement speed needed to switch focus
    int focus_switch_movement_multiplier = 1;

    // How frequency to scan alternate targets
    int distance_scan_frequency_time_steps = 3;

    // Config for the logs
    LogsConfig logs_config{};

    BattleConfig battle_config;
};

/* -------------------------------------------------------------------------------------------------------
 * World
 *
 * This class represents the entire game world within the scope of the ECS system.
 * --------------------------------------------------------------------------------------------------------
 */
class World final : public std::enable_shared_from_this<World>, public LoggerConsumer
{
    typedef World Self;

public:
    // Create a new world with the config and proper data
    static std::shared_ptr<World> Create(
        const WorldConfig& config,
        std::shared_ptr<const GameDataContainer> game_data_container);

    // Helper function to create a deep copy of this World of the initial state
    // NOTE: Can't be called if the battle started
    std::shared_ptr<World> CreateDeepCopyFromInitialState() const;

    // Copyable and NOT movable
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    // Destructor needed otherwise shared_ptr won't know how to destruct
    ~World() override;

    // Start profiling simulation
    void StartProfiling()
    {
        IlluviumStartProfiling();
    }

    // Stop profiling simulation
    void StopProfiling(const std::string& dump_file)
    {
        IlluviumStopProfiling(dump_file);
    }

    // TimeStep world instance
    void TimeStep();

    // Gets all entities
    const std::vector<std::shared_ptr<Entity>>& GetAll() const
    {
        return entities_;
    }

    // Helper function to safe walk over all the entities.
    inline void SafeWalkAll(const std::function<void(const Entity&)>& walk_function) const
    {
        // Store the size here to so that we don't iterate over an entity that was spawned while in the
        // loop
        const size_t size_before_loop = entities_.size();
        for (size_t i = 0; i < size_before_loop; i++)
        {
            // NOTE: We Don't make this into return by reference otherwise this might point to an
            // invalid reference inside the vector if it gets resized
            const std::shared_ptr<Entity> entity = entities_[i];
            walk_function(*entity);
        }
    }

    // Helper function to safe walk over all entities that passes filter lambda
    inline void SafeWalkAllWithFilter(
        const std::function<void(const Entity&)>& walk_function,
        const std::function<bool(const Entity&)>& filter) const
    {
        SafeWalkAll(
            [&](const Entity& entity)
            {
                if (!filter(entity))
                {
                    return;
                }
                walk_function(entity);
            });
    }

    // Helper function to safe walk over all the enemy combat units
    void SafeWalkAllEnemiesOfEntity(const Entity& enemies_of, const std::function<void(const Entity&)>& walk_function)
        const;

    // Gets entities except those specified
    std::vector<std::shared_ptr<Entity>> GetFilteredEntityList(
        const std::unordered_set<EntityID>& exclude_entities) const;

    // Adds a new entity to the world
    Entity& AddEntity(const Team team, const EntityID parent_id = kInvalidEntityID);

    // Does the Entity with the id exists?
    bool HasEntity(const EntityID id) const
    {
        return entity_id_to_index_map_.contains(id);
    }

    // Does the combat unit with this unique id exist?
    bool HasCombatUnitUniqueID(const std::string& unique_id) const
    {
        return unique_ids_map_.contains(unique_id);
    }

    // Helper method to check if a combat unit is alive
    bool IsCombatUnitAlive(const EntityID id) const;

    // Gets the EntityID of the combat unit who made this Combat Unit faint
    EntityID GetCombatUnitVanquisherID(const EntityID id) const;

    // Gets all the combat units who assisted on killing this EntityID
    const std::vector<EntityID>& GetCombatUnitsFaintAssists(const EntityID id) const;

    // Gets the time step in which this combat unite fainted at, if any
    int GetCombatUnitFaintedTimeStep(const EntityID id) const;

    //
    // Entity state and stats helper methods
    //

    // Helper method to find entity to focus on for focused effect
    std::shared_ptr<Entity> GetActiveFocusedEffectEntity(const Entity& entity) const;

    // Helper method to compute the base stats of this entity, if any.
    // Computes the Base Stats = Template + Random Stats + Augments + Weapons/Suits
    const StatsData& GetBaseStats(const EntityID id) const;
    const StatsData& GetBaseStats(const Entity& entity) const;

    // Helper method to compute the live stats of this entity, if any.
    // Computes the Current Stats = Base + Buffs/Debuffs
    // Base = Template + Random Stats
    // Current = Base + Buffs - Debuffs
    StatsData GetLiveStats(const EntityID id) const
    {
        if (!HasEntity(id))
        {
            return {};
        }

        return GetLiveStats(GetByID(id));
    }
    StatsData GetLiveStats(const Entity& entity) const;

    // Gets the FullStatsData for an entity
    FullStatsData GetFullStats(const EntityID id) const
    {
        return FullStatsData{GetBaseStats(id), GetLiveStats(id)};
    }
    FullStatsData GetFullStats(const Entity& entity) const
    {
        return FullStatsData{GetBaseStats(entity), GetLiveStats(entity)};
    }

    //
    // Helper methods to evaluate an expression
    //

    // Expression that uses only sender or sender and receiver is the same entity
    FixedPoint EvaluateExpression(const EffectExpression& expression, const EntityID sender_id) const
    {
        return expression.Evaluate(ExpressionEvaluationContext(this, sender_id, sender_id));
    }

    FixedPoint
    EvaluateExpression(const EffectExpression& expression, const EntityID sender_id, const EntityID receiver_id) const
    {
        return expression.Evaluate(ExpressionEvaluationContext(this, sender_id, receiver_id));
    }

    EntityDataForExpression GetEntityDataForExpression(const EntityID entity_id) const
    {
        return EntityDataForExpression{GetFullStats(entity_id), &GetAllSynergiesOfEntityID(entity_id)};
    }

    // Helper to get the total shield value for all shields on an entity
    FixedPoint GetShieldTotal(const Entity& entity) const;

    // Does the receiver_entity has all the conditions necessary to apply the data
    bool DoesPassConditions(const Entity& receiver_entity, const EnumSet<ConditionType>& required_conditions) const;

    //
    // Synergies
    //

    // Utility function to get all synergies for entity
    const std::vector<SynergyOrder>& GetAllSynergiesOfEntityID(const EntityID entity_id) const
    {
        const SynergiesStateContainer& synergies_state_container = GetSynergiesStateContainer();
        return synergies_state_container.GetTeamAllSynergies(GetEntityTeam(entity_id), IsBattleStarted());
    }

    const std::shared_ptr<SynergiesStateContainer>& GetSynergiesStateContainerPtr() const
    {
        assert(synergies_state_container_);
        return synergies_state_container_;
    }
    const SynergiesStateContainer& GetSynergiesStateContainer() const
    {
        assert(synergies_state_container_);
        return *synergies_state_container_;
    }

    const GameDataContainer& GetGameDataContainer() const;
    const std::shared_ptr<const GameDataContainer>& GetGameDataContainerPtr() const
    {
        return game_data_container_;
    }

    // Apply an augment to an entity
    bool AddAugmentBeforeBattleStarted(const Entity& entity, const AugmentInstanceData& instance) const;

    // Remove augment from an entity
    bool RemoveAugmentBeforeBattleStarted(const Entity& entity, const AugmentInstanceData& instance) const;

    // Apply consumable to an entity
    bool AddConsumableBeforeBattleStarted(const Entity& entity, const ConsumableInstanceData& instance) const;

    // Remove consumable from an entity
    bool RemoveConsumableBeforeBattleStarted(const Entity& entity, const ConsumableInstanceData& instance) const;

    bool AddDroneAugmentBeforeBattleStarted(
        const Team team,
        const DroneAugmentTypeID& drone_augment_type_id,
        std::string* out_error_message = nullptr);

    void RemoveAllDroneAugmentsBeforeBattleStarted();

    // Get Hyper config
    const HyperConfig& GetHyperConfig() const;

    //
    // Before Battle Started
    //

    // Reorder the entities vector to match this new unique_unique_ids_order vector
    void ReorderEntitiesToMatchBeforeBattleStarted(const std::vector<std::string>& unique_ids_order);

    // Change this Combat Unit position
    // NOTE: Only works before the battle has started
    void ChangeCombatUnitPositionBeforeBattleStarted(const EntityID id, const HexGridPosition& new_position);

    // Change this Combat Unit team
    // Returns true if it changed successfully
    // NOTE: Only works before the battle has started
    bool ChangeCombatUnitTeamBeforeBattleStarted(const EntityID id, const Team new_team);

    // Removes this Combat Unit from the World
    // NOTE: Only works before the battle has started
    bool RemoveCombatUnitBeforeBattleStarted(const EntityID id);

    // Checks all the bonds for all combat units before the battle started
    bool CheckAllBondsBeforeBattleStarted();
    bool CheckRangerBondBeforeBattleStarted(const EntityID ranger_entity_id);
    bool CheckRangerBondBeforeBattleStarted(const std::string& ranger_unique_id);
    bool CheckRangerBondBeforeBattleStarted(const Entity& ranger_entity);

    // When a combat unit is removed, it should also be removed from synergy combat classes & combat
    // affinities
    void RefreshCombatSynergiesBeforeBattleStarted();

    //
    // Other helper methods
    //

    const AttachedEffectsHelper& GetAttachedEffectsHelper() const
    {
        return attached_effects_helper_;
    }

    const EffectPackageHelper& GetEffectPackageHelper() const
    {
        return effect_package_helper_;
    }

    const TargetingHelper& GetTargetingHelper() const
    {
        return targeting_helper_;
    }

    const AugmentHelper& GetAugmentHelper() const
    {
        return augment_helper_;
    }

    const EquipmentHelper& GetEquipmentHelper() const
    {
        return equipment_helper_;
    }

    const ConsumableHelper& GetConsumableHelper() const
    {
        return consumable_helper_;
    }

    const GridHelper& GetGridHelper() const
    {
        return grid_helper_;
    }

    const SynergiesHelper& GetSynergiesHelper() const
    {
        return synergies_helper_;
    }

    LevelingHelper GetLevelingHelper() const
    {
        return {
            .growth_rate_percentage = GetLevelingConfig().leveling_grow_rate_percentage,
            .stat_scale = GetStatsConstantsScale(),
        };
    }

    // If BattleConfig::use_max_stage_placement_radius is true returns the maximum possible radius for this unit after
    // evolution. Otherwise returns the radius from combat unit data.
    // If this method fails to find the maximum radius it will print an error and return the radius from full_data
    int GetPreferredUnitRadius(const FullCombatUnitData& full_data) const;

    // Helper function ot updates all the data for this Combat Unit
    // NOTE: This does not modify the position or team, for those use:
    // - ChangeCombatUnitPositionBeforeBattleStarted
    // - ChangeCombatUnitTeamBeforeBattleStarted
    bool UpdateCombatUnitFromData(
        const EntityID id,
        const FullCombatUnitData& full_data,
        const bool refresh_synergies,
        std::string* out_error_message = nullptr);

    // Helper method to get the team of the id.
    // NOTE: This works even if the entity does not exist anymore
    Team GetEntityTeam(const EntityID id) const;

    // Gets the index in the vector of entities for this entity id
    // If the id does not exist then this returns an invalid index.
    size_t GetEntityIDIndex(const EntityID id) const;

    // Gets the Entity by the specified id
    // Should always check with HasEntity before calling this
    Entity& GetByID(const EntityID id) const
    {
        return *GetByIDPtr(id);
    }

    // Same as GetByID but this returns a shared pointer
    const std::shared_ptr<Entity>& GetByIDPtr(const EntityID id) const
    {
        assert(HasEntity(id));
        const size_t index = entity_id_to_index_map_.at(id);
        assert(index < entities_.size());
        return entities_.at(index);
    }

    // Gets the combat unit entity using the unique id
    const std::shared_ptr<Entity>& GetCombatUnitByUniqueIDPtr(const std::string& unique_id) const
    {
        return unique_ids_map_.at(unique_id);
    }

    // Walks up the tree of the id Entity and returns the combat unit that spawned this
    EntityID GetCombatUnitParentID(const EntityID id) const;
    EntityID GetCombatUnitParentID(const Entity& entity) const;

    // Figures out if this entity id has any child entities (aka id is the parent of some other
    // entities)
    bool HasAnyChildEntities(const EntityID id) const
    {
        return !GetChildEntities(id).empty();
    }

    // Gets all the child entities that have the parent id set to this
    std::unordered_set<EntityID> GetChildEntities(const EntityID id) const;

    // Get teams
    const std::vector<Team>& GetTeams() const
    {
        return teams_;
    }

    // Helper function to determine if all the entities are allies
    bool AreAllies(const std::vector<EntityID>& entities) const;
    template <typename... Args>
    bool AreAllies(const EntityID id, Args... args) const
    {
        const std::vector<EntityID> vector = {id, args...};
        return AreAllies(vector);
    }

    // Subscribes the listener to this specific event type when it occurs
    // Returns an event handler ID that can be used to unsubscribe
    EventHandleID SubscribeToEvent(const EventType event_type, const EventCallback& listener);

    // Subscribes the listener to this specific event data type when it occurs
    // Returns an event handler ID that can be used to unsubscribex
    // It creates a wrapper for passed method and call SubscribeToEvent overload above
    template <
        EventType event_type,
        typename EventDataType = event_impl::EventTypeToDataType<event_type>,
        // this 'Enable' does not allow user to pass function with "void(const Event&)" signature
        typename Enable = std::enable_if_t<!std::is_same_v<EventDataType, Event>>>
    EventHandleID SubscribeToEvent(const std::function<void(const EventDataType&)>& listener)
    {
        return SubscribeToEvent(
            event_type,
            [listener](const Event& event)
            {
                listener(event.Get<EventDataType>());
            });
    }

    // Helper function to subscribe the class method to specific event type
    // This will accept methods with const Event& parameter or event_data::EventX paramter
    // For example in this case
    //     world_->SubscribeMethodToEvent<EventType::kBeamActivated>(this, &Self::OnBeamActivated);
    // "Self::OnBeamActivated" can have either "void(const event_data::BeamActivated&)" or "void (const Event&)"
    // signature.
    template <EventType event_type, typename This, typename EventDataType = event_impl::EventTypeToDataType<event_type>>
    EventHandleID SubscribeMethodToEvent(This* object, void (This::*method)(const EventDataType&))
    {
        std::function<void(const EventDataType&)> callback = [object, method](const EventDataType& event)
        {
            (object->*method)(event);
        };
        if constexpr (std::is_same_v<EventDataType, Event>)
        {
            // If subscriber method has "void (const Event&)" signature we
            // don't have to wrap this callback in any way so we just pass it into default SubscribeToEvent.
            return SubscribeToEvent(event_type, callback);
        }
        else
        {
            // Otherwise (like EventData::Fainted) we pass it to template version of
            // SubscribeToEvent which makes a wrapper over that callback which extracts specific type
            return SubscribeToEvent<event_type>(callback);
        }
    }

    // Unsubscribe the listener from this event handle
    void UnsubscribeFromEvent(const EventHandleID& event_handle_id);

    // Emits the event, which in turn notifies all subscribers to that EventType
    void EmitEvent(const Event& event) const;

    // Helper method to emit specific event type.
    // Accepts corresponding event data object from event_data namespace.
    template <EventType event_type>
    void EmitEvent(const event_impl::EventTypeToDataType<event_type>& data)
    {
        Event event(event_type);
        event.Set<event_impl::EventTypeToDataType<event_type>>(data);
        EmitEvent(event);
    }

    // Helper function to build and emit an event
    template <EventType event_type, typename... TArgs>
    void BuildAndEmitEvent(TArgs&&... margs)
    {
        using EventDataType = event_impl::EventTypeToDataType<event_type>;
        const EventDataType data{{std::forward<TArgs>(margs)}...};
        EmitEvent<event_type>(data);
    }

    // Returns the random number in the range [min, max)
    int RandomRange(const int min, const int max)
    {
        return static_cast<int>(random_.Range(static_cast<uint64_t>(min), static_cast<uint64_t>(max)));
    }

    // Has the battle started?
    bool IsBattleStarted() const
    {
        return is_battle_started_;
    }

    // Has the battle finished?
    bool IsBattleFinished() const
    {
        return is_battle_finished_;
    }

    // Returns this world battle result
    const BattleWorldResult& GetBattleResult() const
    {
        return battle_result_;
    }

    // Returns current state of all combat units
    std::vector<BattleEntityResult> GetCombatUnitsState() const;

    // Returns a reference to specified system type
    template <typename T>
    T& GetSystem() const
    {
        const size_t index = GetSystemTypeId<T>();
        auto* system = system_array_[index];
        assert(system != nullptr);
        return *static_cast<T*>(system);
    }

    // The Scale factor for the world stats
    FixedPoint GetStatsConstantsScale() const
    {
        return config_.stats_constants_scale;
    }

    const WorldEffectsConfig& GetWorldEffectsConfig() const;

    // Amount of energy we gain for each attack ability
    FixedPoint GetBaseEnergyGainPerAttack() const
    {
        return config_.stats_constants_scale * config_.base_energy_gain_per_attack;
    }

    // Amount of effective damage combat unit required to receive
    // (after mitigation and shields) to gain 1 energy
    // Details: https://illuvium.atlassian.net/wiki/spaces/AB/pages/72321254/Energy+Calculation#Taking-Damage
    FixedPoint GetOneEneryGainPerEffectiveDamage() const
    {
        return config_.one_energy_gain_per_damage_taken;
    }

    // Return the last entity id created
    int GetLastAddedEntityID() const
    {
        return last_added_entity_id_;
    }

    // Return obstacle map to be reused
    ObstaclesMapType& GetObstacleMapRef() const
    {
        return *config_.obstacles;
    }

    const WorldConfig& GetWorldConfig() const
    {
        return config_;
    }

    const LogsConfig& GetLogsConfig() const
    {
        return config_.logs_config;
    }
    const BattleConfig& GetBattleConfig() const
    {
        return config_.battle_config;
    }

    bool IsOmegaRangeEnabled() const
    {
        return config_.enable_omega_range;
    }

    // Grid scale for internal representation of data
    // The scale is needed to avoid wasting resources in places where we iterate over sizes/spaces
    // This represents how many centimeters there are in one simulation grid unit
    constexpr int GetGridScale() const
    {
        return config_.battle_config.grid_scale;
    }

    // The number of rows from the center line of the arena.
    constexpr int GetMiddleLineWidth() const
    {
        return config_.battle_config.middle_line_width;
    }

    const AugmentsConfig& GetAugmentsConfig() const
    {
        return config_.battle_config.augments_config;
    }
    const ConsumablesConfig& GetConsumablesConfig() const
    {
        return config_.battle_config.consumables_config;
    }
    const OverloadConfig& GetOverloadConfig() const
    {
        return config_.battle_config.overload_config;
    }
    const LevelingConfig& GetLevelingConfig() const
    {
        return config_.battle_config.leveling_config;
    }

    // Get the multiple of movement speed needed to switch focus
    constexpr int GetFocusSwitchMovementMultiplier() const
    {
        return config_.focus_switch_movement_multiplier;
    }

    // Get the frequency of scanning alternate targets
    constexpr int GetDistanceScanFrequencyTimeSteps() const
    {
        return config_.distance_scan_frequency_time_steps;
    }

    // Fraction of grid size to use for pathfinding iterations
    // Lower is slower but allows more complex paths
    // Higher is faster but limits path complexity
    constexpr int GetPathfindingIterationDivisor() const
    {
        return config_.pathfinding_iteration_divisor;
    }

    // How many time steps to tolerate not having a reachable focus
    constexpr int GetUnreachableFocusTimeStepLimit() const
    {
        return config_.unreachable_focus_time_step_limit;
    }

    // Limit of pathfinding search iterations
    constexpr int GetPathfindingIterationLimit() const
    {
        return pathfinding_iteration_limit_;
    }

    // Maximum entity radius
    constexpr int GetMaxEntityRadius() const
    {
        return max_entity_radius_;
    }

    // Infinite range (twice longest side of the grid here, as we do not have a concept of infinity
    // in the range) Super large value could cause an overflow in the math logic, so use maximum
    // possible distance apart instead
    constexpr FixedPoint GetRangeInfinite() const
    {
        return FixedPoint::FromInt(range_infinite_);
    }

    // Gets a world position, in world scale, of the grid position
    constexpr IVector2D ToWorldPosition(const HexGridPosition& grid_position) const
    {
        return GridHelper::ToScaled2D(grid_position) * GetGridScale() / kPrecisionFactor;
    }

    // Helper function to convert a value from axial world space to world position.
    // This is the same as doing HexGridPosition{value, 0}.ToWorldPosition().x
    constexpr int ToWorldPosition(const int value) const
    {
        return (GetGridScale() * (kSqrt3Scaled * value)) / kPrecisionFactor;
    }

    constexpr bool CanApplyOverloadDamage() const
    {
        return overload_apply_damage_;
    }

    constexpr FixedPoint GetOverloadDamagePercentage() const
    {
        return overload_damage_percentage_;
    }

    constexpr FixedPoint GetDeltaOverloadDamagePercentage() const
    {
        return config_.battle_config.overload_config.increase_overload_damage_percentage;
    }

    // Creates a HexGridPosition from IVector2D object in world scale
    constexpr HexGridPosition FromWorldPosition(const IVector2D& world_position) const
    {
        // References:
        // - https://www.redblobgames.com/grids/hexagons/#pixel-to-hex

        const int x = world_position.x;
        const int y = world_position.y;

        // Calculate hex position
        // Relationship between XY and QR is represented by matrix [sqrt(3), 0, sqrt(3)/2, 3/2]
        // which can be derived from the basis vectors
        const int q = (kSqrt3Scaled / 3 * x - kPrecisionFactor / 3 * y) / GetGridScale();
        const int r = ((2 * kPrecisionFactor) / 3 * y) / GetGridScale();

        // Return rounded result
        HexGridPosition result_units;
        HexGridPosition result_sub_units;
        HexGridPosition::Round(HexGridPosition{q, r}, &result_units, &result_sub_units);
        return result_units;
    }

    //
    // LoggerConsumer interface
    //

    void SetLogger(std::shared_ptr<Logger> logger)
    {
        config_.logger = std::move(logger);
    }

    // Returns the logger the world uses
    std::shared_ptr<Logger> GetLogger() const override
    {
        return config_.logger;
    }

    // Builds a nice log prefix for the specified entity
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;

    // Gets the current time step count
    int GetTimeStepCount() const override
    {
        return time_step_counter_;
    }

    void ClearEntityBaseStatCache(const EntityID entity_id) const
    {
        entities_base_stats_map_.erase(entity_id);
    }

    constexpr FixedPoint GetMaxAttackSpeed() const
    {
        return config_.max_attack_speed;
    }

    constexpr FixedPoint GetClampedAttackSpeed(const FixedPoint attack_speed) const
    {
        return (std::min)(attack_speed, GetMaxAttackSpeed());
    }

    // Add global collision for a team
    void AddGlobalCollision(const Team team, const int global_collision_id, const EntityID collided_with);

    // Checks if a team has collision with entity for given global collision id
    bool HasGlobalCollision(const Team team, const int global_collision_id, const EntityID collided_with) const;

    // Clears all collisions for a team and given global collision id
    void ResetGlobalCollision(const Team team, const int global_collision_id);

    // Helper method which applies encounter mods to specified combat units.
    // Used by entity factory to apply encounter mods to combat units when battle already started.
    void ApplyEncounterModsToCombatUnit(const Entity& entity) const;

    const HexGridConfig& GetGridConfig() const
    {
        return hex_grid_config_;
    }

    // Declares relative tick order between two entities
    void AddTickDependency(const EntityID ticks_first, const EntityID ticks_second)
    {
        entities_tick_dependents_[ticks_first].push_back(ticks_second);
        has_to_reorder_entities_ = true;
    }

    const StatsData& GetPreviousLiveStats(const EntityID id) const
    {
        auto it = entities_previous_live_stats_map_.find(id);
        if (it != entities_previous_live_stats_map_.end())
        {
            return it->second;
        }

        return empty_default_stats_;
    }

private:
    // Only Create() can make a new world
    World();

    // Only private copyable
    World(const World&) = default;

    // Sorts entities vector by unique id. Called on the first time step (before battle start).
    // Order (ascending or descending) depends on battle_config.random_seed
    void SortEntitiesByUniqueID();

    void ApplyEncounterModsBeforeBattleStarted();

    void EraseEntitiesMarkedForDeletion();

    // Sorts entities in topological order to prepare tick list
    // https://en.wikipedia.org/wiki/Topological_sorting
    void SortEntitiesTopologically();

    // Private setters
    void SetSynergiesDataContainer(std::shared_ptr<const GameDataContainer> game_data)
    {
        assert(config_.logger);
        synergies_state_container_ = SynergiesStateContainer::Create(config_.logger, std::move(game_data));
    }

    // Check each time step for the overload damage
    bool TimeStepCheckForOverloadDamage();

    // Called at the end of TimeStep()
    void PostTimeStep();

    // Returns the system type ID of specific type
    template <typename T>
    static size_t GetSystemTypeId() noexcept
    {
        static size_t type_id = GetSystemTypeId();
        assert(type_id < kMaxSystems);
        return type_id;
    }

    // Returns a new generic system ID
    // TODO(vampy): Find a better way to allocate ids for the system and components
    // Alternatives:
    // - compile time counter - kinda complicated and ugly
    // - using std::type_index but that requires the man to be walkable in a deterministic order
    static size_t GetSystemTypeId() noexcept
    {
        static size_t last_id = 0;
        const size_t id = last_id;
        last_id++;
        return id;
    }

    // Add a new system to the world
    // NOTE: This is private because we want the order of the systems to be deterministic
    // A different order of the systems will have different results of the simulation
    template <typename T, typename... TArgs>
    T& AddSystem(TArgs&&... margs)
    {
        // Create the system
        auto system = std::make_shared<T>(std::forward<TArgs>(margs)...);
        auto* system_ptr = system.get();

        // Store the system
        system_array_[GetSystemTypeId<T>()] = system_ptr;
        systems_.push_back(std::move(system));

        // Initialise the system
        system_ptr->Init(this);

        // Return the system
        return *system_ptr;
    }

    // Some kinds of entities might not have stats component but still able to have attached effects
    // This methods looks for an entity that serves as stat source for them.
    const Entity* GetStatSourceEntity(const Entity& entity) const;
    const Entity* GetStatSourceEntity(const EntityID entity_id) const
    {
        const auto& entity_ptr = GetByIDPtr(entity_id);
        if (entity_ptr)
        {
            return GetStatSourceEntity(*entity_ptr);
        }

        return nullptr;
    }

    // Private method that does the calculations for GetLiveStats
    StatsData CalculateLiveStats(const Entity& receiver_entity, const AttachedEffectBuffsState& buffs_state) const;

    // Helper to add the sum of total
    FixedPoint AddValueToTotalsForBuffsDebuffs(
        const AttachedEffectState& state,
        StatsData* out_totals,
        StatsData* out_percentages_totals) const;

    // Helper method to calculate the bufffs and debuffs from attaches entities
    void CalculateBuffsFromAttachedEntities(
        const Entity& receiver_entity,
        StatsData* out_buffs_total,
        StatsData* out_buffs_percentages_total) const;

    // Handles the special case of destroying a projectile or zone
    // Returns true if it can, false otherwise
    bool DestroySpawnedEntity(const EntityID id);

    // Check whether a deferred destruction is pending
    bool IsPendingDestruction(const Entity& entity) const;

    // Add to the world all the systems it needs
    void InternalAddSystems();

    // Subscribe the world to every event it needs
    void InternalSubscribeToEvents();

    // Checks if the game is finished
    // Aka one owner had all its
    void CheckForFinishedGame();

    // Check if overload damage will faint all combat untis and who will be the winner
    void CheckForOverloadTieBreaker();

    // Helper overload function to destroy a bunch of combat units
    void OverloadDestroyCombatUnits(
        const std::vector<EntityID>& combat_units_to_destroy,
        const EntityID ignored_entity);

    // Handle world events here
    void OnFainted(const event_data::Fainted& fainted_data);
    void OnBattleFinished(const event_data::BattleFinished& event_data);
    void OnMarkSpawnedEntityAsDestroyed(const event_data::Marked& data);
    void OnEffectApplied(const event_data::Effect& event_data);

    // Erase an entity from entities vector and entities index map
    void EraseEntity(const EntityID id);

    // Readjust fast entity_id_to_index_map_ because some of the values are now incorrect.
    // Should be called after EraseEntity
    void UpdateEntityIDToIndexMap();

    // Helper function for CheckRangerBondBeforeBattleStarted to get the bonded entity by the unique id
    std::shared_ptr<Entity> GetBondedEntity(const Entity& ranger_entity, const std::string& bonded_id) const;

    // Map of stat modifications that must be applied for each spawning combat unit
    // Key: stat
    // Value: stat value to be added to unit base stat
    using StatModifiersMap = std::unordered_map<StatType, FixedPoint>;

    // Collects the stat modifiers for the team and the specified unique_id (if any).
    StatModifiersMap CollectStatModifiers(const Team team, const std::string& unique_id) const;

    // Maximum number of system types in the game
    static constexpr size_t kMaxSystems = 32;

    // Systems array used for fast iteration
    std::vector<std::shared_ptr<System>> systems_{};

    // System array used for fast lookup System::GetSystemTypeId<T>()
    std::array<System*, kMaxSystems> system_array_{};

    // Keep track of all entities
    std::vector<std::shared_ptr<Entity>> entities_{};

    // Keep track of teams
    std::vector<Team> teams_{};

    // The synergies runtime data
    std::shared_ptr<SynergiesStateContainer> synergies_state_container_;

    AttachedEffectsHelper attached_effects_helper_;
    EffectPackageHelper effect_package_helper_;
    TargetingHelper targeting_helper_;
    AugmentHelper augment_helper_;
    EquipmentHelper equipment_helper_;
    SynergiesHelper synergies_helper_;
    ConsumableHelper consumable_helper_;
    GridHelper grid_helper_;

    DroneAugmentsState drone_augments_state_;

    // Immutable game data
    std::shared_ptr<const GameDataContainer> game_data_container_;

    // Listeners array for fast lookup for each event type
    // Key: EventType converted to int
    // Value: Vector of EventCallbacks which is just a function callback that accepts an Event
    std::array<std::vector<EventCallbackPtr>, Event::kMaxEvents> events_subscribers_{};

    // We need this because some entities might get removed from the entities_ vector (like
    // projectiles).
    // Key: the id of an entity.
    // Value: the index inside of the entities_ vector.
    std::unordered_map<EntityID, size_t> entity_id_to_index_map_{};

    // Map to keep track of all the unique ids of all the combat units
    // Key: The unique id of the combat unit (note, always not empty)
    // Value: combat unit entity
    std::unordered_map<std::string, std::shared_ptr<Entity>> unique_ids_map_;

    // Keep track of the last entity id so when we add a new entity we assign this + 1.
    // A entity once created is guaranteed to be unique for the lifetime of this world.
    EntityID last_added_entity_id_ = kInvalidEntityID;

    // A list of vectors to delete in the right before the next TimeStep.
    // We need this as we can't remove the entity while we are in the current TimeStep.
    std::unordered_set<EntityID> entities_to_delete_;

    // The deterministic random number generator
    RandomGenerator random_;

    // The current time step counter of this world
    int time_step_counter_ = 0;

    // Has the battle started?
    bool is_battle_started_ = false;

    // Has the battle finished?
    bool is_battle_finished_ = false;

    // The battle result is set only when is_battle_finished_ = true;
    BattleWorldResult battle_result_;

    // World configurable params
    WorldConfig config_{};

    // This will increase every second and will check if overload can apply damage
    int overload_current_seconds_ = 0;

    // This will increase every second currently with 10%
    FixedPoint overload_damage_percentage_ = 0_fp;

    // In case the battle is not finished, allow Overload System to apply pure damage
    bool overload_apply_damage_ = false;

    // Infinite range (twice longest side of the grid here, as we do not have a concept of infinity
    // in the range) Super large value could cause an overflow in the math logic, so use maximum
    // possible distance apart instead
    int range_infinite_ = 0;

    // Information about hex grid
    HexGridConfig hex_grid_config_{};

    // Maximum entity size
    int max_entity_radius_ = 0;

    // Limit of pathfinding search iterations
    int pathfinding_iteration_limit_ = 0;

    // Helper struct used to keep track the data for the destroyed spawned entities
    struct CachedDataDestroyedSpawnedEntity
    {
        // Last known base stats
        StatsData base_stats{};

        // Last known team of the entity
        Team team = Team::kNone;

        // Last known parent id
        EntityID parent_id = kInvalidEntityID;
    };

    // Keep track of all the data for the destroyed entities.
    // Key: id of the destroyed spawned entity
    // Value: Last known data for the destroyed spawned entity
    std::unordered_map<EntityID, CachedDataDestroyedSpawnedEntity> data_destroyed_entities_map_;

    // Keep track of all the base stats.
    // As the base stats do not change after battle start, we can cache this, and always return a const StatsData&
    // Key: id of the entity
    // Value: The cached base stats
    mutable std::unordered_map<EntityID, StatsData> entities_base_stats_map_;

    // Keep track of all the previous time step live stats of all the entities
    // NOTE: This should only be used by the buffs/debuffs calculation
    // Key: id of the entity
    // Value: The cached live stats
    std::unordered_map<EntityID, StatsData> entities_previous_live_stats_map_;

    // Used in some methods to return a const&
    static constexpr StatsData empty_default_stats_{};

    // Keep track at what time step each combat unit fainted at
    // Key: id of the entity who has fainted
    // Value: time step in which that unit fainted
    std::unordered_map<EntityID, int> entities_fainted_history_map_{};

    // Keep track of the history of all the vanquishers
    // Key: id of the combat unit who has fainted
    // Value: id of the combat unit vanquisher (who made the key id faint)
    std::unordered_map<EntityID, EntityID> vanquisher_history_map_{};

    // Keep track of the history of all detrimental effects assists
    // Key: id of the combat unit receiver of the detrimental effects
    // Value: a vector of combat unit entities who are the senders of the detrimental effect
    std::unordered_map<EntityID, std::vector<EntityID>> detrimental_effects_history_map_{};

    // Keep track of global collisions per team and per global collision id
    // Key: team
    // Value: a unordered_map with a key is global collision id and values is a set of collided entities
    EnumMap<Team, std::unordered_map<int, std::unordered_set<EntityID>>> global_collisions_{};

    // Entity Tick Graph
    // Key: entity id
    // Value: the list of entities that must tick after key entity
    std::unordered_map<EntityID, std::vector<EntityID>> entities_tick_dependents_;

    bool has_to_reorder_entities_ = false;
};

}  // namespace simulation
