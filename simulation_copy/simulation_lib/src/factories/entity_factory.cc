#include "factories/entity_factory.h"

#include <memory>

#include "components/abilities_component.h"
#include "components/attached_effects_component.h"
#include "components/attached_entity_component.h"
#include "components/augment_component.h"
#include "components/aura_component.h"
#include "components/beam_component.h"
#include "components/chain_component.h"
#include "components/combat_synergy_component.h"
#include "components/combat_unit_component.h"
#include "components/consumable_component.h"
#include "components/dash_component.h"
#include "components/decision_component.h"
#include "components/deferred_destruction_component.h"
#include "components/displacement_component.h"
#include "components/drone_augment_component.h"
#include "components/duration_component.h"
#include "components/filtering_component.h"
#include "components/focus_component.h"
#include "components/mark_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/projectile_component.h"
#include "components/shield_component.h"
#include "components/splash_component.h"
#include "components/state_component.h"
#include "components/stats_component.h"
#include "components/zone_component.h"
#include "data/aura_data.hpp"
#include "data/combat_unit_data.h"
#include "data/constants.h"
#include "data/containers/game_data_container.h"
#include "data/stats_data.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "systems/synergy_system.h"
#include "utility/entity_helper.h"
#include "utility/grid_helper.h"
#include "utility/targeting_helper.h"

namespace simulation
{
Entity* EntityFactory::SpawnCombatUnit(
    World& world,
    const FullCombatUnitData& full_data,
    const EntityID parent_id,
    std::string* out_error_message)
{
    assert(world.GetGridConfig().IsInMapRectangleLimits(full_data.instance.position));
    const CombatUnitTypeID& type_id = full_data.data.type_id;

    const int radius_units = world.GetPreferredUnitRadius(full_data);

    // Is valid position?
    static constexpr bool is_taking_space = true;
    const GridHelper& grid_helper = world.GetGridHelper();
    if (!grid_helper.IsValidHexagonPosition(
            {.center = full_data.instance.position, .radius_units = radius_units, .is_taking_space = is_taking_space},
            out_error_message))
    {
        if (out_error_message)
        {
            *out_error_message = fmt::format(
                "type_id = {}, team = {}, {}",
                full_data.data.type_id,
                full_data.instance.team,
                *out_error_message);
        }
        return nullptr;
    }

    // Is non empty id?
    const std::string& unique_id = full_data.instance.id;
    if (unique_id.empty())
    {
        if (out_error_message)
        {
            *out_error_message = fmt::format(
                "type_id = {}, team = {} - Missing unique id",
                full_data.data.type_id,
                full_data.instance.team);
        }

        return nullptr;
    }

    // Duplicate id?
    if (world.HasCombatUnitUniqueID(unique_id))
    {
        if (out_error_message)
        {
            const auto& duplicate_entity_ptr = world.GetCombatUnitByUniqueIDPtr(unique_id);
            *out_error_message = fmt::format(
                "type_id = {}, team = {}, unique_id = {} is already taken by duplicate_type_id = {}",
                full_data.data.type_id,
                full_data.instance.team,
                unique_id,
                duplicate_entity_ptr->Get<CombatUnitComponent>().GetTypeID());
        }

        return nullptr;
    }

    // Combat unit must have valid team
    if (full_data.instance.team == Team::kNone)
    {
        if (out_error_message)
        {
            *out_error_message = fmt::format(
                "Can't spawn with team none. type_id = {}, unique_id = {}",
                full_data.data.type_id,
                full_data.instance.id);
        }

        return nullptr;
    }

    if (full_data.data.type_id.type == CombatUnitType::kCrate)
    {
        if (out_error_message)
        {
            *out_error_message = fmt::format(
                "Can't spawn crates inside the simulation. type_id = {}, unique_id = {}",
                full_data.data.type_id,
                full_data.instance.id);
        }

        return nullptr;
    }

    // Create an entity
    auto& entity = world.AddEntity(full_data.instance.team, parent_id);

    // Add components for CombatUnit
    auto& position_component = entity.Add<PositionComponent>();
    entity.Add<StatsComponent>();
    entity.Add<FocusComponent>();
    entity.Add<AbilitiesComponent>();
    entity.Add<MovementComponent>();
    entity.Add<CombatSynergyComponent>();
    entity.Add<CombatUnitComponent>();
    entity.Add<StateComponent>();
    entity.Add<DecisionComponent>();
    entity.Add<AttachedEffectsComponent>();
    entity.Add<AttachedEntityComponent>();
    entity.Add<DisplacementComponent>();
    entity.Add<AugmentComponent>();
    entity.Add<ConsumableComponent>();

    // Set position to spawn location and size based on type data
    position_component.InitFromData(full_data.instance.position, radius_units, is_taking_space);

    // Keep track of how many entities did the parent (combat unit) spawn
    if (world.HasEntity(parent_id))
    {
        const Entity& parent_entity = world.GetByID(parent_id);
        if (EntityHelper::IsACombatUnit(parent_entity))
        {
            parent_entity.Get<CombatUnitComponent>().IncrementChildrenSpawnedCount();
        }
    }

    // Update the rest from the data
    static constexpr bool refresh_synergies = false;
    std::string update_combat_unit_data_error;
    if (!world.UpdateCombatUnitFromData(entity.GetID(), full_data, refresh_synergies, &update_combat_unit_data_error))
    {
        // This should never happen
        if (out_error_message)
        {
            *out_error_message = fmt::format(
                "type_id: {}, team = {} - UpdateCombatUnitFromData failed with error: {}",
                full_data.data.type_id,
                full_data.instance.team,
                update_combat_unit_data_error);
        }
        return nullptr;
    }

    world.LogDebug(
        "EntityFactory::SpawnCombatUnit - entity = {}, team = {}, type_id: {}, position = {}, radius_units = "
        "{}",
        entity.GetID(),
        full_data.instance.team,
        type_id,
        full_data.instance.position,
        radius_units);

    // Add to Synergy
    // TODO(vampy): Make the synergy system listen to events instead of calling it manually here
    world.GetSystem<SynergySystem>().AddEntity(entity);

    // For regular combat units encounter mods will be applied before the first time step
    // Here we apply encounter mods for units that spawn in the middle of combat (pets).
    if (world.IsBattleStarted())
    {
        world.ApplyEncounterModsToCombatUnit(entity);
    }

    world.BuildAndEmitEvent<EventType::kCombatUnitCreated>(
        entity.GetID(),
        position_component.GetPosition(),
        type_id.type,
        parent_id);

    // Return the new CombatUnit
    return &entity;
}

std::vector<Entity*> EntityFactory::SpawnChainPropagation(World& world, const Team team, const ChainData& chain_data)
{
    static constexpr std::string_view method_name = "EntityFactory::SpawnChainPropagation";

    const EntityID sender_id = chain_data.sender_id;
    if (!world.HasEntity(sender_id))
    {
        assert(false);
        world.LogErr("{} - sender_id = {} does NOT exist. ", method_name, sender_id);
        return {};
    }

    // Create the current focus of each chain skill data
    ChainData data = chain_data;

    data.chain_bounce_max_distance_units = world.GetRangeInfinite().AsInt();

    // Find the combat unit that sent this chain
    if (!EntityHelper::IsACombatUnit(world, data.combat_unit_sender_id))
    {
        // Find the combat unit that spawned this
        data.combat_unit_sender_id = world.GetCombatUnitParentID(data.sender_id);

        // Check if valid
        if (!EntityHelper::IsACombatUnit(world, data.combat_unit_sender_id))
        {
            world.LogErr(
                "{} - combat_unit_sender_id = {} is not a CombatUnit.",
                method_name,
                data.combat_unit_sender_id);
            return {};
        }
    }

    // Check if sender is alive
    if (!CanSpawnFromCombatUnit(method_name, world, data.combat_unit_sender_id))
    {
        world.LogErr("{} - combat_unit_sender_id = {} is not alive.", method_name, data.combat_unit_sender_id);
        return {};
    }

    // Ignore the receiver of the first propagation
    std::unordered_set<EntityID> ignored_targets;
    if (data.ignore_first_propagation_receiver)
    {
        // Used for the chains
        data.old_target_entities.insert(data.first_propagation_receiver_id);

        // Used for the first targeting
        ignored_targets.insert(data.first_propagation_receiver_id);
    }

    // Find Targets of the propagation skill
    const TargetingHelper& targeting_helper = world.GetTargetingHelper();
    auto skill_data = SkillData::Create();
    skill_data->targeting.type = SkillTargetingType::kDistanceCheck;
    skill_data->is_critical = data.is_critical;
    skill_data->effect_package = data.propagation_effect_package;
    skill_data->deployment.type = SkillDeploymentType::kDirect;
    skill_data->targeting.group = data.targeting_group;
    // TODO(konstantin): maybe we want to allow to set it externally?
    skill_data->targeting.num = 1;
    // this is currently not configurable for chains and true by default
    skill_data->targeting.lowest = true;
    skill_data->deployment.guidance = data.deployment_guidance;
    skill_data->deployment.pre_deployment_delay_percentage = data.pre_deployment_delay_percentage;
    skill_data->targeting.guidance = data.targeting_guidance;

    TargetingHelper::SkillTargetFindResult find_result;
    targeting_helper.GetEntitiesOfSkillTarget(sender_id, nullptr, *skill_data, ignored_targets, &find_result);

    // Set the new sender id
    data.sender_id = find_result.true_sender_id;

    world.LogDebug(data.sender_id, "{} - receiver_ids = [{}]", method_name, fmt::join(find_result.receiver_ids, ", "));

    // Spawn a chain with the current
    std::vector<Entity*> chain_entities;
    for (const EntityID receiver_id : find_result.receiver_ids)
    {
        auto* chain_entity = SpawnChainToReceiver(world, team, skill_data, data, receiver_id);
        chain_entities.push_back(chain_entity);
    }

    return chain_entities;
}

Entity* EntityFactory::SpawnChainToReceiver(
    World& world,
    const Team team,
    std::shared_ptr<SkillData> skill_data,
    const ChainData& chain_data,
    const EntityID receiver_id)
{
    static constexpr std::string_view method_name = "EntityFactory::SpawnChainToReceiver";
    EntityID combat_unit_sender_id = kInvalidEntityID;
    if (!CanSpawnFromSenderToReceiver(method_name, world, chain_data.sender_id, receiver_id, &combat_unit_sender_id))
    {
        return nullptr;
    }
    assert(combat_unit_sender_id == chain_data.combat_unit_sender_id);

    // Create an entity
    // NOTE: We use the combat unit as the direct parent, not the chain parent
    auto& chain_entity = world.AddEntity(team, chain_data.combat_unit_sender_id);

    // Add components for chain
    auto& chain_component = chain_entity.Add<ChainComponent>();
    auto& focus_component = chain_entity.Add<FocusComponent>();
    auto& position_component = chain_entity.Add<PositionComponent>();
    auto& stats_component = chain_entity.Add<StatsComponent>();
    auto& abilities_component = chain_entity.Add<AbilitiesComponent>();
    auto& filtering_component = chain_entity.Add<FilteringComponent>();
    chain_entity.Add<DecisionComponent>();

    // Init the chain data
    chain_component.SetComponentData(chain_data);

    focus_component.SetSelectionType(FocusComponent::SelectionType::kPredefined);
    focus_component.SetRefocusType(FocusComponent::RefocusType::kNever);
    focus_component.SetFocus(world.GetByIDPtr(receiver_id));

    const auto& receiver_entity = world.GetByID(receiver_id);

    // Set position to spawn location
    position_component.InitFromData(receiver_entity.Get<PositionComponent>().GetPosition(), 0, false);

    // Set filtering data
    filtering_component.SetOnlyNewTargets(chain_data.only_new_targets);
    filtering_component.SetPrioritizeNewTargets(chain_data.prioritize_new_targets);
    filtering_component.SetOldTargetEntities(chain_data.old_target_entities);

    // Copy the stats of the entity that fired this chain
    // Or the chain that fired this chain.
    StatsData stats_data = GetStatsDataForSpawnedEntity(world, chain_data.combat_unit_sender_id);
    // Infinite range
    stats_data.Set(StatType::kAttackRangeUnits, world.GetRangeInfinite());

    // Copy to chain
    stats_component.SetTemplateStats(stats_data);

    // Create the attack abilities that ignores the propagation skill targeting and deployment
    // because it is from an effect package so we do directly
    AbilitiesData attack_abilities{};
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Store parent stats
    UpdateCombatUnitParentLiveStats(world, chain_entity);

    // Create the skill
    {
        AbilityData& ability = attack_abilities.AddAbility();
        ability.name = chain_data.source_context.ability_name;
        ability.ignore_attack_speed = true;

        // How long does this chain last
        ability.total_duration_ms = chain_data.chain_delay_ms;

        // A chain entity for one target, so the ability can only activate once
        ability.is_use_once = true;

        // Use the chain component current skill data
        ability.AddSkill(std::move(skill_data));

        ability.source_context = chain_data.source_context;
    }

    // Set on the component
    abilities_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);

    // Emit event
    world.LogDebug(
        chain_data.sender_id,
        "{} - chain = {} [chain_number = {}, chain_delay_ms = {}, position = {}]",
        method_name,
        chain_entity.GetID(),
        chain_data.chain_number,
        chain_data.chain_delay_ms,
        position_component.GetPosition());

    // Get Position of receiver
    const auto& receiver_position_component = receiver_entity.Get<PositionComponent>();

    // Get Position of original combat unit sender
    const auto& combat_unit_sender_entity = world.GetByID(chain_component.GetCombatUnitSenderID());
    const auto& combat_unit_sender_position_component = combat_unit_sender_entity.Get<PositionComponent>();

    // Bounce position near chain target on path from sender
    // Use original combat unit sender position for size needed
    const GridHelper& grid_helper = world.GetGridHelper();
    GridHelper::BuildObstaclesParameters build_parameters;
    build_parameters.radius_needed = combat_unit_sender_position_component.GetRadius();
    build_parameters.source = &combat_unit_sender_entity;
    grid_helper.BuildObstacles(build_parameters);
    const auto& actual_receiver_position = receiver_position_component.GetPosition();

    // Source and target are the same so we can expand from this point
    const HexGridPosition chain_bounce_position = grid_helper.GetOpenPositionNearLocationOnPath(
        actual_receiver_position,
        actual_receiver_position,
        combat_unit_sender_position_component.GetRadius(),
        chain_data.chain_bounce_max_distance_units);

    // Ensure bounce position is valid
    static constexpr bool is_taking_space = false;
    if (grid_helper.IsValidHexagonPosition(
            {.center = chain_bounce_position, .radius_units = 0, .is_taking_space = is_taking_space}))
    {
        chain_component.SetBouncePosition(chain_bounce_position);
    }
    else
    {
        // Fall back to target position
        chain_component.SetBouncePosition(actual_receiver_position);
    }

    event_data::ChainCreated event_data;
    event_data.entity_id = chain_entity.GetID();
    event_data.sender_id = chain_data.sender_id;
    event_data.combat_unit_sender_id = chain_data.combat_unit_sender_id;
    event_data.receiver_id = receiver_id;
    event_data.chain_position = position_component.GetPosition();
    event_data.receiver_position = actual_receiver_position;
    event_data.chain_bounce_position = chain_component.GetBouncePosition();
    event_data.delay_ms = chain_data.chain_delay_ms;
    event_data.source_context = chain_data.source_context;
    event_data.is_critical = chain_data.is_critical;

    world.EmitEvent<EventType::kChainCreated>(event_data);

    // Return the new chain
    return &chain_entity;
}

Entity* EntityFactory::SpawnSplash(
    World& world,
    const Team team,
    const EntityID receiver_id,
    const HexGridPosition& position,
    const SplashData& splash_data)
{
    EntityID combat_unit_sender_id = kInvalidEntityID;
    if (!CanSpawnFromSender(
            "EntityFactory::SpawnSplash",
            world,
            position,
            splash_data.sender_id,
            &combat_unit_sender_id))
    {
        return nullptr;
    }

    // Create an entity
    auto& splash_entity = world.AddEntity(team, combat_unit_sender_id);

    // Add components for splash
    auto& splash_component = splash_entity.Add<SplashComponent>();

    // Focus is ignored because ths splash skill will always target the the splash entity
    splash_entity.Add<FocusComponent>();
    auto& position_component = splash_entity.Add<PositionComponent>();
    auto& stats_component = splash_entity.Add<StatsComponent>();
    auto& abilities_component = splash_entity.Add<AbilitiesComponent>();
    auto& filtering_component = splash_entity.Add<FilteringComponent>();
    splash_entity.Add<DecisionComponent>();

    splash_component.SetComponentData(splash_data);
    if (splash_data.ignore_first_propagation_receiver)
    {
        filtering_component.AddIgnoredEntityID(receiver_id);
    }

    // Set position to spawn location
    position_component.InitFromData(position, 0, false);

    // Copy the stats of the combat unit that fired this splash
    // Or the splash that fired this splash.
    StatsData stats_data = GetStatsDataForSpawnedEntity(world, combat_unit_sender_id);

    // Infinite range
    stats_data.Set(StatType::kAttackRangeUnits, world.GetRangeInfinite());

    // Copy to splash
    stats_component.SetTemplateStats(stats_data);

    // Store parent stats
    UpdateCombatUnitParentLiveStats(world, splash_entity);

    // Create the attack abilities that ignores the propagation skill targeting and deployment
    // because it is from an effect package so we do directly
    AbilitiesData attack_abilities{};
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Set on the component
    auto new_context = splash_data.source_context;
    new_context.Add(SourceContextType::kSplash);

    // Create the skill
    {
        auto& ability = attack_abilities.AddAbility();
        ability.name = splash_data.source_context.ability_name;
        ability.ignore_attack_speed = true;
        auto skill = SkillData::Create();
        skill->SetPropagationSkillSplashDefaults();
        skill->is_critical = splash_data.is_critical;
        skill->effect_package = splash_data.propagation_effect_package;
        skill->zone.radius_units = splash_data.splash_radius_units;
        skill->targeting.guidance = splash_data.targeting_guidance;
        skill->deployment.guidance = splash_data.deployment_guidance;

        // Use the splash component skill
        ability.AddSkill(skill);

        ability.source_context = new_context;
    }
    abilities_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);

    // Emit event
    world.LogDebug(
        splash_data.sender_id,
        "EntityFactory::SpawnSplash - splash = {} [position = {}]",
        splash_entity.GetID(),
        position_component.GetPosition());

    event_data::SplashCreated event_data;
    event_data.entity_id = splash_entity.GetID();
    event_data.sender_id = splash_data.sender_id;
    event_data.combat_unit_sender_id = combat_unit_sender_id;
    event_data.position = position;
    event_data.source_context = splash_data.source_context;
    event_data.is_critical = splash_data.is_critical;

    world.EmitEvent<EventType::kSplashCreated>(event_data);

    // Return the new splash
    return &splash_entity;
}

IVector2D EntityFactory::FindRayRectIntersection(
    const IVector2D ray_origin,
    const IVector2D ray_direction,
    const IVector2D rect_min_point,
    const IVector2D rect_max_point)
{
    constexpr uint8_t left = 1 << 0;
    constexpr uint8_t right = 1 << 1;
    constexpr uint8_t top = 1 << 2;
    constexpr uint8_t bottom = 1 << 3;
    uint8_t control = 0;

    if (ray_direction.x > 0)
    {
        control |= right;
    }
    else if (ray_direction.x < 0)
    {
        control |= left;
    }

    if (ray_direction.y > 0)
    {
        control |= top;
    }
    else if (ray_direction.y < 0)
    {
        control |= bottom;
    }

    const auto corner = [&](const int v_line_x, const int h_line_y)
    {
        const int x = ray_origin.x + (ray_direction.x * (h_line_y - ray_origin.y)) / ray_direction.y;
        if (x >= rect_min_point.x && x <= rect_max_point.x)
        {
            return IVector2D(x, h_line_y);
        }

        const int y = ray_origin.y + (ray_direction.y * (v_line_x - ray_origin.x)) / ray_direction.x;
        return IVector2D(v_line_x, y);
    };

    switch (control)
    {
    case top:
        return IVector2D(ray_origin.x, rect_max_point.y);
    case bottom:
        return IVector2D(ray_origin.x, rect_min_point.y);
    case left:
        return IVector2D(rect_min_point.x, ray_origin.y);
    case right:
        return IVector2D(rect_max_point.x, ray_origin.y);
    case top | right:
        return corner(rect_max_point.x, rect_max_point.y);
    case top | left:
        return corner(rect_min_point.x, rect_max_point.y);
    case bottom | right:
        return corner(rect_max_point.x, rect_min_point.y);
    case bottom | left:
        return corner(rect_min_point.x, rect_min_point.y);
    default:
        // May only happen if s == r, which is invalid input
        assert(false);
        return {};
    }
}

Entity* EntityFactory::SpawnProjectile(
    World& world,
    const Team team,
    const HexGridPosition& position,
    const HexGridPosition& target,
    const ProjectileData& projectile_data)
{
    EntityID combat_unit_sender_id = kInvalidEntityID;
    if (!CanSpawnFromSender(
            "EntityFactory::SpawnProjectile",
            world,
            position,
            projectile_data.sender_id,
            &combat_unit_sender_id))
    {
        return nullptr;
    }

    // Create an entity
    auto& projectile_entity = world.AddEntity(team, combat_unit_sender_id);

    // Add components for projectile
    auto& projectile_component = projectile_entity.Add<ProjectileComponent>();
    auto& focus_component = projectile_entity.Add<FocusComponent>();
    auto& position_component = projectile_entity.Add<PositionComponent>();
    auto& movement_component = projectile_entity.Add<MovementComponent>();
    auto& stats_component = projectile_entity.Add<StatsComponent>();
    auto& abilities_component = projectile_entity.Add<AbilitiesComponent>();
    projectile_entity.Add<DecisionComponent>();
    projectile_entity.Add<FilteringComponent>();

    // Init the projectile data
    projectile_component.SetComponentData(projectile_data);

    // A projectile should never change target
    focus_component.SetSelectionType(FocusComponent::SelectionType::kPredefined);
    focus_component.SetRefocusType(FocusComponent::RefocusType::kNever);

    if (world.HasEntity(projectile_data.receiver_id) && !projectile_component.ContinueAfterTarget())
    {
        focus_component.SetFocus(world.GetByIDPtr(projectile_data.receiver_id));
    }

    // Set position to spawn location
    position_component.InitFromData(position, projectile_data.radius_units, false);

    // Follow to the target
    if (projectile_data.is_homing)
    {
        movement_component.SetMovementType(MovementComponent::MovementType::kDirect);
    }
    else
    {
        movement_component.SetMovementType(MovementComponent::MovementType::kDirectPosition);

        if (projectile_data.continue_after_target)
        {
            // Determine position on the grid behind the target
            const auto& grid_config = world.GetGridConfig();
            const auto world_source = world.ToWorldPosition(position);
            const auto world_target = world.ToWorldPosition(target);
            const auto world_pos = FindRayRectIntersection(
                world_source,
                world_target - world_source,
                world.ToWorldPosition(grid_config.GetMinHexGridPosition()),
                world.ToWorldPosition(grid_config.GetMaxHexGridPosition()));
            movement_component.SetMovementTargetPosition(world.FromWorldPosition(world_pos));
        }
        else
        {
            // Set the target position, if possible
            movement_component.SetMovementTargetPosition(target);
        }
    }

    // Set the speed of the projectile
    movement_component.SetMovementSpeedSubUnits(FixedPoint::FromInt(projectile_data.move_speed_sub_units));

    // To make the system more flexible we make the projectile use the ability system
    // We carry the payload of the skill into our own data structures

    // Copy the stats of the combat unit that fired this projectile
    const StatsData stats_data = GetStatsDataForSpawnedEntity(world, combat_unit_sender_id);

    // Copy to projectile
    stats_component.SetTemplateStats(stats_data);

    // Store parent stats
    UpdateCombatUnitParentLiveStats(world, projectile_entity);

    // Create the attack abilities
    AbilitiesData attack_abilities{};

    // Ability timers should be at zero
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Create the skill
    auto& ability = attack_abilities.AddAbility();
    ability.name = projectile_data.source_context.ability_name;
    ability.source_context = projectile_data.source_context;

    // Ignore Attack ability speed
    ability.ignore_attack_speed = true;

    // Set the payload to be direct and for the current target
    auto& skill = ability.AddSkill();
    skill.targeting.type = SkillTargetingType::kCurrentFocus;
    skill.deployment.type = SkillDeploymentType::kDirect;
    skill.is_critical = projectile_data.is_critical;
    skill.deployment.guidance = projectile_data.deployment_guidance;

    // Copy the payload from the projectile data
    if (projectile_data.skill_data)
    {
        skill.effect_package = projectile_data.skill_data->effect_package;
    }

    // Set on the component
    abilities_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);

    // Emit event
    world.LogDebug(
        projectile_data.sender_id,
        "EntityFactory::SpawnProjectile - projectile = {}, receiver = {}",
        projectile_entity.GetID(),
        projectile_data.receiver_id);

    event_data::ProjectileCreated event_data;
    event_data.entity_id = projectile_entity.GetID();
    event_data.sender_id = projectile_data.sender_id;
    event_data.combat_unit_sender_id = combat_unit_sender_id;
    event_data.receiver_id = projectile_data.receiver_id;
    event_data.position = position_component.GetPosition();
    event_data.is_blockable = projectile_data.is_blockable;
    event_data.apply_to_all = projectile_data.apply_to_all;
    event_data.is_homing = projectile_data.is_homing;
    event_data.is_critical = projectile_data.is_critical;
    event_data.receiver_position = target;
    event_data.move_speed_sub_units = projectile_data.move_speed_sub_units;
    event_data.source_context = projectile_data.source_context;

    world.EmitEvent<EventType::kProjectileCreated>(event_data);

    // Return the new projectile
    return &projectile_entity;
}

int EntityFactory::CalculateZoneSpawnDirection(
    const World& world,
    const ZoneEffectShape zone_shape,
    const HexGridPosition& zone_spawn_position,
    const Entity& combat_unit_sender)
{
    if (zone_shape == ZoneEffectShape::kHexagon)
    {
        return 0;
    }

    const GridHelper& grid_helper = world.GetGridHelper();
    // Calculate zone spawn direction as direction from sender position to zone spawn position
    // NOTE: for non movable zone
    const PositionComponent& sender_position_component = combat_unit_sender.Get<PositionComponent>();
    const HexGridPosition sender_position = sender_position_component.GetPosition();
    if (sender_position != zone_spawn_position)
    {
        return world.ToWorldPosition(sender_position).AngleToPosition(world.ToWorldPosition(zone_spawn_position));
    }

    // But if sender spawns zone on self, use direction from sender position to current focus position
    if (combat_unit_sender.Has<FocusComponent>())
    {
        FocusComponent& sender_focus_component = combat_unit_sender.Get<FocusComponent>();
        if (sender_focus_component.GetFocusID() != kInvalidEntityID)
        {
            return grid_helper.GetAngle360Between(combat_unit_sender.GetID(), sender_focus_component.GetFocusID());
        }

        // The last attempt is to try previous focus (as current focus may be reset before zone skill cast)
        if (sender_focus_component.GetPreviousFocusID() != kInvalidEntityID)
        {
            return grid_helper.GetAngle360Between(
                combat_unit_sender.GetID(),
                sender_focus_component.GetPreviousFocusID());
        }
    }

    world.LogErr(
        "EntityFactory::SpawnZone - {} spawns a zone with shape {} which requires some direction but we "
        "can't determine it.",
        combat_unit_sender.GetID(),
        zone_shape);

    return 0;
}

Entity* EntityFactory::SpawnZone(
    World& world,
    const Team team,
    const HexGridPosition& arg_zone_target_position,
    ZoneData& zone_data)
{
    EntityID combat_unit_sender_id = kInvalidEntityID;
    if (!CanSpawnFromSender(
            "EntityFactory::SpawnZone",
            world,
            arg_zone_target_position,
            zone_data.sender_id,
            &combat_unit_sender_id))
    {
        return nullptr;
    }

    const auto& grid_helper = world.GetGridHelper();
    const auto& grid_config = world.GetGridConfig();

    HexGridPosition zone_target_position = arg_zone_target_position;
    if (zone_data.skill_data->zone.predefined_target_position != PredefinedGridPosition::kNone)
    {
        zone_target_position =
            grid_config.ResolvePredefinedPosition(team, zone_data.skill_data->zone.predefined_target_position);
    }

    // Create an entity
    auto& zone_entity = world.AddEntity(team, combat_unit_sender_id);
    const auto& combat_unit_sender = world.GetByID(combat_unit_sender_id);

    // Add components for zone
    auto& zone_component = zone_entity.Add<ZoneComponent>();
    auto& focus_component = zone_entity.Add<FocusComponent>();
    auto& position_component = zone_entity.Add<PositionComponent>();
    auto& stats_component = zone_entity.Add<StatsComponent>();
    auto& abilities_component = zone_entity.Add<AbilitiesComponent>();
    auto& filtering_component = zone_entity.Add<FilteringComponent>();
    zone_entity.Add<DecisionComponent>();
    auto& duration_component = zone_entity.Add<DurationComponent>();
    zone_entity.Add<DeferredDestructionComponent>();

    zone_component.SetSenderID(combat_unit_sender_id);

    const auto& parent_entity = world.GetByID(zone_data.original_sender_id);
    if (parent_entity.Has<FilteringComponent>())
    {
        auto& parent_filtering_component = parent_entity.Get<FilteringComponent>();

        // Cycle through
        for (const auto id : parent_filtering_component.GetIgnoredReceiverIDs())
        {
            filtering_component.AddIgnoredEntityID(id);
        }
    }

    zone_data.spawn_direction_degrees =
        CalculateZoneSpawnDirection(world, zone_data.shape, zone_target_position, combat_unit_sender);

    // Init the zone data
    zone_component.SetComponentData(zone_data);

    // Zone spawn location
    // NOTE: for non movable zone
    HexGridPosition zone_spawn_location = zone_target_position;

    if (zone_data.attach_to_entity != kInvalidEntityID)
    {
        auto& movement_component = zone_entity.Add<MovementComponent>();
        movement_component.SetMovementType(MovementComponent::MovementType::kSnap);
        movement_component.SetSnapEntityID(zone_data.attach_to_entity);
        if (world.HasEntity(zone_data.attach_to_entity))
        {
            auto& attach_target = world.GetByID(zone_data.attach_to_entity);
            if (attach_target.Has<PositionComponent>())
            {
                const auto& attach_target_position_component = attach_target.Get<PositionComponent>();
                zone_spawn_location = attach_target_position_component.GetPosition();
            }
        }
    }
    // Set movement data, if applicable
    else if (zone_data.movement_speed_sub_units_per_time_step > 0)
    {
        const auto* combat_unit_position_component =
            grid_helper.GetSourcePositionComponent(world.GetByID(combat_unit_sender_id));
        if (combat_unit_position_component)
        {
            // Movable zone spawn location
            zone_spawn_location = combat_unit_position_component->GetPosition();

            if (zone_data.skill_data->zone.predefined_spawn_position != PredefinedGridPosition::kNone)
            {
                zone_spawn_location =
                    grid_config.ResolvePredefinedPosition(team, zone_data.skill_data->zone.predefined_spawn_position);
            }

            // Calculate zone spawn direction based on angle of combat unit and enemy positions
            zone_data.spawn_direction_degrees = combat_unit_position_component->AngleToPosition(zone_target_position);

            // Calculate the vector from the attacking unit and the sending zone unit
            // NOTE: For movable zone we moving zone from combat unit to receiver position
            const HexGridPosition vector_to_target =
                zone_target_position.ToSubUnits() - zone_spawn_location.ToSubUnits();

            // Distance between combat entity and combat enemy entity
            const int distance_subunits = vector_to_target.Length();

            // For how long the zone lasts in time steps
            const int travel_time_time_steps = distance_subunits / zone_data.movement_speed_sub_units_per_time_step;

            // For how long the zone lasts in ms
            const int travel_time_ms = Time::TimeStepsToMs(travel_time_time_steps);

            // If the distance between units is greater than the duration of the zone,
            // the zone moves to the unit's position.
            auto& movement_component = zone_entity.Add<MovementComponent>();
            if (travel_time_ms > zone_data.duration_ms)
            {
                // For how long the zone lasts in ms
                zone_data.duration_ms = travel_time_ms;

                // Move the zone to unit position
                movement_component.SetMovementType(MovementComponent::MovementType::kDirectPosition);
                movement_component.SetMovementTargetPosition(zone_target_position);
            }
            else
            {
                // Set the target as a scaled normal vector
                movement_component.SetMovementType(MovementComponent::MovementType::kDirectVector);
                movement_component.SetMovementTargetVector(vector_to_target.ToNormalizedAndScaled());
            }

            movement_component.SetMovementSpeedSubUnits(
                FixedPoint::FromInt(zone_data.movement_speed_sub_units_per_time_step));
        }
    }

    // A zone should never change target as its targets are already temporary
    focus_component.SetSelectionType(FocusComponent::SelectionType::kPredefined);
    focus_component.SetRefocusType(FocusComponent::RefocusType::kNever);

    // Set position to spawn location
    position_component.InitFromData(zone_spawn_location, 0, false);

    // Inform duration component of zone duration
    duration_component.SetDurationMs(zone_data.duration_ms);

    // To make the system more flexible we make the zone use the ability system
    // We carry the payload of the skill into our own data structures

    // Copy the stats of the combat unit that spawned this zone
    const StatsData stats_data = GetStatsDataForSpawnedEntity(world, combat_unit_sender_id);

    // Copy to zone
    stats_component.SetTemplateStats(stats_data);

    // Store parent stats
    UpdateCombatUnitParentLiveStats(world, zone_entity);

    // Create the attack abilities
    AbilitiesData attack_abilities{};

    // Ability timers should be at zero
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Create the skill
    auto& ability = attack_abilities.AddAbility();
    ability.name = zone_data.source_context.ability_name;
    ability.source_context = zone_data.source_context;

    // Ignore Attack ability speed
    ability.ignore_attack_speed = true;

    // Set the payload to be direct and for the current target
    auto& skill = ability.AddSkill();
    skill.targeting.type = SkillTargetingType::kCurrentFocus;
    skill.deployment.type = SkillDeploymentType::kDirect;
    skill.is_critical = zone_data.is_critical;

    // Copy the payload from the zone data
    if (zone_data.skill_data)
    {
        skill.deployment.guidance = zone_data.skill_data->deployment.guidance;
        skill.effect_package = zone_data.skill_data->effect_package;
    }

    // Set on the component
    abilities_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);

    // Emit event
    world.LogDebug(zone_data.sender_id, "EntityFactory::SpawnZone - zone = {}", zone_entity.GetID());

    // Setting zone growth if not set by the user and the zone max is higher than the starting zone
    if (zone_data.max_radius_sub_units > 0 && zone_data.growth_rate_sub_units_per_time_step == 0)
    {
        zone_component.SetGrowthRateSubUnitsPerStep(
            (zone_component.GetMaxRadiusSubUnits() - zone_component.GetRadiusSubUnits()) /
            zone_component.GetDurationTimeSteps());

        world.LogDebug(
            zone_data.sender_id,
            "EntityFactory::SpawnZone - zone = {}, growing = {}",
            zone_entity.GetID(),
            zone_component.GetGrowthRateSubUnitsPerStep());
    }

    event_data::ZoneCreated event_data;
    event_data.entity_id = zone_entity.GetID();
    event_data.sender_id = zone_data.sender_id;
    event_data.combat_unit_sender_id = combat_unit_sender_id;
    event_data.position = position_component.GetPosition();
    event_data.shape = zone_data.shape;
    event_data.radius_sub_units = zone_data.radius_sub_units;
    event_data.max_radius_sub_units = zone_data.max_radius_sub_units;
    event_data.width_sub_units = zone_data.width_sub_units;
    event_data.height_sub_units = zone_data.height_sub_units;
    event_data.spawn_direction_degrees = zone_data.spawn_direction_degrees;
    event_data.direction_degrees = zone_data.direction_degrees;
    event_data.duration_ms = zone_data.duration_ms;
    event_data.source_context = zone_data.source_context;
    event_data.is_critical = zone_data.is_critical;

    world.EmitEvent<EventType::kZoneCreated>(event_data);

    // Return the new zone
    return &zone_entity;
}

Entity* EntityFactory::SpawnBeam(
    World& world,
    const Team team,
    const HexGridPosition& position,
    const HexGridPosition& target,
    const BeamData& beam_data)
{
    EntityID combat_unit_sender_id = kInvalidEntityID;
    if (!CanSpawnFromSender("EntityFactory::SpawnBeam", world, position, beam_data.sender_id, &combat_unit_sender_id))
    {
        return nullptr;
    }

    // Create an entity
    auto& beam_entity = world.AddEntity(team, combat_unit_sender_id);

    // Add components for beam
    auto& beam_component = beam_entity.Add<BeamComponent>();
    auto& focus_component = beam_entity.Add<FocusComponent>();
    auto& position_component = beam_entity.Add<PositionComponent>();
    auto& stats_component = beam_entity.Add<StatsComponent>();
    auto& abilities_component = beam_entity.Add<AbilitiesComponent>();
    beam_entity.Add<DecisionComponent>();
    beam_entity.Add<DeferredDestructionComponent>();
    beam_entity.Add<FilteringComponent>();

    // Init the beam data
    beam_component.SetComponentData(beam_data);

    // A beam should never change target as its targets are already temporary
    focus_component.SetSelectionType(FocusComponent::SelectionType::kPredefined);
    focus_component.SetRefocusType(FocusComponent::RefocusType::kNever);

    // Set position to spawn location
    position_component.InitFromData(position, 0, false);

    // To make the system more flexible we make the beam use the ability system
    // We carry the payload of the skill into our own data structures

    // Copy the stats of the combat unit that spawned this beam
    const StatsData stats_data = GetStatsDataForSpawnedEntity(world, combat_unit_sender_id);

    // Copy to beam stats
    stats_component.SetTemplateStats(stats_data);

    // Store parent stats
    UpdateCombatUnitParentLiveStats(world, beam_entity);

    // Create the attack abilities
    AbilitiesData attack_abilities{};

    // Ability timers should be at zero
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Create the skill
    auto& ability = attack_abilities.AddAbility();
    ability.name = beam_data.source_context.ability_name;
    ability.source_context = beam_data.source_context;

    // Ignore Attack ability speed
    ability.ignore_attack_speed = true;

    // Set the payload to be direct and for the current target
    auto& skill = ability.AddSkill();
    skill.targeting.type = SkillTargetingType::kCurrentFocus;
    skill.deployment.type = SkillDeploymentType::kDirect;
    skill.is_critical = beam_data.is_critical;

    // Copy the payload from the beam data
    int max_total_time_ms = 0;
    if (beam_data.skill_data)
    {
        skill.deployment.guidance = beam_data.skill_data->deployment.guidance;
        skill.effect_package = beam_data.skill_data->effect_package;
        max_total_time_ms = beam_data.skill_data->channel_time_ms;
    }

    // Set on the component
    abilities_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);

    // Calculate length_sub_units
    const HexGridPosition sub_units_vector = GridHelper::GetSubUnitsVectorBetweenLocation(position, target);

    beam_component.SetWorldLengthSubUnits(world.ToWorldPosition(sub_units_vector).Length());

    // Calculate initial beam direction
    // Get the angle from this position to the receiver position
    // Going counter-clockwise from where x > 0 and y = 0
    const int beam_direction_degrees = position_component.AngleToPosition(target);

    beam_component.SetDirectionDegrees(beam_direction_degrees);

    // Emit event
    world.LogDebug(
        beam_data.sender_id,
        "EntityFactory::SpawnBeam - beam = {}, receiver_id = {}, beam_direction_degrees = {}",
        beam_entity.GetID(),
        beam_data.receiver_id,
        beam_component.GetDirectionDegrees());

    event_data::BeamCreated event_data;
    event_data.entity_id = beam_entity.GetID();
    event_data.sender_id = beam_component.GetSenderID();
    event_data.combat_unit_sender_id = combat_unit_sender_id;
    event_data.receiver_id = beam_component.GetReceiverID();  // Can be invalid
    event_data.beam_direction_degrees = beam_component.GetDirectionDegrees();
    event_data.beam_world_length_sub_units = beam_component.GetWorldLengthSubUnits();
    event_data.beam_width_sub_units = beam_component.GetWidthSubUnits();
    event_data.position = position_component.GetPosition();
    event_data.max_total_time_ms = max_total_time_ms;
    event_data.source_context = beam_component.GetSourceContext();
    event_data.is_critical = beam_data.is_critical;

    world.EmitEvent<EventType::kBeamCreated>(event_data);

    // Return the new beam
    return &beam_entity;
}

Entity* EntityFactory::SpawnDash(World& world, const Team team, const DashData& dash_data, const int skill_duration_ms)
{
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // Receiver required
    if (!CanSpawnFromSenderToReceiver(
            "EntityFactory::SpawnDash",
            world,
            {},
            dash_data.sender_id,
            dash_data.receiver_id,
            &combat_unit_sender_id))
    {
        return nullptr;
    }

    // Create an entity
    auto& dash_entity = world.AddEntity(team, combat_unit_sender_id);
    world.AddTickDependency(dash_entity.GetID(), combat_unit_sender_id);

    // Add components for dashes
    auto& dash_component = dash_entity.Add<DashComponent>();
    auto& focus_component = dash_entity.Add<FocusComponent>();
    auto& movement_component = dash_entity.Add<MovementComponent>();
    auto& stats_component = dash_entity.Add<StatsComponent>();
    auto& abilities_component = dash_entity.Add<AbilitiesComponent>();
    dash_entity.Add<FilteringComponent>();

    // Init the dash data
    dash_component.SetComponentData(dash_data);

    // A dash should never change target
    focus_component.SetSelectionType(FocusComponent::SelectionType::kPredefined);
    focus_component.SetRefocusType(FocusComponent::RefocusType::kNever);

    // Store initial position
    const auto& sender_entity = world.GetByID(dash_data.sender_id);
    auto& sender_position_component = sender_entity.Get<PositionComponent>();
    dash_component.SetLastPosition(sender_position_component.GetPosition());

    // Set movement to be direct
    // Follow to the target
    movement_component.SetMovementType(MovementComponent::MovementType::kDirectPosition);

    // Retrieve a reserved position, if available
    HexGridPosition open_position = sender_position_component.GetReservedPosition();

    const GridHelper& grid_helper = world.GetGridHelper();
    if (open_position == kInvalidHexHexGridPosition)
    {
        // Build obstacle map to find open spot
        grid_helper.BuildObstacles(&sender_entity);

        // Set the target position
        const auto& receiver_entity = world.GetByID(dash_data.receiver_id);
        const auto& receiver_position_component = receiver_entity.Get<PositionComponent>();

        // Get open spot - we know the target location is not usable for at least the sum of the radius units
        // Evaluate position criteria
        if (dash_data.land_behind)
        {
            open_position = grid_helper.GetOpenPositionBehind(
                sender_position_component.GetPosition(),
                receiver_position_component.GetPosition(),
                sender_position_component.GetRadius());
        }
        else
        {
            open_position = grid_helper.GetOpenPositionNearby(
                receiver_position_component.GetPosition(),
                sender_position_component.GetRadius());
        }
    }

    // At this point, position could still be invalid (no space on board)
    if (open_position == kInvalidHexHexGridPosition)
    {
        // Can't go anywhere, set target to self
        movement_component.SetMovementTargetPosition(sender_position_component.GetPosition());
    }
    else
    {
        // Reserve position if not already reserved
        sender_position_component.SetReservedPosition(open_position);

        // Set target to open spot
        movement_component.SetMovementTargetPosition(open_position);
    }

    // Set the speed of the dash
    const auto dash_vector = sender_position_component.GetReservedPosition() - sender_position_component.GetPosition();
    const auto dash_distance_sub_units = dash_vector.ToSubUnits().Length();
    const auto skill_duration_time_steps = FixedPoint::FromInt(skill_duration_ms) / FixedPoint::FromInt(kMsPerTimeStep);
    const auto dash_speed_sub_units_per_time_step =
        FixedPoint::FromInt(dash_distance_sub_units) / skill_duration_time_steps;

    // in sub units per time step
    movement_component.SetMovementSpeedSubUnits(dash_speed_sub_units_per_time_step);

    // To make the system more flexible we make the dashes use the ability system
    // We carry the payload of the skill into our own data structures

    // Copy the stats of the entity that started this dash
    StatsData stats_data = GetStatsDataForSpawnedEntity(world, combat_unit_sender_id);

    // Dash range
    stats_data.Set(StatType::kAttackRangeUnits, FixedPoint::FromInt(dash_data.range_units));

    // Copy to dash
    stats_component.SetTemplateStats(stats_data);

    // Store parent stats
    UpdateCombatUnitParentLiveStats(world, dash_entity);

    // Create the attack abilities
    AbilitiesData attack_abilities{};

    // Ability timers should be at zero
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Create the skill
    auto& ability = attack_abilities.AddAbility();
    ability.name = dash_data.source_context.ability_name;
    ability.source_context = dash_data.source_context;

    // Ignore Attack ability speed
    ability.ignore_attack_speed = true;

    // Set the payload to be direct and for the current target
    auto& skill = ability.AddSkill();
    skill.targeting.type = SkillTargetingType::kCurrentFocus;
    skill.deployment.type = SkillDeploymentType::kDirect;

    // Copy the payload from the dash data
    if (dash_data.skill_data)
    {
        skill.deployment.guidance = dash_data.skill_data->deployment.guidance;
        skill.effect_package = dash_data.skill_data->effect_package;
    }

    // Set on the component
    abilities_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);

    // Emit event
    world.LogDebug(
        dash_data.sender_id,
        "EntityFactory::SpawnDash - dash = {}, receiver = {}",
        dash_entity.GetID(),
        dash_data.receiver_id);

    event_data::DashCreated event_data;
    event_data.entity_id = dash_entity.GetID();
    event_data.sender_id = dash_data.sender_id;
    event_data.combat_unit_sender_id = combat_unit_sender_id;
    event_data.receiver_id = dash_data.receiver_id;
    event_data.target_position = movement_component.GetMovementTargetPosition();
    event_data.source_context = dash_data.source_context;

    world.EmitEvent<EventType::kDashCreated>(event_data);

    // Stop normal movement for the unit
    auto& sender_movement_component = world.GetByIDPtr(dash_data.sender_id)->Get<MovementComponent>();
    sender_movement_component.IncrementMovementPaused();

    // Return the new dash
    return &dash_entity;
}

Entity* EntityFactory::SpawnShieldAndAttach(World& world, const Team team, const ShieldData& shield_data)
{
    EntityID combat_unit_sender_id = kInvalidEntityID;
    if (!CanSpawnFromSenderToReceiver(
            "EntityFactory::SpawnShieldAndAttach",
            world,
            {},
            shield_data.sender_id,
            shield_data.receiver_id,
            &combat_unit_sender_id))
    {
        return nullptr;
    }

    const auto& receiver_entity = world.GetByID(shield_data.receiver_id);
    if (!receiver_entity.Has<AttachedEntityComponent>())
    {
        world.LogErr(
            "EntityFactory::SpawnShieldAndAttach - receiver_id = {} does not have the "
            "AttachedEntityComponent Component.",
            shield_data.receiver_id);
        return nullptr;
    }

    // Create an entity
    auto& shield_entity = world.AddEntity(team, combat_unit_sender_id);

    // Add components for shield
    auto& shield_component = shield_entity.Add<ShieldComponent>();
    shield_entity.Add<AbilitiesComponent>();
    shield_entity.Add<DecisionComponent>();
    shield_entity.Add<FocusComponent>();
    shield_entity.Add<AttachedEffectsComponent>();
    auto& duration_component = shield_entity.Add<DurationComponent>();
    shield_entity.Add<DeferredDestructionComponent>();

    // Init the shield data
    auto new_shield_data = shield_data;
    new_shield_data.source_context.Add(SourceContextType::kShield);
    shield_component.SetComponentData(new_shield_data);
    duration_component.SetDurationMs(new_shield_data.duration_ms);

    // Attach the shield to shield_data.receiver_id entity
    const AttachedEntity attached_entity{.type = AttachedEntityType::kShield, .id = shield_entity.GetID()};
    receiver_entity.Get<AttachedEntityComponent>().AddAttachedEntity(attached_entity);

    // Emit event
    world.LogDebug(
        new_shield_data.sender_id,
        "EntityFactory::SpawnShieldAndAttach - shield = {}, receiver_id = {}",
        shield_entity.GetID(),
        new_shield_data.receiver_id);

    event_data::ShieldCreated event_data;
    event_data.entity_id = shield_entity.GetID();
    event_data.sender_id = shield_component.GetSenderID();
    event_data.combat_unit_sender_id = combat_unit_sender_id;
    event_data.receiver_id = shield_component.GetReceiverID();
    event_data.value = shield_component.GetValue();
    event_data.source_context = new_shield_data.source_context;

    world.EmitEvent<EventType::kShieldCreated>(event_data);

    // Return the new shield
    return &shield_entity;
}

Entity* EntityFactory::SpawnAuraAndAttach(World& world, AuraData data)
{
    static constexpr std::string_view method_name = "EntityFactory::SpawnAuraAndAttach";

    EntityID combat_unit_sender_id = kInvalidEntityID;
    if (!CanSpawnFromSenderToReceiver(
            method_name,
            world,
            {},
            data.aura_sender_id,
            data.receiver_id,
            &combat_unit_sender_id))
    {
        return nullptr;
    }

    // Aura entity is a child of combat unit it was applied to because it is going to search nearby entities
    // using position of it's parent.It still might have different team (for example when someone spawns detrimental
    // aura on enemy)
    auto& aura =
        world.AddEntity(world.GetEntityTeam(data.aura_sender_id), world.GetCombatUnitParentID(data.receiver_id));
    auto& aura_component = aura.Add<AuraComponent>();
    data.source_context.Add(SourceContextType::kAura);
    aura_component.SetComponentData(std::move(data));
    aura_component.SetCreatedAtTimeStep(world.GetTimeStepCount());

    world.EmitEvent<EventType::kAuraCreated>({.entity_id = aura.GetID()});

    return &aura;
}

Entity* EntityFactory::SpawnMarkAndAttach(World& world, const Team team, const MarkData& mark_data)
{
    EntityID combat_unit_sender_id = kInvalidEntityID;
    if (!CanSpawnFromSenderToReceiver(
            "EntityFactory::SpawnMarkAndAttach",
            world,
            {},
            mark_data.sender_id,
            mark_data.receiver_id,
            &combat_unit_sender_id))
    {
        return nullptr;
    }

    const auto& receiver_entity = world.GetByID(mark_data.receiver_id);
    if (!receiver_entity.Has<AttachedEntityComponent>())
    {
        world.LogErr(
            "EntityFactory::SpawnMarkAndAttach - receiver_id = {} does not have the "
            "AttachedEntityComponent Component.",
            mark_data.receiver_id);
        return nullptr;
    }

    // Create an entity
    auto& mark_entity = world.AddEntity(team, combat_unit_sender_id);

    // Add components for mark
    auto& mark_component = mark_entity.Add<MarkComponent>();
    mark_entity.Add<AttachedEffectsComponent>();
    mark_entity.Add<AbilitiesComponent>();
    mark_entity.Add<DeferredDestructionComponent>();
    auto& duration_component = mark_entity.Add<DurationComponent>();

    auto new_mark_data = mark_data;
    new_mark_data.source_context.Add(SourceContextType::kMark);
    mark_component.SetComponentData(new_mark_data);
    duration_component.SetDurationMs(new_mark_data.duration_ms);

    // Attach the mark to mark_data.receiver_id entity
    const AttachedEntity attached_entity{AttachedEntityType::kMark, mark_entity.GetID()};
    receiver_entity.Get<AttachedEntityComponent>().AddAttachedEntity(attached_entity);

    // Emit event
    world.LogDebug(
        new_mark_data.sender_id,
        "EntityFactory::SpawnMarkAndAttach - mark = {}, receiver_id = {}, should_destroy_on_sender_death = {}",
        mark_entity.GetID(),
        new_mark_data.receiver_id,
        new_mark_data.should_destroy_on_sender_death);

    event_data::MarkCreated event_data;
    event_data.entity_id = mark_entity.GetID();
    event_data.sender_id = mark_component.GetSenderID();
    event_data.combat_unit_sender_id = combat_unit_sender_id;
    event_data.receiver_id = mark_component.GetReceiverID();
    event_data.source_context = new_mark_data.source_context;

    world.EmitEvent<EventType::kMarkCreated>(event_data);

    return &mark_entity;
}

Entity* EntityFactory::SpawnSynergyEntity(World& world, const Team team)
{
    auto& synergy_entity = world.AddEntity(team);
    synergy_entity.Add<SynergyEntityComponent>();
    synergy_entity.Add<AbilitiesComponent>();
    return &synergy_entity;
}

Entity*
EntityFactory::SpawnDroneAugmentEntity(World& world, const DroneAugmentData& drone_augment_data, const Team team)
{
    // NOTE: We spawn all drone augment types, because we must still enforce the limit
    auto& drone_augment_entity = world.AddEntity(team);

    auto& component = drone_augment_entity.Add<DroneAugmentEntityComponent>();
    component.SetDroneAugmentTypeID(drone_augment_data.type_id);

    auto& abilities_component = drone_augment_entity.Add<AbilitiesComponent>();
    abilities_component.SetAbilitiesData(drone_augment_data.innate_abilities, AbilityType::kInnate);
    return &drone_augment_entity;
}

bool EntityFactory::CanSpawnFromSender(
    const std::string_view method_context,
    const World& world,
    const HexGridPosition& position,
    const EntityID sender_id,
    EntityID* out_combat_unit_sender_id)
{
    static constexpr bool is_taking_space = false;
    const GridHelper& grid_helper = world.GetGridHelper();
    if (!grid_helper.IsValidHexagonPosition(
            {.center = position, .radius_units = 0, .is_taking_space = is_taking_space}))
    {
        world.LogErr("{} - Invalid Grid Position = {}", method_context, position);
        return false;
    }

    return CanSpawnFromSender(method_context, world, sender_id, out_combat_unit_sender_id);
}

bool EntityFactory::CanSpawnFromCombatUnit(
    const std::string_view method_context,
    const World& world,
    const EntityID combat_unit_id)
{
    if (!world.HasEntity(combat_unit_id))
    {
        world.LogErr("{} - combat_unit_sender_id = {} does NOT exist.", method_context, combat_unit_id);
        return false;
    }

    const auto& combat_unit = world.GetByID(combat_unit_id);
    if (!EntityHelper::IsACombatUnit(combat_unit))
    {
        world.LogErr("{} - combat_unit_sender_id = {} is NOT a combat unit.", method_context, combat_unit_id);
        return false;
    }

    return true;
}

bool EntityFactory::CanSpawnFromSender(
    const std::string_view method_context,
    const World& world,
    const EntityID sender_id,
    EntityID* out_combat_unit_sender_id)
{
    if (!world.HasEntity(sender_id))
    {
        world.LogErr("{} - sender_id = {} does not exist.", method_context, sender_id);
        return false;
    }

    // Check if sender combat unit is alive
    *out_combat_unit_sender_id = world.GetCombatUnitParentID(sender_id);
    if (!EntityHelper::IsASynergy(world, sender_id) && !EntityHelper::IsADroneAugment(world, sender_id) &&
        !CanSpawnFromCombatUnit(method_context, world, *out_combat_unit_sender_id))
    {
        return false;
    }

    return true;
}

bool EntityFactory::CanSpawnFromSenderToReceiver(
    const std::string_view method_context,
    const World& world,
    const EntityID sender_id,
    const EntityID receiver_id,
    EntityID* out_combat_unit_sender_id)
{
    *out_combat_unit_sender_id = kInvalidEntityID;
    if (!world.HasEntity(receiver_id))
    {
        world.LogErr("{} - receiver_id = {} does not exist.", method_context, receiver_id);
        return false;
    }

    return CanSpawnFromSender(method_context, world, sender_id, out_combat_unit_sender_id);
}

bool EntityFactory::CanSpawnFromSenderToReceiver(
    const std::string_view method_context,
    const World& world,
    const HexGridPosition& position,
    const EntityID sender_id,
    const EntityID receiver_id,
    EntityID* out_combat_unit_sender_id)
{
    *out_combat_unit_sender_id = kInvalidEntityID;
    if (!world.HasEntity(receiver_id))
    {
        world.LogErr("{} - receiver_id = {} does not exist.", method_context, receiver_id);
        return false;
    }

    return CanSpawnFromSender(method_context, world, position, sender_id, out_combat_unit_sender_id);
}

StatsData EntityFactory::GetStatsDataForSpawnedEntity(const World& world, const EntityID combat_unit_id)
{
    StatsData stats_data = world.GetLiveStats(combat_unit_id);

    // Max hit chance for beam
    stats_data.Set(StatType::kHitChancePercentage, kMaxPercentageFP);

    // Right on top of it
    stats_data.Set(StatType::kAttackRangeUnits, 0_fp);

    return stats_data;
}

void EntityFactory::UpdateCombatUnitParentLiveStats(const World& world, const Entity& entity)
{
    if (!entity.Has<StatsComponent>())
    {
        return;
    }

    // Nothing to update
    const EntityID parent_id = entity.GetParentID();
    if (!world.HasEntity(parent_id))
    {
        return;
    }

    auto& stats_component = entity.Get<StatsComponent>();
    const Entity& parent_entity = world.GetByID(parent_id);
    if (EntityHelper::IsACombatUnit(parent_entity))
    {
        // Calculate the stats of the combat unit
        stats_component.SetCombatUnitParentLiveStats(world.GetLiveStats(parent_entity));
    }
    else if (parent_entity.Has<StatsComponent>())
    {
        // Get the stats from the parent that itself is not a combat unit
        const auto& parent_stats_component = parent_entity.Get<StatsComponent>();
        stats_component.SetCombatUnitParentLiveStats(parent_stats_component.GetCombatUnitParentLiveStats());
    }
}

}  // namespace simulation
