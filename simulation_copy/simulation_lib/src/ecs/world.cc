#include "ecs/world.h"

#include <algorithm>
#include <functional>

#include "components/attached_entity_component.h"
#include "components/augment_component.h"
#include "components/aura_component.h"
#include "components/beam_component.h"
#include "components/chain_component.h"
#include "components/combat_synergy_component.h"
#include "components/combat_unit_component.h"
#include "components/consumable_component.h"
#include "components/dash_component.h"
#include "components/deferred_destruction_component.h"
#include "components/drone_augment_component.h"
#include "components/focus_component.h"
#include "components/mark_component.h"
#include "components/movement_component.h"
#include "components/projectile_component.h"
#include "components/shield_component.h"
#include "components/splash_component.h"
#include "components/stats_component.h"
#include "components/zone_component.h"
#include "data/battle_result.h"
#include "data/combat_unit_data.h"
#include "data/containers/game_data_container.h"
#include "ecs/event_types_data.h"
#include "ecs/system.h"
#include "factories/entity_factory.h"
#include "systems/ability_system.h"
#include "systems/attached_effects_system.h"
#include "systems/attached_entity_system.h"
#include "systems/augment_system.h"
#include "systems/aura_system.h"
#include "systems/beam_system.h"
#include "systems/chain_system.h"
#include "systems/consumable_system.h"
#include "systems/dash_system.h"
#include "systems/decision_system.h"
#include "systems/destruction_system.h"
#include "systems/displacement_system.h"
#include "systems/effect_system.h"
#include "systems/energy_gain_system.h"
#include "systems/focus_system.h"
#include "systems/health_gain_system.h"
#include "systems/hyper_system.h"
#include "systems/movement_system.h"
#include "systems/overload_system.h"
#include "systems/projectile_system.h"
#include "systems/splash_system.h"
#include "systems/state_system.h"
#include "systems/synergy_system.h"
#include "systems/zone_system.h"
#include "utility/augment_helper.h"
#include "utility/entity_helper.h"
#include "utility/equipment_helper.h"
#include "utility/evolution_helper.h"
#include "utility/grid_helper.h"
#include "utility/leveling_helper.h"
#include "utility/stats_helper.h"
#include "utility/string.h"
#include "utility/vector_helper.h"

namespace simulation
{
World::World() {}
World::~World() {}

std::shared_ptr<World> World::Create(
    const WorldConfig& config,
    std::shared_ptr<const GameDataContainer> game_data_container)
{
    // NOTE: can't access private constructor with make_shared
    auto world = std::shared_ptr<World>(new World);

    // Set the config
    world->config_ = config;

    // Initialize logger
    if (!world->config_.logger)
    {
        // Create default one
        world->SetLogger(Logger::Create());
    }

    // Check the height and width of the grid (Must be odd numbers)
    if (world->config_.battle_config.grid_height % 2 == 0 || world->config_.battle_config.grid_width % 2 == 0)
    {
        world->LogErr(
            "World::Create - invalid world size - {} x {} . Must be odd numbers",
            world->config_.battle_config.grid_height,
            world->config_.battle_config.grid_width);
        return nullptr;
    }

    // Set the data containers
    world->game_data_container_ = std::move(game_data_container);
    world->SetSynergiesDataContainer(world->game_data_container_);

    world->attached_effects_helper_ = AttachedEffectsHelper{world.get()};
    world->effect_package_helper_ = EffectPackageHelper{world.get()};
    world->targeting_helper_ = TargetingHelper{world.get()};
    world->augment_helper_ = AugmentHelper{world.get()};
    world->equipment_helper_ = EquipmentHelper{world.get()};
    world->synergies_helper_ = SynergiesHelper{world.get()};
    world->consumable_helper_ = ConsumableHelper{world.get()};
    world->hex_grid_config_ = HexGridConfig(config.battle_config.grid_width, config.battle_config.grid_height);
    world->grid_helper_ = GridHelper{world.get()};
    world->drone_augments_state_ = DroneAugmentsState{world.get()};

    // Maximum entity size
    world->max_entity_radius_ =
        std::min(world->GetGridConfig().GetGridHeight() / 2, world->GetGridConfig().GetGridWidth()) - 1;

    // Infinite range (twice longest side of the grid here, as we do not have a concept of infinity
    // in the range) Super large value could cause an overflow in the math logic, so use maximum
    // possible distance apart instead
    world->range_infinite_ =
        std::max(world->GetGridConfig().GetGridHeight(), world->GetGridConfig().GetGridWidth()) * 2;

    const size_t grid_size = world->GetGridConfig().GetGridSize();

    // Limit of pathfinding search iterations
    world->pathfinding_iteration_limit_ = static_cast<int>(grid_size) / world->GetPathfindingIterationDivisor();

    // Create new set of obstacles if needed
    if (!world->config_.obstacles)
    {
        world->config_.obstacles = std::make_shared<ObstaclesMapType>(grid_size);
    }
    else if (world->config_.obstacles->size() != grid_size)
    {
        world->config_.obstacles->resize(grid_size);
    }

    // Initialize variables
    world->random_.Init(world->config_.battle_config.random_seed);

    if (!world->GetSynergiesHelper().CheckSynergiesInAugments())
    {
        world->LogErr("World::Create - CheckSynergiesInAugments detected some errors. Can't create world.");
        return nullptr;
    }

    // Subscribe to events
    world->InternalSubscribeToEvents();

    // Add systems
    world->InternalAddSystems();

    world->LogDebug("Created world with configuration:");
    world->LogDebug("    Battle Configuration:");
    world->LogDebug("        grid_width: {}", world->config_.battle_config.grid_width);
    world->LogDebug("        grid_height: {}", world->config_.battle_config.grid_height);
    world->LogDebug("        grid_scale: {}", world->config_.battle_config.grid_scale);
    world->LogDebug("        random_seed: {}", world->config_.battle_config.random_seed);
    world->LogDebug("        middle_line_width: {}", world->config_.battle_config.middle_line_width);
    world->LogDebug("        sort_by_unique_id: {}", world->config_.battle_config.sort_by_unique_id);
    world->LogDebug("        max_weapon_amplifiers: {}", world->config_.battle_config.max_weapon_amplifiers);

    world->LogDebug("        Augments configuration:");
    world->LogDebug("            max_augments: {}", world->config_.battle_config.augments_config.max_augments);
    world->LogDebug(
        "            max_drone_augments: {}",
        world->config_.battle_config.augments_config.max_drone_augments);

    world->LogDebug("        Consumables configuration:");
    world->LogDebug("            max_consumables: {}", world->config_.battle_config.consumables_config.max_consumables);

    world->LogDebug("        Overload configuration:");
    world->LogDebug(
        "            enable_overload_system: {}",
        world->config_.battle_config.overload_config.enable_overload_system);
    world->LogDebug(
        "            start_seconds_apply_overload_damage: {}",
        world->config_.battle_config.overload_config.start_seconds_apply_overload_damage);
    world->LogDebug(
        "            increase_overload_damage_percentage: {}",
        world->config_.battle_config.overload_config.increase_overload_damage_percentage);

    world->LogDebug("        Leveling configuration:");
    world->LogDebug(
        "            enable_leveling_system: {}",
        world->config_.battle_config.leveling_config.enable_leveling_system);
    world->LogDebug(
        "            leveling_grow_rate_percentage: {}",
        world->config_.battle_config.leveling_config.leveling_grow_rate_percentage);

    return world;
}

std::shared_ptr<World> World::CreateDeepCopyFromInitialState() const
{
    // NOTE: We have this limitation because we can not gurantee we are copying everything
    // See: AbilitiesComponent and AttachedEffectsComponent
    if (IsBattleStarted())
    {
        LogErr("World::CreateDeepCopy - NOT ALLOWED (game already started). IGNORING.");
        return nullptr;
    }

    // NOTE: can't access private constructor with make_shared
    auto new_world = std::shared_ptr<World>(new World{*this});

    // The systems don't store any pointers so we can just resubscribe to these new systems by just adding them
    new_world->attached_effects_helper_ = AttachedEffectsHelper{new_world.get()};
    new_world->effect_package_helper_ = EffectPackageHelper{new_world.get()};
    new_world->targeting_helper_ = TargetingHelper{new_world.get()};
    new_world->augment_helper_ = AugmentHelper{new_world.get()};
    new_world->equipment_helper_ = EquipmentHelper{new_world.get()};
    new_world->consumable_helper_ = ConsumableHelper{new_world.get()};
    new_world->grid_helper_ = GridHelper{new_world.get()};
    new_world->synergies_helper_ = SynergiesHelper{new_world.get()};
    new_world->battle_result_ = {};
    new_world->unique_ids_map_.clear();

    new_world->drone_augments_state_ = drone_augments_state_;
    new_world->drone_augments_state_.ChangeWorld(new_world.get());

    new_world->SetSynergiesDataContainer(game_data_container_);

    new_world->InternalSubscribeToEvents();
    new_world->InternalAddSystems();

    // Deep Copy the entities
    new_world->entities_.clear();
    for (const auto& entity : entities_)
    {
        new_world->entities_.push_back(entity->CreateDeepCopyFromInitialState(new_world->shared_from_this()));
    }
    for (const auto& entity : new_world->GetAll())
    {
        if (entity->Has<CombatUnitComponent>())
        {
            new_world->unique_ids_map_[entity->Get<CombatUnitComponent>().GetUniqueID()] = entity;
        }
    }
    new_world->UpdateEntityIDToIndexMap();

    return new_world;
}

void World::TimeStep()
{
    // Game has started event
    if (!is_battle_started_)
    {
        if (config_.battle_config.sort_by_unique_id)
        {
            SortEntitiesByUniqueID();
        }

        SortEntitiesTopologically();
        CheckAllBondsBeforeBattleStarted();
        ApplyEncounterModsBeforeBattleStarted();

        is_battle_started_ = true;

        // Clear some caches so that they are reconstructed
        entities_base_stats_map_.clear();

        SafeWalkAll(
            [&](const Entity& entity)
            {
                // NOTE: Don't init the metered stats here otherwise a lot of tests will fail
                EntityHelper::InitRandomOverflowStats(*this, entity);

                // Init the live stats
                entities_previous_live_stats_map_[entity.GetID()] = GetLiveStats(entity.GetID());
            });

        // Run PreBattleStarted for all the systems
        SafeWalkAll(
            [&](const Entity& entity)
            {
                // InitialTimeStep entity for every system
                for (const auto& system : systems_)
                {
                    system->PreBattleStarted(entity);
                }
            });

        // Emit events
        BuildAndEmitEvent<EventType::kBattleStarted>();

        // Emit initial TimeStepped event as battle starts
        BuildAndEmitEvent<EventType::kTimeStepped>(time_step_counter_);

        // Run InitialTimeStep for all the systems
        SafeWalkAll(
            [&](const Entity& entity)
            {
                // InitialTimeStep entity for every system
                for (const auto& system : systems_)
                {
                    system->InitialTimeStep(entity);
                }
            });
    }
    if (is_battle_finished_)
    {
        LogWarn("World::TimeStep - Battle is already Finished");
    }

    // Advance world counter
    time_step_counter_++;

    // Fire overload?
    if (config_.battle_config.overload_config.enable_overload_system)
    {
        // Reset if whenever or not we apply actual damage
        overload_apply_damage_ = false;
        if (TimeStepCheckForOverloadDamage())
        {
            // Battle Ended
            return;
        }
    }

    EraseEntitiesMarkedForDeletion();
    SortEntitiesTopologically();

    // Time step all the systems
    SafeWalkAll(
        [&](const Entity& entity)
        {
            // LogDebug(entity->GetID(), "World::TimeStep - for loop [entity_index = {}]",  entity_index);

            // TimeStep entity for every system
            for (const auto& system : systems_)
            {
                system->TimeStep(entity);
            }
        });

    // Post Time step all the systems
    PostTimeStep();

    // Emit TimeStepped event after all processing finished this time step
    BuildAndEmitEvent<EventType::kTimeStepped>(time_step_counter_);
}

void World::PostTimeStep()
{
    SafeWalkAll(
        [&](const Entity& entity)
        {
            // PostTimeStep entity for every system
            for (const auto& system : systems_)
            {
                system->PostTimeStep(entity);
            }

            // Update the cache
            entities_previous_live_stats_map_[entity.GetID()] = GetLiveStats(entity.GetID());
        });

    // PostTimeStep for every system
    for (const auto& system : systems_)
    {
        system->PostSystemTimeStep();
    }
}

bool World::TimeStepCheckForOverloadDamage()
{
    const int seconds = Time::TimeStepsToSeconds(time_step_counter_);
    if (seconds < config_.battle_config.overload_config.start_seconds_apply_overload_damage + overload_current_seconds_)
    {
        return false;
    }

    // Emit event
    LogDebug(
        "World::TimeStepCheckForOverloadDamage - overload_current_seconds_ = {}, overload_damage_percentage_ = {}",
        overload_current_seconds_,
        overload_damage_percentage_);
    BuildAndEmitEvent<EventType::kOverloadStarted>(overload_current_seconds_);

    // This should be called before overload system.
    // In order to make sure we verify the combat units before they are fainted within the time step
    CheckForOverloadTieBreaker();
    if (is_battle_finished_)
    {
        return true;
    }

    // Battle not finished yet, increase seconds and allow Overload System to do actual damage
    overload_apply_damage_ = true;
    overload_current_seconds_ += 1;
    overload_damage_percentage_ += GetDeltaOverloadDamagePercentage();

    return false;
}

void World::SafeWalkAllEnemiesOfEntity(
    const Entity& enemies_of,
    const std::function<void(const Entity&)>& walk_function) const
{
    const auto filter_enemies_of_combat_unit = [&enemies_of](const Entity& entity)
    {
        // Check if entity is no longer active
        if (!entity.IsActive())
        {
            return false;
        }

        // Check if combat unit
        if (!EntityHelper::IsACombatUnit(entity))
        {
            return false;
        }

        // Only enemies should pass filter
        return !entity.IsAlliedWith(enemies_of);
    };

    SafeWalkAllWithFilter(walk_function, filter_enemies_of_combat_unit);
}

std::vector<std::shared_ptr<Entity>> World::GetFilteredEntityList(
    const std::unordered_set<EntityID>& exclude_entities) const
{
    std::vector<std::shared_ptr<Entity>> result;

    for (const auto& entity : GetAll())
    {
        // Skip excluded entities
        if (exclude_entities.contains(entity->GetID()))
        {
            continue;
        }

        // Add the rest
        result.push_back(entity);
    }

    return result;
}

Entity& World::AddEntity(const Team team, const EntityID parent_id)
{
    // Create a new entity (with a unique id) and add it to the list
    // NOTE: We can't use the entities_.size() as the id for our entity because some entities might
    // get removed
    const EntityID entity_id = last_added_entity_id_ + 1;
    auto entity = Entity::Create(shared_from_this(), team, entity_id, parent_id);
    auto* entity_ptr = entity.get();

    // Add to vector
    const size_t index = entities_.size();
    entities_.push_back(std::move(entity));

    // Keep track of the index
    entity_id_to_index_map_[entity_id] = index;

    // Mark this id as used
    last_added_entity_id_ = entity_id;

    // Track teams
    if (!VectorHelper::ContainsValue(teams_, team))
    {
        teams_.emplace_back(team);
    }

    // Send event
    LogDebug(entity_id, "World::AddEntity - new entities size = {}", entities_.size());
    BuildAndEmitEvent<EventType::kCreated>(entity_id);

    // Returns pointer to the entity
    return *entity_ptr;
}

bool World::IsCombatUnitAlive(const EntityID id) const
{
    if (!EntityHelper::IsACombatUnit(*this, id))
    {
        return false;
    }

    return GetByID(id).IsActive();
}

EntityID World::GetCombatUnitVanquisherID(const EntityID id) const
{
    if (!EntityHelper::IsACombatUnit(*this, id))
    {
        return kInvalidEntityID;
    }

    // No history
    if (!vanquisher_history_map_.contains(id))
    {
        return kInvalidEntityID;
    }

    return vanquisher_history_map_.at(id);
}

const std::vector<EntityID>& World::GetCombatUnitsFaintAssists(const EntityID id) const
{
    // Empty vector to be able to always return by const&
    static std::vector<EntityID> empty_vector;

    // No History, should be impossible?
    if (!detrimental_effects_history_map_.contains(id))
    {
        return empty_vector;
    }

    return detrimental_effects_history_map_.at(id);
}

int World::GetCombatUnitFaintedTimeStep(const EntityID id) const
{
    if (!EntityHelper::IsACombatUnit(*this, id))
    {
        return 0;
    }
    if (!entities_fainted_history_map_.contains(id))
    {
        return 0;
    }

    return entities_fainted_history_map_.at(id);
}

bool World::AddAugmentBeforeBattleStarted(const Entity& entity, const AugmentInstanceData& instance) const
{
    return augment_helper_.AddAugment(entity, instance);
}

bool World::RemoveAugmentBeforeBattleStarted(const Entity& entity, const AugmentInstanceData& instance) const
{
    return augment_helper_.RemoveAugment(entity, instance);
}

bool World::AddConsumableBeforeBattleStarted(const Entity& entity, const ConsumableInstanceData& instance) const
{
    return consumable_helper_.AddConsumable(entity, instance);
}

bool World::RemoveConsumableBeforeBattleStarted(const Entity& entity, const ConsumableInstanceData& instance) const
{
    return consumable_helper_.RemoveConsumable(entity, instance);
}

bool World::AddDroneAugmentBeforeBattleStarted(
    const Team team,
    const DroneAugmentTypeID& drone_augment_type_id,
    std::string* out_error_message)
{
    static constexpr std::string_view method_name = "World::AddDroneAugmentBeforeBattleStarted";
    if (IsBattleStarted())
    {
        const std::string error_message =
            fmt::format("{} - NOT ALLOWED (game already started). IGNORING.", method_name);
        if (out_error_message)
        {
            *out_error_message = error_message;
        }

        LogErr("{}", error_message);
        return false;
    }

    if (!GetGameDataContainer().HasDroneAugmentData(drone_augment_type_id))
    {
        const std::string error_message = fmt::format(
            "{} - Drone Augment data for id = {}, DOES NOT EXIST",
            method_name,
            drone_augment_type_id,
            team);
        if (out_error_message)
        {
            *out_error_message = error_message;
        }

        LogErr("{}", error_message);
        return false;
    }

    std::shared_ptr<const DroneAugmentData> drone_augment_data_ptr =
        GetGameDataContainer().GetDroneAugmentData(drone_augment_type_id);
    if (!drone_augments_state_.AddDroneAugment(team, *drone_augment_data_ptr))
    {
        const std::string error_message = fmt::format(
            "{} - Failed to add drone augment = {} for team = {}",
            method_name,
            drone_augment_type_id,
            team);
        if (out_error_message)
        {
            *out_error_message = error_message;
        }

        LogErr("{}", error_message);
        return false;
    }

    if (!EntityFactory::SpawnDroneAugmentEntity(*this, *drone_augment_data_ptr, team))
    {
        const std::string error_message =
            fmt::format("{} - Failed to spawn drone augment = {}", method_name, drone_augment_type_id);
        if (out_error_message)
        {
            *out_error_message = error_message;
        }

        LogErr("{}", error_message);
        return false;
    }

    return true;
}

void World::RemoveAllDroneAugmentsBeforeBattleStarted()
{
    if (IsBattleStarted())
    {
        LogErr("World::RemoveAllDroneAugmentsBeforeBattleStarted - NOT ALLOWED (game already started). IGNORING.");
        return;
    }

    drone_augments_state_.ResetState();

    // Remove spawned drone augments
    std::vector<std::shared_ptr<Entity>> entities_to_remove{};

    for (const auto& entity : entities_)
    {
        if (!EntityHelper::IsADroneAugment(*entity))
        {
            continue;
        }

        entities_to_remove.push_back(entity);
    }

    for (const auto& entity_to_remove : entities_to_remove)
    {
        EraseEntity(entity_to_remove->GetID());
    }
}

const HyperConfig& World::GetHyperConfig() const
{
    return GetGameDataContainer().GetHyperConfig();
}

std::shared_ptr<Entity> World::GetActiveFocusedEffectEntity(const Entity& entity) const
{
    std::shared_ptr<Entity> closest_focused = nullptr;
    int closest_dist = std::numeric_limits<int>::max();

    // Need position for this to work
    if (!entity.Has<PositionComponent>())
    {
        return nullptr;
    }

    if (!EntityHelper::IsACombatUnit(entity))
    {
        return nullptr;
    }

    // Get the position
    const auto& position_component = grid_helper_.GetSourcePositionComponent(entity);

    // Look for focused attached effect
    for (const auto& opponent_entity : GetAll())
    {
        // Ignore if it's not an opponent
        if (opponent_entity->IsAlliedWith(entity))
        {
            continue;
        }

        // Ignore units that are not targetable
        if (!EntityHelper::IsTargetable(*opponent_entity))
        {
            continue;
        }

        // Must have attached effects for this to work
        if (!opponent_entity->Has<AttachedEffectsComponent>())
        {
            continue;
        }

        // Check if this opponent has focused state
        if (EntityHelper::IsFocused(*opponent_entity))
        {
            // Get opponent position
            const auto& opponent_position_component = opponent_entity->Get<PositionComponent>();

            // Get vector and distance between entities
            const HexGridPosition vector_to_other =
                opponent_position_component.GetPosition() - position_component->GetPosition();
            const int sum_dist_from_center = opponent_position_component.GetRadius() + position_component->GetRadius();
            const int dist = vector_to_other.Length() - sum_dist_from_center;

            // Check if closest
            if (dist < closest_dist)
            {
                closest_dist = dist;
                closest_focused = opponent_entity;
            }
        }
    }

    // Return closest, if any
    return closest_focused;
}

const StatsData& World::GetBaseStats(const EntityID id) const
{
    if (!HasEntity(id))
    {
        // Try the deleted cache?
        if (data_destroyed_entities_map_.contains(id))
        {
            return data_destroyed_entities_map_.at(id).base_stats;
        }

        return empty_default_stats_;
    }

    const auto& entity = GetByID(id);
    return GetBaseStats(entity);
}

const StatsData& World::GetBaseStats(const Entity& entity) const
{
    // Does not have the required stats component
    if (!entity.Has<StatsComponent>())
    {
        return empty_default_stats_;
    }

    // Update the cache if the entity does not have an entry
    // or if the battle is not started (as we can remove/add augments/weapons)
    const EntityID id = entity.GetID();
    if (!IsBattleStarted() || !entities_base_stats_map_.contains(id))
    {
        const auto& stats_component = entity.Get<StatsComponent>();
        entities_base_stats_map_[id] = stats_component.GetBaseStats();
    }

    return entities_base_stats_map_.at(id);
}

const Entity* World::GetStatSourceEntity(const Entity& entity) const
{
    if (entity.Has<StatsComponent>())
    {
        return &entity;
    }
    else if (entity.Has<ShieldComponent>())
    {
        const auto& shield_state_component = entity.Get<ShieldComponent>();
        const auto& owner_id = shield_state_component.GetReceiverID();
        return GetByIDPtr(owner_id).get();
    }

    return nullptr;
}

StatsData World::GetLiveStats(const Entity& entity) const
{
    const Entity* stat_source = GetStatSourceEntity(entity);

    if (!stat_source) return {};

    if (const auto* attached_effects_component = entity.GetPtr<AttachedEffectsComponent>())
    {
        // Use the buffs/debuffs to calculate the current stats
        return CalculateLiveStats(*stat_source, attached_effects_component->GetBuffsState());
    }
    else
    {
        // Does not have attached effects, use empty buffs/debuffs
        return CalculateLiveStats(*stat_source, {});
    }
}

StatsData World::CalculateLiveStats(const Entity& receiver_entity, const AttachedEffectBuffsState& buffs_state) const
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID receiver_id = receiver_entity.GetID();
    const auto& receiver_stats_component = receiver_entity.Get<StatsComponent>();

    // NOTE: We bypass the cache, because we want to get the true current health/energy/sub hyper values
    const StatsData receiver_base_stats = receiver_stats_component.GetBaseStats();

    const FixedPoint before_current_energy = receiver_stats_component.GetCurrentEnergy();
    const FixedPoint before_current_health = receiver_stats_component.GetCurrentHealth();
    if (GetLogsConfig().enable_calculate_live_stats)
    {
        LogDebug(receiver_id, "World::CalculateLiveStats");
        LogDebug(
            receiver_id,
            "- BEFORE current_health = {}, max_health = {}",
            before_current_health,
            receiver_base_stats.Get(StatType::kMaxHealth));
    }

    // Calculate total buffs total and debuffs total for each stat type
    StatsData buffs_total;
    StatsData buffs_percentages_total;
    StatsData debuffs_total;
    StatsData debuffs_percentages_total;

    // Add buffs
    for (const auto& [stat_type, all_attached_buffs] : buffs_state.buffs)
    {
        // Compute all the buffs
        FixedPoint buff_total_value = 0_fp;
        for (const AttachedEffectStatePtr& attached_buff : all_attached_buffs)
        {
            buff_total_value += AddValueToTotalsForBuffsDebuffs(*attached_buff, &buffs_total, &buffs_percentages_total);
        }

        if (GetLogsConfig().enable_calculate_live_stats)
        {
            LogDebug(
                receiver_id,
                "| buff stat_type = {}, buff_total_value = {}, size = {}",
                stat_type,
                buff_total_value,
                all_attached_buffs.size());
        }
    }

    // Calculate debuffs only if not immune to detrimental effects
    const bool calculate_debuffs = !EntityHelper::IsImmuneToAllDetrimentalEffects(receiver_entity);
    if (calculate_debuffs)
    {
        // Subtract debuffs
        for (const auto& [stat_type, all_attached_debuffs] : buffs_state.debuffs)
        {
            FixedPoint debuff_total_value = 0_fp;
            for (const AttachedEffectStatePtr& attached_debuff : all_attached_debuffs)
            {
                debuff_total_value +=
                    AddValueToTotalsForBuffsDebuffs(*attached_debuff, &debuffs_total, &debuffs_percentages_total);
            }

            if (GetLogsConfig().enable_calculate_live_stats)
            {
                LogDebug(
                    receiver_id,
                    "| debuff stat_type = {}, debuff_total_value = {}, size = {}",
                    stat_type,
                    debuff_total_value,
                    all_attached_debuffs.size());
            }
        }
    }

    // Calculates the buff from the attaches entities
    CalculateBuffsFromAttachedEntities(receiver_entity, &buffs_total, &buffs_percentages_total);

    // Calculate live stats
    StatsData receiver_live_stats = receiver_base_stats;

    // Obtain willpower to adjust debuffs
    const FixedPoint willpower_percentage = kMaxPercentageFP - receiver_live_stats.Get(StatType::kWillpowerPercentage);

    // Calculate buffs
    // Iterate over all fields
    for (const StatType stat : EnumSet<StatType>::MakeFull())
    {
        // Compute the base/buff/debuff values
        const FixedPoint base_value = receiver_live_stats.Get(stat);
        const FixedPoint buff_total_value = buffs_total.Get(stat);
        const FixedPoint buff_percentage_total_value = buffs_percentages_total.Get(stat);
        FixedPoint debuff_total_value = debuffs_total.Get(stat);
        FixedPoint debuff_percentage_total_value = debuffs_percentages_total.Get(stat);

        if (buff_total_value == 0_fp && debuff_total_value == 0_fp && buff_percentage_total_value == 0_fp &&
            debuff_percentage_total_value == 0_fp)
        {
            continue;
        }

        // Adjust debuffs by willpower
        if (debuff_total_value != 0_fp)
        {
            debuff_total_value = willpower_percentage.AsPercentageOf(debuff_total_value);
        }
        if (debuff_percentage_total_value != 0_fp)
        {
            debuff_percentage_total_value = willpower_percentage.AsPercentageOf(debuff_percentage_total_value);
        }

        if (StatsHelper::IsStandardStatType(stat))
        {
            // https://illuvium.atlassian.net/wiki/spaces/AB/pages/100335901/Standard+Combat+Stat
            const FixedPoint value = base_value + buff_total_value - debuff_total_value;
            const FixedPoint percentage =
                kMaxPercentageFP + buff_percentage_total_value - debuff_percentage_total_value;
            const FixedPoint live_value = percentage.AsPercentageOf(value);
            receiver_live_stats.Set(stat, live_value);
        }
        else if (StatsHelper::IsNegatedStatType(stat))
        {
            // https://illuvium.atlassian.net/wiki/spaces/AB/pages/206569663/Negated+Combat+Stat
            const FixedPoint value = base_value - buff_total_value + debuff_total_value;
            const FixedPoint percentage =
                kMaxPercentageFP - buff_percentage_total_value + debuff_percentage_total_value;
            const FixedPoint live_value = percentage.AsPercentageOf(value);
            receiver_live_stats.Set(stat, live_value);
        }
        else
        {
            LogWarn(receiver_id, "| stat = `{}` is not handled by any category", stat);
        }

        if (GetLogsConfig().enable_calculate_live_stats)
        {
            LogDebug(
                receiver_id,
                "| Calculate - stat_type = {}, base_value = {}, current_value = {}, buff_total_value = {}, "
                " buff_percentage_total_value = {}, debuff_total_value = {}, debuff_percentage_total_value = {}",
                stat,
                base_value,
                receiver_live_stats.Get(stat),
                buff_total_value,
                buff_percentage_total_value,
                debuff_total_value,
                debuff_percentage_total_value);
        }
    }

    // Clamp/abs values
    for (const StatType stat : EnumSet<StatType>::MakeFull())
    {
        const FixedPoint current_value = receiver_live_stats.Get(stat);
        if (StatsHelper::IsPercentageTypeToClamp(stat))
        {
            receiver_live_stats.Set(stat, std::clamp(current_value, kMinPercentageFP, kMaxPercentageFP));
        }
        else
        {
            receiver_live_stats.Set(stat, std::max(0_fp, current_value));
        }
    }

    const FixedPoint after_current_energy = receiver_live_stats.Get(StatType::kCurrentEnergy);
    const FixedPoint after_current_health = receiver_live_stats.Get(StatType::kCurrentHealth);
    if (GetLogsConfig().enable_calculate_live_stats)
    {
        LogDebug(
            receiver_id,
            "- AFTER current_health = {}, max_health = {}, attack_dodge_chance_percentage = {}",
            after_current_health,
            receiver_live_stats.Get(StatType::kMaxHealth),
            receiver_live_stats.Get(StatType::kAttackDodgeChancePercentage));
    }

    // Sanity check, buffs/debuffs shouldn't affect current health/energy
    if (before_current_health != after_current_health)
    {
        LogErr(
            receiver_id,
            "- HEALTH MODIFIED BY BUFF - before_current_health ({}) != after_current_health ({})",
            before_current_health,
            after_current_health);
        assert(before_current_health == after_current_health);
    }
    if (before_current_energy != after_current_energy)
    {
        LogErr(
            receiver_id,
            "- ENERGY MODIFIED BY BUFF - before_current_energy ({}) != after_current_energy ({})",
            before_current_energy,
            after_current_energy);
        assert(before_current_energy == after_current_energy);
    }

    // Current health should be the same as the stats component current health
    if (after_current_health != receiver_stats_component.GetCurrentHealth())
    {
        LogErr(
            receiver_id,
            "- HEALTH DIFFERENT FROM STATS COMPONENT - after_current_health ({}) != "
            "StatsComponent::GetCurrentHealth ({})",
            after_current_health,
            receiver_stats_component.GetCurrentHealth());
        assert(after_current_health == receiver_stats_component.GetCurrentHealth());
    }

    return receiver_live_stats;
}

FixedPoint World::AddValueToTotalsForBuffsDebuffs(
    const AttachedEffectState& state,
    StatsData* out_totals,
    StatsData* out_percentages_totals) const
{
    auto& effect_data = state.effect_data;
    const StatType stat = effect_data.type_id.stat_type;

    const FixedPoint& buff_value = state.GetCapturedValue();

    // Set the out depending as how it is used,
    // Stats that are based on percentages cannot be calculated using a percentage buff
    // - https://illuvium.atlassian.net/wiki/spaces/AB/pages/272728087/Percentage+Combat+Stat
    if (effect_data.GetExpression().is_used_as_percentage && !StatsHelper::IsPercentageType(stat))
    {
        // Percentage buff
        const FixedPoint current_value = out_percentages_totals->Get(stat);
        out_percentages_totals->Set(stat, current_value + buff_value);
    }
    else
    {
        // Standard buff
        const FixedPoint current_value = out_totals->Get(stat);
        out_totals->Set(stat, current_value + buff_value);
    }

    return buff_value;
}

void World::CalculateBuffsFromAttachedEntities(
    const Entity& receiver_entity,
    StatsData* out_buffs_total,
    StatsData* out_buffs_percentages_total) const
{
    if (!receiver_entity.Has<AttachedEntityComponent>())
    {
        return;
    }

    const EntityID receiver_id = receiver_entity.GetID();
    const auto& attached_entity_component = receiver_entity.Get<AttachedEntityComponent>();
    const auto& attached_entities = attached_entity_component.GetAttachedEntities();
    for (const auto& attached_entity : attached_entities)
    {
        if (!HasEntity(attached_entity.id))
        {
            LogWarn(receiver_id, "| attached entity id = {} does not exist", attached_entity.id);
            continue;
        }

        // Get entity details
        const auto& entity = GetByID(attached_entity.id);
        const auto& effects_component = entity.Get<AttachedEffectsComponent>();
        const auto& entity_buffs_state = effects_component.GetBuffsState();

        // Apply buffs from this entity
        for (const auto& [stat_type, all_attached_buffs] : entity_buffs_state.buffs)
        {
            // Compute all the buffs
            FixedPoint buff_total_value = 0_fp;
            for (const AttachedEffectStatePtr& attached_buff : all_attached_buffs)
            {
                buff_total_value +=
                    AddValueToTotalsForBuffsDebuffs(*attached_buff, out_buffs_total, out_buffs_percentages_total);
            }

            if (GetLogsConfig().enable_calculate_live_stats)
            {
                LogDebug(
                    receiver_id,
                    "| attached entity buff stat_type = {}, buff_total_value = {}, size = {}",
                    stat_type,
                    buff_total_value,
                    all_attached_buffs.size());
            }
        }
    }
}

bool World::DestroySpawnedEntity(const EntityID id)
{
    if (!EntityHelper::IsSpawned(*this, id))
    {
        return false;
    }

    LogDebug(id, "World::DestroySpawnedEntity");
    auto& entity = GetByID(id);

    // Fill the last known data cache for the destroyed spawned entities
    {
        CachedDataDestroyedSpawnedEntity data;
        data.team = entity.GetTeam();
        data.base_stats = GetBaseStats(id);
        data.parent_id = entity.GetParentID();
        data_destroyed_entities_map_[id] = data;
    }

    // Deactivate the entity so that the rest of the systems in the current time step don't handle
    // it
    EntityHelper::Kill(entity);

    // Already exists, this is a problem (but's that is okay during end of battle cleanup)
    if (entities_to_delete_.contains(id) && !is_battle_finished_)
    {
        LogWarn(
            id,
            "World::DestroySpawnedEntity - ALREADY MARKED FOR DESTRUCTION. IS THERE A BUG IN YOUR "
            "CODE?");
        return true;
    }

    // Mark for deletion in the next time step
    entities_to_delete_.insert(id);

    if (EntityHelper::IsAProjectile(*this, id))
    {
        // Emit event
        const auto& position_component = entity.Get<PositionComponent>();
        BuildAndEmitEvent<EventType::kProjectileDestroyed>(id, position_component.GetPosition());
    }
    else if (EntityHelper::IsAZone(*this, id))
    {
        // Emit event
        const auto& position_component = entity.Get<PositionComponent>();
        BuildAndEmitEvent<EventType::kZoneDestroyed>(id, position_component.GetPosition());
    }
    else if (EntityHelper::IsABeam(*this, id))
    {
        // Emit event
        const auto& position_component = entity.Get<PositionComponent>();
        BuildAndEmitEvent<EventType::kBeamDestroyed>(id, position_component.GetPosition());
    }
    else if (EntityHelper::IsAChain(*this, id))
    {
        // Emit event
        const auto& position_component = entity.Get<PositionComponent>();

        // Find out if this is the last chain
        assert(EntityHelper::IsACombatUnit(*this, entity.GetParentID()));
        bool is_last_chain = true;
        const std::unordered_set<EntityID> child_entities = GetChildEntities(entity.GetParentID());
        for (const EntityID child_id : child_entities)
        {
            // Has another chain that is not this chain we are about to destroy
            if (child_id != id && EntityHelper::IsAChain(*this, child_id))
            {
                is_last_chain = false;
                break;
            }
        }

        BuildAndEmitEvent<EventType::kChainDestroyed>(id, position_component.GetPosition(), is_last_chain);
    }
    else if (EntityHelper::IsASplash(*this, id))
    {
        // Emit event
        const auto& position_component = entity.Get<PositionComponent>();
        BuildAndEmitEvent<EventType::kSplashDestroyed>(id, position_component.GetPosition());
    }
    else if (EntityHelper::IsAShield(*this, id))
    {
        const auto& shield_state_component = entity.Get<ShieldComponent>();

        DestructionReason destruction_reason = DestructionReason::kNone;
        if (entity.Has<DeferredDestructionComponent>())
        {
            const auto& destruction_component = entity.Get<DeferredDestructionComponent>();
            destruction_reason = destruction_component.GetDestructionReason();
        }

        // Emit event
        BuildAndEmitEvent<EventType::kShieldDestroyed>(
            id,
            shield_state_component.GetSenderID(),
            shield_state_component.GetReceiverID(),
            destruction_reason);
    }
    else if (EntityHelper::IsAMark(*this, id))
    {
        const auto& mark_state_component = entity.Get<MarkComponent>();

        DestructionReason destruction_reason = DestructionReason::kNone;
        if (entity.Has<DeferredDestructionComponent>())
        {
            const auto& destruction_component = entity.Get<DeferredDestructionComponent>();
            destruction_reason = destruction_component.GetDestructionReason();
        }

        // Emit event
        BuildAndEmitEvent<EventType::kMarkDestroyed>(
            id,
            mark_state_component.GetSenderID(),
            mark_state_component.GetReceiverID(),
            destruction_reason);
    }
    else if (EntityHelper::IsADash(*this, id))
    {
        const auto& dash_component = entity.Get<DashComponent>();
        const EntityID dash_sender_id = dash_component.GetSenderID();

        // Emit event
        event_data::DashDestroyed dash_destroyed{};
        dash_destroyed.entity_id = id;
        dash_destroyed.sender_id = dash_sender_id;

        if (HasEntity(dash_sender_id))
        {
            const auto& dash_sender_entity = GetByID(dash_sender_id);
            auto& sender_movement_component = dash_sender_entity.Get<MovementComponent>();
            sender_movement_component.DecrementMovementPaused();  // Resume normal movement
            dash_destroyed.last_position = dash_sender_entity.Get<PositionComponent>().GetPosition();
        }
        else
        {
            dash_destroyed.last_position = entity.Get<PositionComponent>().GetPosition();
        }

        EmitEvent<EventType::kDashDestroyed>(dash_destroyed);
    }
    else if (EntityHelper::IsAnAura(*this, id))
    {
        EmitEvent<EventType::kAuraDestroyed>({.entity_id = id});
    }

    return true;
}

bool World::IsPendingDestruction(const Entity& entity) const
{
    if (entity.Has<DeferredDestructionComponent>())
    {
        const auto& destruction_component = entity.Get<DeferredDestructionComponent>();
        return destruction_component.IsPendingDestruction();
    }

    return false;
}

FixedPoint World::GetShieldTotal(const Entity& entity) const
{
    // Make sure entity has an AttachedEntityComponent, which could have shields
    if (!entity.Has<AttachedEntityComponent>())
    {
        return 0_fp;
    }

    // Get all the attached entities
    const auto& attached_entity_component = entity.Get<AttachedEntityComponent>();
    FixedPoint shield_total = 0_fp;

    // Iterate the attached entities to get the shields and the sum of their values
    const std::vector<AttachedEntity>& attached_entities = attached_entity_component.GetAttachedEntities();
    for (const AttachedEntity& attached_entity : attached_entities)
    {
        if (attached_entity.type != AttachedEntityType::kShield)
        {
            continue;
        }
        if (!HasEntity(attached_entity.id))
        {
            LogWarn(
                entity.GetID(),
                "World::GetShieldTotal - attached entity id = {} does not exist",
                attached_entity.id);
            continue;
        }

        const auto& shield_entity = GetByID(attached_entity.id);

        // Ignore shields that are pending destruction
        if (!IsPendingDestruction(shield_entity))
        {
            assert(shield_entity.Has<ShieldComponent>());
            const auto& shield_component = shield_entity.Get<ShieldComponent>();
            shield_total += shield_component.GetValue();
        }
    }

    return shield_total;
}

bool World::DoesPassConditions(const Entity& receiver_entity, const EnumSet<ConditionType>& required_conditions) const
{
    if (required_conditions.IsEmpty())
    {
        return true;
    }

    for (ConditionType condition : required_conditions)
    {
        switch (condition)
        {
        case ConditionType::kPoisoned:
        {
            // Confirm the target is poisoned
            if (!EntityHelper::IsPoisoned(receiver_entity))
            {
                return false;
            }
            break;
        }
        case ConditionType::kWounded:
            // Confirm the target is wounded
            if (!EntityHelper::IsWounded(receiver_entity))
            {
                return false;
            }
            break;
        case ConditionType::kBurned:
            // Confirm the target is burned
            if (!EntityHelper::IsBurned(receiver_entity))
            {
                return false;
            }
            break;
        case ConditionType::kFrosted:
            // Confirm the target is frosted
            if (!EntityHelper::IsFrosted(receiver_entity))
            {
                return false;
            }
            break;
        case ConditionType::kShielded:
            // Confirm the target is shielded
            if (GetShieldTotal(receiver_entity) <= 0_fp)
            {
                return false;
            }
            break;
        default:
            LogErr(receiver_entity.GetID(), "World::DoesPassConditions - condition = `{}` not handled", condition);
            return false;
        }
    }
    return true;
}

bool World::AreAllies(const std::vector<EntityID>& entities) const
{
    if (entities.empty())
    {
        return false;
    }

    Team team = Team::kNone;
    for (const EntityID id : entities)
    {
        // Skip over invalid entities
        if (!HasEntity(id))
        {
            continue;
        }

        const Entity& entity = GetByID(id);

        // Set first team
        if (team == Team::kNone)
        {
            team = entity.GetTeam();
            continue;
        }

        // Different owner, not allies
        if (team != entity.GetTeam())
        {
            return false;
        }
    }

    return true;
}

EventHandleID World::SubscribeToEvent(const EventType event_type, const EventCallback& listener)
{
    assert(Event::IsEventTypeValid(event_type));
    assert(events_subscribers_.size() == Event::kMaxEvents);

    const size_t event_type_index = static_cast<size_t>(event_type);
    std::vector<EventCallbackPtr>& event_listeners = events_subscribers_[event_type_index];

    EventCallbackPtr listener_ptr = std::make_shared<EventCallback>(listener);
    const EventCallbackWeakPtr listener_weak_ptr = listener_ptr;
    event_listeners.emplace_back(std::move(listener_ptr));

    EventHandleID event_handle_id;
    event_handle_id.listener_weak_ptr = listener_weak_ptr;
    event_handle_id.type = event_type;
    return event_handle_id;
}

void World::UnsubscribeFromEvent(const EventHandleID& event_handle_id)
{
    assert(Event::IsEventTypeValid(event_handle_id.type));
    assert(events_subscribers_.size() == Event::kMaxEvents);

    const size_t event_type_index = static_cast<size_t>(event_handle_id.type);
    std::vector<EventCallbackPtr>& event_listeners = events_subscribers_[event_type_index];

    // Only remove if listener is still valid
    if (!event_handle_id.listener_weak_ptr.expired())
    {
        const EventCallbackPtr listener_ptr = event_handle_id.listener_weak_ptr.lock();
        VectorHelper::EraseValue(event_listeners, listener_ptr);
    }
}

void World::EmitEvent(const Event& event) const
{
    const size_t event_index = static_cast<size_t>(event.GetTypeAsInt());

    const std::vector<EventCallbackPtr>& event_listeners = events_subscribers_[event_index];
    const size_t size_before_loop = event_listeners.size();
    for (size_t index = 0; index < size_before_loop; index++)
    {
        const EventCallbackPtr listener_ptr = event_listeners[index];
        assert(listener_ptr);

        const EventCallback& listener = *listener_ptr;
        listener(event);
    }

    assert(size_before_loop == event_listeners.size());
}

EntityID World::GetCombatUnitParentID(const EntityID id) const
{
    // Try the deleted cache
    if (!HasEntity(id))
    {
        if (data_destroyed_entities_map_.contains(id))
        {
            return GetCombatUnitParentID(data_destroyed_entities_map_.at(id).parent_id);
        }

        return kInvalidEntityID;
    }

    return GetCombatUnitParentID(GetByID(id));
}

EntityID World::GetCombatUnitParentID(const Entity& entity) const
{
    // The entity is the combat unit
    // TODO(vampy): Handle this specially for pets as they are spawned entities
    if (EntityHelper::IsACombatUnit(entity))
    {
        return entity.GetID();
    }

    EntityID combat_unit_id = entity.GetParentID();
    while (true)
    {
        if (!HasEntity(combat_unit_id))
        {
            // Does not exist
            break;
        }

        if (EntityHelper::IsACombatUnit(*this, combat_unit_id))
        {
            // Found parent
            return combat_unit_id;
        }
        else
        {
            // Walk up the tree
            combat_unit_id = GetByID(combat_unit_id).GetParentID();
        }
    }

    return kInvalidEntityID;
}

std::unordered_set<EntityID> World::GetChildEntities(const EntityID id) const
{
    std::unordered_set<EntityID> child_entities;
    for (const auto& entity : GetAll())
    {
        if (entity->GetParentID() == id)
        {
            child_entities.insert(entity->GetID());
        }
    }

    return child_entities;
}

void World::ReorderEntitiesToMatchBeforeBattleStarted(const std::vector<std::string>& unique_ids_order)
{
    if (IsBattleStarted())
    {
        LogErr("World::ReorderEntitiesToMatchBeforeBattleStarted - NOT ALLOWED (game already started). IGNORING.");
        return;
    }

    const size_t spawned_drone_augments = drone_augments_state_.CalculateDroneAugmentCount();

    // Size of unique_ids + drone augments must be equal to count of all spawned entities
    if (unique_ids_order.size() + spawned_drone_augments != entities_.size())
    {
        LogErr(
            "World::ReorderEntitiesToMatchBeforeBattleStarted - unique_ids_order.size({}) + drone augments ({}) != "
            "entities_.size({}). "
            "IGNORING.",
            unique_ids_order.size(),
            spawned_drone_augments,
            entities_.size());
        return;
    }

    // Check if all the unique ids already exist
    for (const std::string& id : unique_ids_order)
    {
        if (!HasCombatUnitUniqueID(id))
        {
            LogErr(
                "World::ReorderEntitiesToMatchBeforeBattleStarted - id = {} does not exist inside "
                "unique_ids_map_. IGNORING.",
                id);
            return;
        }
    }

    // Start rebuilding entities_ with the new order from unique_unique_ids_order
    entities_.clear();

    for (const std::string& id : unique_ids_order)
    {
        std::shared_ptr<Entity> entity = GetCombatUnitByUniqueIDPtr(id);
        entities_.push_back(entity);
    }
    LogDebug("World::ReorderEntitiesToMatchBeforeBattleStarted - unique_ids_order = {}", unique_ids_order);

    // Re-Spawn all drone augments after units, so their ability will be executed later
    // Important for using augment buffs
    constexpr std::array teams = {Team::kBlue, Team::kRed};
    for (const Team team : teams)
    {
        const std::vector<DroneAugmentData>& team_augments = drone_augments_state_.GetDroneAugments(team);
        for (const DroneAugmentData& drone_augment_data : team_augments)
        {
            if (!EntityFactory::SpawnDroneAugmentEntity(*this, drone_augment_data, team))
            {
                LogErr(
                    "World::ReorderEntitiesToMatchBeforeBattleStarted - Failed to spawn drone augment = {}",
                    drone_augment_data.type_id);
            }
        }
    }

    // Rebuild entity_id_to_index_map_
    entity_id_to_index_map_.clear();
    UpdateEntityIDToIndexMap();
}

void World::SortEntitiesByUniqueID()
{
    std::vector<std::string> ids;
    ids.reserve(unique_ids_map_.size());
    for (const auto& [id, _] : unique_ids_map_)
    {
        ids.push_back(id);
    }

    const bool sort_ascending = Math::IsEven(config_.battle_config.random_seed);
    if (sort_ascending)
    {
        std::sort(ids.begin(), ids.end(), std::less<std::string>{});
    }
    else
    {
        std::sort(ids.begin(), ids.end(), std::greater<std::string>{});
    }

    ReorderEntitiesToMatchBeforeBattleStarted(ids);
}

void World::ApplyEncounterModsToCombatUnit(const Entity& entity) const
{
    if (!EntityHelper::IsACombatUnit(entity))
    {
        return;
    }

    if (!config_.battle_config.teams_encounter_mods.contains(entity.GetTeam()))
    {
        return;
    }

    static constexpr std::string_view method_name = "ApplyEncounterModsToCombatUnit";
    AbilitiesComponent& abilities_component = entity.Get<AbilitiesComponent>();

    for (const EncounterModInstanceData& encounter_mod_instance :
         config_.battle_config.teams_encounter_mods.at(entity.GetTeam()))
    {
        const EncounterModTypeID& type_id = encounter_mod_instance.type_id;

        if (!game_data_container_->GetEncounterModData(type_id))
        {
            LogErr("{} - can't find encounter data for type id \"{}\"", method_name, type_id);
            continue;
        }

        LogDebug(entity.GetID(), "{} - Applying encounter mod {}", method_name, encounter_mod_instance);
        if (const auto encounter_mod_data = game_data_container_->GetEncounterModData(type_id))
        {
            for (const auto& innate : encounter_mod_data->innate_abilities)
            {
                abilities_component.AddDataInnateAbility(innate);
            }
        }
    }
}

void World::ApplyEncounterModsBeforeBattleStarted()
{
    static constexpr std::string_view method_name = "ApplyEncounterModsBeforeBattleStarted";

    if (IsBattleStarted())
    {
        LogErr("{} - NOT ALLOWED (game already started). IGNORING.", method_name);
        return;
    }

    SafeWalkAll(
        [&](const Entity& entity)
        {
            ApplyEncounterModsToCombatUnit(entity);
        });
}

void World::EraseEntitiesMarkedForDeletion()
{
    // TODO(vampy): Maybe only empty this after we reach a threshold

    // Walk through the entities array in reverse, and if the entity is marked for removal we erase it.
    // It happens in reverse to avoid entity_id_to_index_map_ map invalidation
    size_t index = entities_.size();
    while (index > 0)
    {
        --index;

        const EntityID entity_id = entities_[index]->GetID();

        // if entity marked for deletion
        if (entities_to_delete_.contains(entity_id))
        {
            EraseEntity(entity_id);
        }
    }
    entities_to_delete_.clear();
    UpdateEntityIDToIndexMap();
}

void World::SortEntitiesTopologically()
{
    ILLUVIUM_PROFILE_FUNCTION();

    if (!has_to_reorder_entities_) return;

    has_to_reorder_entities_ = false;

    // Cleanup
    for (auto it = entities_tick_dependents_.begin(); it != entities_tick_dependents_.end();)
    {
        const EntityID entity_id = it->first;
        if (HasEntity(entity_id))
        {
            std::vector<EntityID>& dependents = it->second;
            VectorHelper::EraseValuesForCondition(
                dependents,
                [this](const EntityID id)
                {
                    return !HasEntity(id);
                });

            ++it;
        }
        else
        {
            it = entities_tick_dependents_.erase(it);
        }
    }

    // Move entities to the stack and fill entities array during graph walk.
    // Entities added to the stack in reverse order because we ae going to pop from back
    std::vector<std::shared_ptr<Entity>> stack(entities_.rbegin(), entities_.rend());

    // Have to store algorithm result in a separate collection because
    // it will use GetIDPtr while walking the graph, which uses entities_ array
    std::vector<std::shared_ptr<Entity>> sorted_entities;

    // Set of already added entities
    std::unordered_set<EntityID> visited;

    // DFS. Flattens the graph in a way that each node appears after every of its parent
    while (!stack.empty())
    {
        // Take the top item from the stack
        auto entity_ptr = stack.back();
        const EntityID entity_id = entity_ptr->GetID();
        stack.pop_back();

        {
            // Try add to visited list.
            const bool inserted = visited.insert(entity_id).second;

            // Skip an iteration if already visited
            if (!inserted) continue;
        }

        // Add entity to result array
        sorted_entities.push_back(entity_ptr);

        // Add dependants (if any) to the beginning of the queue
        if (entities_tick_dependents_.contains(entity_id))
        {
            for (const EntityID depedant : entities_tick_dependents_.at(entity_id))
            {
                stack.push_back(GetByIDPtr(depedant));
            }
        }
    }

    entities_ = sorted_entities;

    UpdateEntityIDToIndexMap();
}

void World::ChangeCombatUnitPositionBeforeBattleStarted(const EntityID id, const HexGridPosition& new_position)
{
    if (!HasEntity(id))
    {
        return;
    }
    if (IsBattleStarted())
    {
        LogErr(
            id,
            "World::ChangeCombatUnitPositionBeforeBattleStarted - NOT ALLOWED (game already started). IGNORING.");
        return;
    }

    const auto& entity = GetByID(id);
    if (!entity.Has<PositionComponent>())
    {
        LogErr(
            id,
            "World::ChangeCombatUnitPositionBeforeBattleStarted - Entity doesn't have the PositionComponent. "
            "IGNORING.");
        return;
    }

    LogDebug(id, "World::ChangeCombatUnitPositionBeforeBattleStarted - new_position = {}", new_position);
    auto& position = entity.Get<PositionComponent>();
    position.SetPosition(new_position);
}

bool World::ChangeCombatUnitTeamBeforeBattleStarted(const EntityID id, const Team new_team)
{
    if (!HasEntity(id))
    {
        return false;
    }
    if (IsBattleStarted())
    {
        LogErr(id, "World::ChangeCombatUnitTeamBeforeBattleStarted - NOT ALLOWED (game already started). IGNORING.");
        return false;
    }

    auto& entity = GetByID(id);
    if (entity.IsAlliedWith(new_team))
    {
        // Same team
        return false;
    }

    // Set the new team
    LogDebug(id, "World::ChangeCombatUnitTeamBeforeBattleStarted - new_team = {}", new_team);
    entity.SetTeam(new_team);

    // Refresh the synergy system
    RefreshCombatSynergiesBeforeBattleStarted();

    return true;
}

bool World::RemoveCombatUnitBeforeBattleStarted(const EntityID id)
{
    if (!HasEntity(id))
    {
        // This method can be invoked in some cases when a selection is cancel in Unreal
        // end the entity is not yet created; in this case return
        return false;
    }

    if (IsBattleStarted())
    {
        LogErr(id, "World::RemoveCombatUnitBeforeBattleStarted - NOT ALLOWED (game already started). IGNORING.");
        return false;
    }

    const auto& entity_to_remove = GetByIDPtr(id);
    if (!EntityHelper::IsACombatUnit(*entity_to_remove))
    {
        LogErr(id, "World::RemoveCombatUnitBeforeBattleStarted - Entity is not a combat unit. IGNORING.");
        return false;
    }

    // Erase from fast map
    const auto& combat_unit_component = entity_to_remove->Get<CombatUnitComponent>();
    assert(!combat_unit_component.GetUniqueID().empty());
    unique_ids_map_.erase(combat_unit_component.GetUniqueID());

    // Copy team
    const Team team = entity_to_remove->GetTeam();
    EraseEntity(id);
    UpdateEntityIDToIndexMap();

    // Remove team if there is no other entity with that team
    bool team_available = false;
    for (const auto& entity : entities_)
    {
        if (entity->IsAlliedWith(team))
        {
            team_available = true;
            break;
        }
    }

    // the removed entity had the last owner. remove owner as well
    if (team_available == false)
    {
        VectorHelper::EraseValue(teams_, team);
        LogDebug("World::RemoveCombatUnitBeforeBattleStarted - Removed from team: {}", team);
    }

    if (entities_.empty())
    {
        last_added_entity_id_ = kInvalidEntityID;
    }

    // Refresh the synergy system
    RefreshCombatSynergiesBeforeBattleStarted();

    return true;
}

bool World::CheckAllBondsBeforeBattleStarted()
{
    if (IsBattleStarted())
    {
        LogErr("World::CheckAllBondsBeforeBattleStarted - NOT ALLOWED (game already started). IGNORING.");
        return false;
    }

    const auto& all_entities = GetAll();
    for (const auto& entity : all_entities)
    {
        CheckRangerBondBeforeBattleStarted(*entity);
    }

    RefreshCombatSynergiesBeforeBattleStarted();
    return true;
}

bool World::CheckRangerBondBeforeBattleStarted(const EntityID ranger_entity_id)
{
    if (!HasEntity(ranger_entity_id))
    {
        return false;
    }

    return CheckRangerBondBeforeBattleStarted(GetByID(ranger_entity_id));
}

bool World::CheckRangerBondBeforeBattleStarted(const std::string& ranger_unique_id)
{
    if (!HasCombatUnitUniqueID(ranger_unique_id))
    {
        return false;
    }

    return CheckRangerBondBeforeBattleStarted(*GetCombatUnitByUniqueIDPtr(ranger_unique_id));
}

bool World::CheckRangerBondBeforeBattleStarted(const Entity& ranger_entity)
{
    // Check only rangers
    if (!EntityHelper::IsARangerCombatUnit(ranger_entity))
    {
        return false;
    }
    const auto& combat_unit_component = ranger_entity.Get<CombatUnitComponent>();
    auto& combat_synergy_component = ranger_entity.Get<CombatSynergyComponent>();

    // Default to TypeData in case we can't find any bonded_entity
    combat_synergy_component.SetCombatAffinity(combat_synergy_component.GetDefaultTypeDataCombatAffinity());
    combat_synergy_component.SetCombatClass(combat_synergy_component.GetDefaultTypeDataCombatClass());

    // Get the entity that matches this bond
    const std::string& bonded_id = combat_unit_component.GetBondedUniqueID();
    std::shared_ptr<Entity> bonded_entity = GetBondedEntity(ranger_entity, bonded_id);

    // Can't find anything
    if (bonded_entity == nullptr)
    {
        // Was set from bond but it got invalidated because the bonded entity was removed from the battle board
        if (combat_synergy_component.IsSetFromBond())
        {
            combat_synergy_component.SetIsSetFromBond(false);
            LogDebug(
                ranger_entity.GetID(),
                "World::CheckRangerBondBeforeBattleStarted - Restoring TypeData (combat_class = {}, combat_affinity "
                "= {}) because bonded_id = {} does not exist",
                combat_synergy_component.GetDefaultTypeDataCombatClass(),
                combat_synergy_component.GetDefaultTypeDataCombatAffinity(),
                bonded_id);
        }
        else if (!bonded_id.empty())
        {
            LogErr(
                ranger_entity.GetID(),
                "World::CheckRangerBondBeforeBattleStarted - Can't find bonded_id = {}",
                bonded_id);
        }
        return false;
    }

    //
    // Bond
    //
    const CombatAffinity current_combat_affinity = combat_synergy_component.GetCombatAffinity();
    const CombatClass current_combat_class = combat_synergy_component.GetCombatClass();

    const auto [bonded_primary_affinity, bonded_primary_class] =
        *EntityHelper::GetIlluvialPrimarySynergies(*bonded_entity);

    LogDebug(
        ranger_entity.GetID(),
        "World::CheckRangerBondBeforeBattleStarted - Bonding Classes = `{}` (Existing) + `{}` (Dominant), Affinities "
        "= `{}` (Existing) + `{}` (Dominant), bonded_id = {}",
        current_combat_class,
        bonded_primary_class,
        current_combat_affinity,
        bonded_primary_affinity,
        bonded_id);

    // Do the bonding using the helper
    const CombatSynergyBonus new_combat_affinity =
        GetSynergiesHelper().Bond(current_combat_affinity, bonded_primary_affinity);
    const CombatSynergyBonus new_combat_class = GetSynergiesHelper().Bond(current_combat_class, bonded_primary_class);

    // Set on the ranger
    combat_synergy_component.SetCombatClass(new_combat_class.GetClass());
    combat_synergy_component.SetCombatAffinity(new_combat_affinity.GetAffinity());

    // Set dominants if they were not already set from weapon
    if (combat_synergy_component.GetDominantCombatClass() == CombatClass::kNone)
    {
        combat_synergy_component.SetDominantCombatClass(bonded_primary_class);
    }
    if (combat_synergy_component.GetDominantCombatAffinity() == CombatAffinity::kNone)
    {
        combat_synergy_component.SetDominantCombatAffinity(bonded_primary_affinity);
    }
    combat_synergy_component.SetIsSetFromBond(true);

    LogDebug(
        ranger_entity.GetID(),
        "World::CheckRangerBondBeforeBattleStarted - new_combat_class = {}, new_combat_affinity = {}",
        combat_synergy_component.GetCombatClass(),
        combat_synergy_component.GetCombatAffinity());
    return true;
}

std::shared_ptr<Entity> World::GetBondedEntity(const Entity& ranger_entity, const std::string& bonded_id) const
{
    // Does not exist
    if (!HasCombatUnitUniqueID(bonded_id))
    {
        return nullptr;
    }

    // Found the bonded unit but it is not valid?
    const auto& possible_bonded_entity = GetCombatUnitByUniqueIDPtr(bonded_id);
    if (!EntityHelper::CanRangerBondWithIlluvial(ranger_entity, *possible_bonded_entity))
    {
        LogErr(
            ranger_entity.GetID(),
            "World::GetBondedEntity - Found bonded_id = {}, but it is either: not "
            "an ally, not a combat unit, or does not have any dominant combat synergies",
            bonded_id);
        return nullptr;
    }

    return possible_bonded_entity;
}

void World::RefreshCombatSynergiesBeforeBattleStarted()
{
    GetSystem<SynergySystem>().RefreshCombatSynergies();
}

Team World::GetEntityTeam(const EntityID id) const
{
    // Try the deleted cache?
    if (!HasEntity(id))
    {
        if (data_destroyed_entities_map_.contains(id))
        {
            return data_destroyed_entities_map_.at(id).team;
        }

        // This should never happen
        return Team::kNone;
    }

    // Exists
    return GetByID(id).GetTeam();
}

size_t World::GetEntityIDIndex(const EntityID id) const
{
    const auto& all_entities = GetAll();
    for (size_t i = 0; i < all_entities.size(); i++)
    {
        const auto& entity = all_entities[i];
        if (entity->GetID() == id)
        {
            return i;
        }
    }

    return kInvalidIndex;
}

int World::GetPreferredUnitRadius(const FullCombatUnitData& full_data) const
{
    static constexpr std::string_view method_name = "World::GetPreferredUnitRadius";
    const auto& type_id = full_data.GetCombatUnitTypeID();
    const bool is_illuvial = type_id.type == CombatUnitType::kIlluvial;
    const bool use_placement_radius =
        !IsBattleStarted() && is_illuvial && GetBattleConfig().use_max_stage_placement_radius;

    if (use_placement_radius)
    {
        int radius_units = 0;
        if (EvolutionHelper::FindMaximumPossibleRadiusInEvolutionGraph(
                GetGameDataContainer().GetCombatUnitsDataContainer(),
                type_id,
                &radius_units))
        {
            return radius_units;
        }

        LogErr(
            "{} - Failed to get data for one of the units from evolution tree for {}. Falling back to combat unit "
            "radius from combat unit data (r = {})",
            method_name,
            type_id,
            full_data.data.radius_units);
    }

    return full_data.data.radius_units;
}

bool World::UpdateCombatUnitFromData(
    const EntityID entity_id,
    const FullCombatUnitData& full_data,
    const bool refresh_synergies,
    std::string* out_error_message)
{
    static constexpr std::string_view method_name = "World::UpdateCombatUnitFromData";

    if (!HasEntity(entity_id))
    {
        const std::string error_message = fmt::format("{} - Entity id = {} does not exist", method_name, entity_id);
        if (out_error_message)
        {
            *out_error_message = error_message;
        }

        LogErr("{}", error_message);
        return false;
    }
    if (!EntityHelper::IsACombatUnit(*this, entity_id))
    {
        const std::string error_message = fmt::format("{} - Entity {} is not a COMBAT UNIT.", method_name, entity_id);
        if (out_error_message)
        {
            *out_error_message = error_message;
        }

        LogErr(entity_id, "{}", error_message);
        return false;
    }

    const CombatUnitInstanceData& instance = full_data.instance;
    const std::string& unique_id = instance.id;
    if (unique_id.empty())
    {
        const std::string error_message =
            fmt::format("{} - Missing unique id for type_id: {{{}}}", method_name, full_data.GetCombatUnitTypeID());
        if (out_error_message)
        {
            *out_error_message = error_message;
        }

        LogErr(entity_id, "{}", error_message);
        return false;
    }

    const auto& entity = GetByIDPtr(entity_id);
    auto& combat_unit_component = entity->Get<CombatUnitComponent>();
    auto& stats_component = entity->Get<StatsComponent>();
    auto& movement_component = entity->Get<MovementComponent>();
    auto& focus_component = entity->Get<FocusComponent>();
    auto& position_component = entity->Get<PositionComponent>();
    auto& combat_synergy_component = entity->Get<CombatSynergyComponent>();
    auto& abilities_component = entity->Get<AbilitiesComponent>();
    const CombatUnitTypeID& combat_unit_type_id = full_data.GetCombatUnitTypeID();

    // Clear previous map entry if the unique id changed in the meantime
    unique_ids_map_.erase(combat_unit_component.GetUniqueID());

    // Duplicate id?
    if (HasCombatUnitUniqueID(unique_id))
    {
        const auto& duplicate_entity_ptr = GetCombatUnitByUniqueIDPtr(unique_id);

        const std::string error_message = fmt::format(
            "{} - type_id: {}, unique_id = {} is already taken by duplicate_type_id = {}",
            method_name,
            full_data.GetCombatUnitTypeID(),
            unique_id,
            duplicate_entity_ptr->Get<CombatUnitComponent>().GetTypeID());
        if (out_error_message)
        {
            *out_error_message = error_message;
        }

        LogErr(entity_id, "{}", error_message);
        return false;
    }

    // CombatUnit always changes target to the most convenient one
    focus_component.SetSelectionType(FocusComponent::SelectionType::kClosestEnemy);
    focus_component.SetRefocusType(FocusComponent::RefocusType::kAlways);

    // Set new radius
    position_component.SetRadius(GetPreferredUnitRadius(full_data));

    // Synergies data
    // Check if the units is a ranger and if we should bond
    const CombatClass combat_class = equipment_helper_.GetDefaultTypeDataCombatClass(full_data);
    const CombatAffinity combat_affinity = equipment_helper_.GetDefaultTypeDataCombatAffinity(full_data);

    // By default the dominant class/affinity comes from the instance
    CombatClass dominant_combat_class = instance.dominant_combat_class;
    CombatAffinity dominant_combat_affinity = instance.dominant_combat_affinity;

    // Set level for the unit
    stats_component.SetLevel(full_data.GetLevel());

    // Set template for the Illuvial
    stats_component.SetTemplateStats(full_data.data.type_data.stats);

    // Set depending on type
    if (combat_unit_type_id.type == CombatUnitType::kRanger)
    {
        // Check if the weapon/suits even exist
        const CombatWeaponInstanceData& weapon_instance = instance.equipped_weapon;
        const CombatWeaponTypeID& weapon_type_id = weapon_instance.type_id;

        // Can equip normal weapons?
        std::string equip_error_message;
        if (!equipment_helper_.CanEquipNormalWeapon(weapon_type_id, &equip_error_message))
        {
            const std::string error_message = fmt::format("{} - {}", method_name, equip_error_message);
            if (out_error_message)
            {
                *out_error_message = error_message;
            }

            LogErr(entity->GetID(), "{}", error_message);
            return false;
        }

        // Can equip amplifiers on the weapon?
        for (const CombatWeaponAmplifierInstanceData& equipped_amplifier : weapon_instance.equipped_amplifiers)
        {
            if (!equipment_helper_.CanEquipAmplifierToWeaponInstance(
                    weapon_instance,
                    equipped_amplifier.type_id,
                    &equip_error_message))
            {
                const std::string error_message = fmt::format("{} - {}", method_name, equip_error_message);
                if (out_error_message)
                {
                    *out_error_message = error_message;
                }

                LogErr(entity->GetID(), "{}", error_message);
                return false;
            }
        }

        const CombatSuitTypeID& suit_type_id = full_data.GetEquippedSuitTypeID();
        const auto suit_data = GetGameDataContainer().GetSuitData(suit_type_id);
        if (!suit_data)
        {
            const std::string error_message =
                fmt::format("{} - Suit data for id = {}, DOES NOT EXIST!", method_name, suit_type_id);
            if (out_error_message)
            {
                *out_error_message = error_message;
            }

            LogErr(entity->GetID(), "{}", error_message);
            return false;
        }

        // Set equipment stats
        const StatsData total_weapon_stats = equipment_helper_.GetTotalWeaponInstanceStats(weapon_instance);
        stats_component.SetEquipmentStats(total_weapon_stats, suit_data->stats);

        // Set the type ids of the weapon and suit
        combat_unit_component.SetEquippedWeapon(instance.equipped_weapon);
        combat_unit_component.SetEquippedSuit(instance.equipped_suit);

        // For ranger this comes from the weapons
        const auto weapon_data = game_data_container_->GetWeaponData(weapon_type_id);
        assert(weapon_data);
        dominant_combat_class = weapon_data->dominant_combat_class;
        dominant_combat_affinity = weapon_data->dominant_combat_affinity;
    }

    // Convert random percentage stats to absolute values
    StatsData random_modifier_stats;
    for (const StatType stat_type : EnumSet<StatType>::MakeFull())
    {
        // Template + Equipment
        const FixedPoint base_value =
            stats_component.GetTemplateStats().Get(stat_type) + stats_component.GetEquipmentStats().Get(stat_type);
        const FixedPoint percentage = instance.random_percentage_modifier_stats.Get(stat_type);
        random_modifier_stats.Set(stat_type, percentage.AsPercentageOf(base_value));
    }

    // Keep track of the percentage stats
    stats_component.SetRandomPercentageModifierStats(instance.random_percentage_modifier_stats);

    // Add the random modifier stats
    stats_component.SetRandomModifierStats(random_modifier_stats);

    // Set custom stats overrides
    stats_component.OverrideBaseStats(instance.stats_overrides);

    // Set the TypeID and bonding data
    combat_unit_component.SetTypeID(combat_unit_type_id);
    combat_unit_component.SetUniqueID(unique_id);
    combat_unit_component.SetBondedUniqueID(instance.bonded_id);
    combat_unit_component.SetFinish(instance.finish);

    // Fill fast map for unique ids
    assert(!combat_unit_component.GetUniqueID().empty());
    unique_ids_map_[combat_unit_component.GetUniqueID()] = entity;

    // Set movement to navigate around other grid
    movement_component.SetMovementType(MovementComponent::MovementType::kNavigation);

    movement_component.SetMovementSpeedSubUnits(stats_component.GetBaseValueForType(StatType::kMoveSpeedSubUnits));

    // Save the original type data to restore if the bonded with combat unit is removed from the battle board.
    // Check CheckAllBondsBeforeBattleStarted.
    combat_synergy_component.SetDefaultTypeDataCombatClass(combat_class);
    combat_synergy_component.SetDefaultTypeDataCombatAffinity(combat_affinity);

    LogDebug(
        entity_id,
        "{} - combat_class = {}, combat_affinity = {}, dominant_combat_class = {}, "
        "dominant_combat_affinity = {}, unique_id = {}",
        method_name,
        combat_class,
        combat_affinity,
        dominant_combat_class,
        dominant_combat_affinity,
        combat_unit_component.GetUniqueID());

    combat_synergy_component.SetCombatClass(combat_class);
    combat_synergy_component.SetCombatAffinity(combat_affinity);
    combat_synergy_component.SetDominantCombatClass(dominant_combat_class);
    combat_synergy_component.SetDominantCombatAffinity(dominant_combat_affinity);

    // Set augments
    const auto& augment_component = entity->Get<AugmentComponent>();

    // Remove existing augments
    // NOTE: This is a copy because the vector is going to be modified by RemoveAugmentBeforeBattleStarted
    const std::vector<AugmentInstanceData> existing_augments = augment_component.GetEquippedAugments();
    for (const AugmentInstanceData& existing_augment : existing_augments)
    {
        RemoveAugmentBeforeBattleStarted(*entity, existing_augment);
    }

    // Add the augments from the instance
    for (const AugmentInstanceData& equipped_augment : instance.equipped_augments)
    {
        AddAugmentBeforeBattleStarted(*entity, equipped_augment);
    }

    // Remove existing consumables
    // NOTE: This is a copy because the vector is going to be modified by RemoveConsumableBeforeBattleStarted
    const auto& consumable_component = entity->Get<ConsumableComponent>();
    const std::vector<ConsumableInstanceData> existing_consumables = consumable_component.GetEquippedConsumables();
    for (const ConsumableInstanceData& existing_consumable : existing_consumables)
    {
        RemoveConsumableBeforeBattleStarted(*entity, existing_consumable);
    }

    // Add consumables
    for (const ConsumableInstanceData& equipped_consumable : instance.equipped_consumables)
    {
        AddConsumableBeforeBattleStarted(*entity, equipped_consumable);
    }

    auto setup_abilities = [&](const AbilityType ability_type) -> bool
    {
        std::string error_setup_abilities;

        AbilitiesData abilities;
        if (equipment_helper_
                .GetMergedAbilitiesDataForAbilityType(full_data, ability_type, &abilities, &error_setup_abilities))
        {
            abilities_component.SetAbilitiesData(abilities, ability_type);
        }

        if (!error_setup_abilities.empty())
        {
            LogErr(
                entity_id,
                "Failed to setup abilities for ability_type = {}, instance: {}. Error = {}",
                ability_type,
                instance,
                error_setup_abilities);
        }

        return error_setup_abilities.empty();
    };

    // Set Abilities
    setup_abilities(AbilityType::kAttack);
    setup_abilities(AbilityType::kOmega);
    setup_abilities(AbilityType::kInnate);

    // Add innate abilities from suites (maybe worth moving to a separate method)
    if (combat_unit_type_id.type == CombatUnitType::kRanger)
    {
        const auto& suit_id = full_data.GetEquippedSuitTypeID();
        if (suit_id.IsValid())
        {
            const auto suit_data = GetGameDataContainer().GetSuitData(suit_id);
            if (suit_data)
            {
                abilities_component.AddDataInnateAbilities(suit_data->innate_abilities);
            }
            else
            {
                LogErr("{} - Could not find suit data for this suit id: \"{}\"", method_name, suit_id);
            }
        }

        // const FixedPoint omega_damage_percentage =
        //     stats_component.GetBaseValueForType(StatType::kOmegaDamagePercentage);
        // const FixedPoint shield_gain_efficiency_percentage =
        //     stats_component.GetBaseValueForType(StatType::kShieldGainEfficiencyPercentage);
        // const FixedPoint health_gain_efficiency_percentage =
        //     stats_component.GetBaseValueForType(StatType::kHealthGainEfficiencyPercentage);
        // const FixedPoint energy_gain_efficiency_percentage =
        //     stats_component.GetBaseValueForType(StatType::kEnergyGainEfficiencyPercentage);
        // LogDebug(
        //     "{} - omega_damage_percentage = {}, shield_gain_efficiency_percentage = {}, "
        //     "health_gain_efficiency_percentage = {}, energy__gain_efficiency_percentage = {}",
        //     method_name,
        //     omega_damage_percentage,
        //     shield_gain_efficiency_percentage,
        //     health_gain_efficiency_percentage,
        //     energy_gain_efficiency_percentage);
    }

    if (GetLevelingConfig().enable_leveling_system)
    {
        GetLevelingHelper().SetBonusStats(*entity);
    }

    // Initializes the metered stats
    EntityHelper::InitMeteredStats(*this, *entity);

    // Refresh the synergy system
    if (refresh_synergies)
    {
        RefreshCombatSynergiesBeforeBattleStarted();
    }

    return true;
}

void World::InternalAddSystems()
{
    // Clear previous state
    systems_.clear();
    system_array_.fill(nullptr);

    AddSystem<DecisionSystem>();
    AddSystem<FocusSystem>();
    AddSystem<MovementSystem>();
    if (config_.battle_config.overload_config.enable_overload_system)
    {
        AddSystem<OverloadSystem>();
    }
    AddSystem<SynergySystem>();

    // Dashes must be handled before ability system, otherwise we can miss one tick of dash
    // movement (the next skill starts, fails targeting and interrupts the whole ability)
    AddSystem<DashSystem>();

    AddSystem<AbilitySystem>();
    AddSystem<EffectSystem>();
    AddSystem<SplashSystem>();
    AddSystem<ChainSystem>();
    AddSystem<ProjectileSystem>();

    // NOTE: Destruction system is before all entities that have the DeferredDestructionComponent
    AddSystem<DestructionSystem>();

    AddSystem<AuraSystem>();
    AddSystem<ZoneSystem>();
    AddSystem<BeamSystem>();
    AddSystem<AttachedEntitySystem>();
    AddSystem<EnergyGainSystem>();
    AddSystem<HealthGainSystem>();
    AddSystem<AttachedEffectsSystem>();
    AddSystem<StateSystem>();
    AddSystem<DisplacementSystem>();

    if (config_.enable_hyper_system)
    {
        AddSystem<HyperSystem>();
    }

    AddSystem<AugmentSystem>();
    AddSystem<ConsumableSystem>();
}

void World::InternalSubscribeToEvents()
{
    // Clear previous state
    events_subscribers_.fill({});

    SubscribeMethodToEvent<EventType::kFainted>(this, &Self::OnFainted);
    SubscribeMethodToEvent<EventType::kBattleFinished>(this, &Self::OnBattleFinished);
    SubscribeMethodToEvent<EventType::kMarkProjectileAsDestroyed>(this, &Self::OnMarkSpawnedEntityAsDestroyed);
    SubscribeMethodToEvent<EventType::kMarkZoneAsDestroyed>(this, &Self::OnMarkSpawnedEntityAsDestroyed);
    SubscribeMethodToEvent<EventType::kMarkBeamAsDestroyed>(this, &Self::OnMarkSpawnedEntityAsDestroyed);
    SubscribeMethodToEvent<EventType::kMarkChainAsDestroyed>(this, &Self::OnMarkSpawnedEntityAsDestroyed);
    SubscribeMethodToEvent<EventType::kMarkSplashAsDestroyed>(this, &Self::OnMarkSpawnedEntityAsDestroyed);
    SubscribeMethodToEvent<EventType::kMarkDashAsDestroyed>(this, &Self::OnMarkSpawnedEntityAsDestroyed);
    SubscribeMethodToEvent<EventType::kMarkAttachedEntityAsDestroyed>(this, &Self::OnMarkSpawnedEntityAsDestroyed);
    SubscribeMethodToEvent<EventType::kMarkAuraAsDestroyed>(this, &Self::OnMarkSpawnedEntityAsDestroyed);
    SubscribeMethodToEvent<EventType::kOnEffectApplied>(this, &Self::OnEffectApplied);
}

void World::BuildLogPrefixFor(const EntityID id, std::string* out_str) const
{
    auto string_inserter = std::back_inserter(*out_str);
    auto format = [&]<typename... Args>(fmt::format_string<Args...> format_args, Args&&... args)
    {
        fmt::format_to(string_inserter, format_args, std::forward<Args>(args)...);
    };

    // Does not exist, was it deleted?
    if (!HasEntity(id))
    {
        format("[MISSING ENTITY = {}]", id);
        return;
    }

    const auto& entity = GetByID(id);

    format("[entity = {}] [team = {}]", id, entity.GetTeam());

    // Build optional part
    if (EntityHelper::IsAProjectile(*this, id))
    {
        const auto& projectile_component = entity.Get<ProjectileComponent>();
        format(
            " [PROJECTILE sender = {}, receiver = {}]",
            projectile_component.GetSenderID(),
            projectile_component.GetReceiverID());
    }
    else if (EntityHelper::IsAZone(*this, id))
    {
        const auto& zone_component = entity.Get<ZoneComponent>();
        format(" [ZONE sender = {}]", zone_component.GetSenderID());
    }
    else if (EntityHelper::IsABeam(*this, id))
    {
        const auto& beam_component = entity.Get<BeamComponent>();
        format(" [BEAM sender = {}]", beam_component.GetSenderID());
    }
    else if (EntityHelper::IsAChain(*this, id))
    {
        const auto& chain_component = entity.Get<ChainComponent>();
        const auto& focus_component = entity.Get<FocusComponent>();
        format(
            " [CHAIN sender = {}, focus = {}, chain_number = {}]",
            chain_component.GetCombatUnitSenderID(),
            focus_component.GetFocusID(),
            chain_component.GetChainNumber());
    }
    else if (EntityHelper::IsASplash(*this, id))
    {
        const auto& splash_component = entity.Get<SplashComponent>();
        format(" [SPLASH sender = {}]", splash_component.GetSenderID());
    }
    else if (EntityHelper::IsAShield(*this, id))
    {
        const auto& shield_component = entity.Get<ShieldComponent>();
        format(
            " [SHIELD sender = {}, receiver = {}]",
            shield_component.GetSenderID(),
            shield_component.GetReceiverID());
    }
    else if (EntityHelper::IsAMark(*this, id))
    {
        const auto& mark_component = entity.Get<MarkComponent>();
        format(" [MARK sender = {}, receiver = {}]", mark_component.GetSenderID(), mark_component.GetReceiverID());
    }
    else if (EntityHelper::IsADash(*this, id))
    {
        const auto& dash_component = entity.Get<DashComponent>();
        format(" [DASH sender = {}, receiver = {}]", dash_component.GetSenderID(), dash_component.GetReceiverID());
    }
    else if (EntityHelper::IsACombatUnit(*this, id))
    {
        const auto& combat_unit_component = entity.Get<CombatUnitComponent>();
        const std::string type_id_string = fmt::format("{}", combat_unit_component.GetTypeID());
        if (!type_id_string.empty())
        {
            format(" [{}]", type_id_string);
        }
    }
    else if (EntityHelper::IsAnAura(*this, id))
    {
        const auto& aura_data = entity.Get<AuraComponent>().GetComponentData();
        format(" [AURA sender = {}, receiver = {}]", aura_data.aura_sender_id, aura_data.receiver_id);
    }
    else if (EntityHelper::IsASynergy(*this, id))
    {
        format(" [SYNERGY]");
    }
    else if (EntityHelper::IsADroneAugment(*this, id))
    {
        const auto& drone_component = entity.Get<DroneAugmentEntityComponent>();
        format(" [DRONE AUGMENT {{{}}}]", drone_component.GetDroneAugmentTypeID());
    }
}

void World::AddGlobalCollision(const Team team, const int global_collision_id, const EntityID collided_with)
{
    std::unordered_map<int, std::unordered_set<EntityID>>& team_global_collisions = global_collisions_.GetOrAdd(team);
    std::unordered_set<EntityID>& collisions_for_id = team_global_collisions[global_collision_id];
    collisions_for_id.emplace(collided_with);
}

bool World::HasGlobalCollision(const Team team, const int global_collision_id, const EntityID collided_with) const
{
    if (!global_collisions_.Contains(team))
    {
        return false;
    }
    const std::unordered_map<int, std::unordered_set<EntityID>>& team_global_collisions = global_collisions_.Get(team);

    const auto collisions_for_id_iterator = team_global_collisions.find(global_collision_id);
    if (collisions_for_id_iterator == team_global_collisions.end())
    {
        return false;
    }

    const std::unordered_set<EntityID>& collisions_for_id = collisions_for_id_iterator->second;
    return collisions_for_id.contains(collided_with);
}

void World::ResetGlobalCollision(const Team team, const int global_collision_id)
{
    std::unordered_map<int, std::unordered_set<EntityID>>& team_global_collisions = global_collisions_.GetOrAdd(team);
    std::unordered_set<EntityID>& collisions_for_id = team_global_collisions[global_collision_id];
    collisions_for_id.clear();
}

void World::CheckForFinishedGame()
{
    // Maps from
    // Key: Team
    // Value: count of active combat units count that belong to this team
    EnumMap<Team, size_t> owners_combat_units_map{};

    // Populate the map
    for (const auto& entity : GetAll())
    {
        // Populate map
        if (entity->IsActive() && EntityHelper::IsACombatUnit(*entity))
        {
            owners_combat_units_map.GetOrAdd(entity->GetTeam()) += 1;
        }
    }

    // TODO(vampy): Find a way to enforce this
    // Can we have more than 2 teams?
    if (owners_combat_units_map.Size() > 2)
    {
        LogDebug(
            "World::CheckForFinishedGame - owners_entities_map.entity_size() == {} SHOULD NEVER "
            "HAPPEN",
            owners_combat_units_map.Size());
        assert(false);
        return;
    }

    // One or Two teams
    Team winner_team = Team::kNone;
    if (owners_combat_units_map.Size() == 2 || owners_combat_units_map.Size() == 1)
    {
        // Found the team with non zero combat units still active
        for (const auto& [team, entities_count] : owners_combat_units_map)
        {
            // This team is dead
            if (entities_count == 0)
            {
                continue;
            }

            if (winner_team == Team::kNone)
            {
                // First winner
                winner_team = team;
            }
            else
            {
                // Found another one with non empty combat units, nobody wins :()
                winner_team = Team::kNone;
            }
        }
    }

    // Emit event
    if (winner_team != Team::kNone)
    {
        LogDebug("World::CheckForFinishedGame - winner_team = {}", winner_team);
        BuildAndEmitEvent<EventType::kBattleFinished>(winner_team);
    }
}

void World::CheckForOverloadTieBreaker()
{
    std::vector<Entity*> combat_units;
    combat_units.reserve(entities_.size());

    EnumMap<Team, int> teams_alive_counts;

    // Get only active combat units
    for (const auto& entity : entities_)
    {
        if (!entity->IsActive())
        {
            continue;
        }

        if (!EntityHelper::IsACombatUnit(*entity))
        {
            continue;
        }

        combat_units.push_back(entity.get());

        teams_alive_counts.GetOrAdd(entity->GetTeam()) += 1;
    }
    LogDebug("World::CheckForOverloadTieBreaker - combat_units.size() = {}", combat_units.size());

    // There are less than 2 combat units; something is wrong
    if (combat_units.size() < 2)
    {
        return;
    }

    auto max_least_negative_health_percents = FixedPoint::FromInt(std::numeric_limits<int>::min());
    size_t selected_index = 0;
    std::set<FixedPoint> damage_perecentage_combat_units;
    EnumSet<Team> owners_combat_units;
    std::vector<EntityID> combat_units_to_destroy;

    // Iterate through combat units and check which one will die in the current time step.
    // NOTE: that this will not actually apply the damage
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/236783470/Overload
    for (size_t index = 0; index < combat_units.size(); index++)
    {
        const auto& entity = *combat_units[index];
        const auto& stats_component = entity.Get<StatsComponent>();
        const StatsData entity_live_stats = GetLiveStats(entity);
        const FixedPoint max_health = entity_live_stats.Get(StatType::kMaxHealth);
        const FixedPoint current_health = stats_component.GetCurrentHealth();
        const FixedPoint current_overload_damage_percentage =
            GetOverloadDamagePercentage() + GetDeltaOverloadDamagePercentage();

        // What purest damage percentage will be applied this time step
        const Team team = entity.GetTeam();
        const FixedPoint overload_damage =
            Math::CalculateOverloadDamage(current_overload_damage_percentage, max_health, teams_alive_counts.Get(team));
        const FixedPoint health_after_damage = current_health - overload_damage;

        // The combat unit will not die this time step
        if (health_after_damage > 0_fp)
        {
            owners_combat_units.Add(team);

            // There are still 2 teams in the game; return
            if (owners_combat_units.Size() == 2)
            {
                return;
            }
        }
        else
        {
            // Set the health to 0 and mark for destruction
            combat_units_to_destroy.emplace_back(combat_units[index]->GetID());
        }

        const FixedPoint health_after_damage_percents = health_after_damage / max_health;
        damage_perecentage_combat_units.emplace(health_after_damage_percents);

        // The Combat Unit with biggest health will win
        if (health_after_damage_percents > max_least_negative_health_percents)
        {
            max_least_negative_health_percents = health_after_damage_percents;
            selected_index = index;
        }
    }

    // If there is only one team left; he is the winner
    Team winner_team = Team::kNone;
    if (owners_combat_units.Size() == 1)
    {
        // Get the first
        winner_team = *(owners_combat_units.begin());

        // Destroy all combat units with negative life
        OverloadDestroyCombatUnits(combat_units_to_destroy, kInvalidEntityID);

        BuildAndEmitEvent<EventType::kBattleFinished>(winner_team);
        return;
    }

    // Destroy all combat units except the selected one
    EntityID selected_unit = kInvalidEntityID;

    // All Combat Units have same health, last order Combat Unit will be the winner
    // TODO (Sergiu):: In case we add random order; we need to keep track of last combat unit that performs action
    // Right now it's vector order so we take last element
    if (damage_perecentage_combat_units.size() == 1)
    {
        selected_unit = combat_units.back()->GetID();
        winner_team = GetByID(selected_unit).GetTeam();
    }
    else
    {
        // Least negative health
        selected_unit = combat_units[selected_index]->GetID();
        winner_team = GetByID(selected_unit).GetTeam();
    }

    // Destroy all combat units except selected_unit
    OverloadDestroyCombatUnits(combat_units_to_destroy, selected_unit);
    BuildAndEmitEvent<EventType::kBattleFinished>(winner_team);
}

void World::OverloadDestroyCombatUnits(
    const std::vector<EntityID>& combat_units_to_destroy,
    const EntityID ignored_entity)
{
    for (const EntityID entity_id_to_faint : combat_units_to_destroy)
    {
        // Ignore this entity
        if (entity_id_to_faint == ignored_entity)
        {
            continue;
        }

        // Reduce health to 0
        EntityHelper::Kill(GetByID(entity_id_to_faint));

        event_data::Fainted event_data;
        event_data.entity_id = entity_id_to_faint;
        event_data.vanquisher_id = entity_id_to_faint;
        event_data.source_context.Add(SourceContextType::kOverload);
        event_data.death_reason = DeathReasonType::kOverload;
        EmitEvent<EventType::kFainted>(event_data);
    }
}

void World::OnMarkSpawnedEntityAsDestroyed(const event_data::Marked& data)
{
    DestroySpawnedEntity(data.entity_id);
}

void World::OnFainted(const event_data::Fainted& fainted_data)
{
    LogDebug(fainted_data.entity_id, "World::OnEventFainted");

    // Add to history
    vanquisher_history_map_[fainted_data.entity_id] = GetCombatUnitParentID(fainted_data.vanquisher_id);
    entities_fainted_history_map_[fainted_data.entity_id] = time_step_counter_;

    CheckForFinishedGame();
}

void World::OnEffectApplied(const event_data::Effect& event_data)
{
    const EffectData& data = event_data.data;

    // Only interested in detrimental effects
    if (!data.IsTypeDetrimental())
    {
        return;
    }

    const EntityID sender_id = event_data.sender_id;
    const EntityID receiver_id = event_data.receiver_id;
    if (!EntityHelper::IsACombatUnit(*this, receiver_id))
    {
        return;
    }

    // Get the true combat unit sender
    const EntityID sender_combat_unit_id = GetCombatUnitParentID(sender_id);

    // Get or Create the senders vector
    std::vector<EntityID>& senders_vector = detrimental_effects_history_map_[receiver_id];

    // NOTE: We use a vector for the senders because the order matters, using an std::unordered_set is not guaranteed to
    // give you the same order on every platform. And the vector.size() <= 20, so iteration is super fast.
    if (!VectorHelper::ContainsValue(senders_vector, sender_combat_unit_id))
    {
        senders_vector.push_back(sender_combat_unit_id);
    }
}

void World::OnBattleFinished(const event_data::BattleFinished& event_data)
{
    // Already finished
    if (is_battle_finished_)
    {
        LogWarn("World::OnBattleFinished - Was already called");
        return;
    }

    is_battle_finished_ = true;

    // Destroy all non combat unit entities
    LogDebug("World::OnBattleFinished - Destroying all non combat unit entities");
    for (const auto& entity : GetAll())
    {
        // Ignore combat units and attachable entities
        const EntityID id = entity->GetID();
        if (EntityHelper::IsACombatUnit(*this, id) || EntityHelper::IsAttachable(*this, id))
        {
            continue;
        }

        DestroySpawnedEntity(id);
    }

    // Tell attached effects system to deactivate all effects
    LogDebug("World::OnBattleFinished - Removing all attached effects");
    auto& attached_effects_system = GetSystem<AttachedEffectsSystem>();
    attached_effects_system.RemoveAllEffects();

    // Make a new battle result
    battle_result_ = {};
    battle_result_.winning_team = event_data.winning_team;
    battle_result_.duration_time_steps = time_step_counter_;
    battle_result_.combat_units_end_state = GetCombatUnitsState();

    // NOTE: The entities still exist in the array, they are just marked as deactivated
    LogDebug("World::OnBattleFinished - winner_team = {}", event_data.winning_team);
}

std::vector<BattleEntityResult> World::GetCombatUnitsState() const
{
    std::vector<BattleEntityResult> combat_units_end_state;
    for (const auto& entity : GetAll())
    {
        // Only interested in combat units
        if (!EntityHelper::IsACombatUnit(*entity))
        {
            continue;
        }

        BattleEntityResult entity_state;
        entity_state.unique_id = entity->Get<CombatUnitComponent>().GetUniqueID();
        entity_state.entity_id = entity->GetID();
        entity_state.team = entity->GetTeam();

        const auto& position_component = entity->Get<PositionComponent>();
        entity_state.position = position_component.GetPosition();

        const auto& stats_component = entity->Get<StatsComponent>();
        entity_state.max_health = stats_component.GetBaseValueForType(StatType::kMaxHealth);
        entity_state.current_health = stats_component.GetCurrentHealth();
        entity_state.current_energy = stats_component.GetCurrentEnergy();
        entity_state.fainted_time_step = GetCombatUnitFaintedTimeStep(entity_state.entity_id);
        entity_state.history_data = stats_component.GetHistoryData();

        combat_units_end_state.push_back(entity_state);
    }

    return combat_units_end_state;
}

const WorldEffectsConfig& World::GetWorldEffectsConfig() const
{
    return GetGameDataContainer().GetWorldEffectsConfig();
}

void World::EraseEntity(const EntityID id)
{
    if (!HasEntity(id))
    {
        return;
    }

    const size_t index = entity_id_to_index_map_.at(id);
    std::string prefix;
    BuildLogPrefixFor(id, &prefix);
    LogDebug("World::EraseEntity - Entity = {}, index = {}, id = {}", prefix, index, id);

    // Sanity checks
    assert(index < entities_.size());
    assert(entities_[index]->GetID() == id);

    // Delete from the vector
    VectorHelper::EraseIndex(entities_, index);

    // Delete from the map
    entity_id_to_index_map_.erase(id);
}

void World::UpdateEntityIDToIndexMap()
{
    entity_id_to_index_map_.clear();

    // Do one loop over all entities to update the values.
    const size_t size = entities_.size();
    for (size_t entity_index = 0; entity_index < size; entity_index++)
    {
        const auto& entity = entities_[entity_index];
        entity_id_to_index_map_[entity->GetID()] = entity_index;
    }
}

const GameDataContainer& World::GetGameDataContainer() const
{
    assert(game_data_container_);
    return *game_data_container_;
}

}  // namespace simulation
