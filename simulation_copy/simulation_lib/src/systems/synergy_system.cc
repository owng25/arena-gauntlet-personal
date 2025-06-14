#include "systems/synergy_system.h"

#include <vector>

#include "components/combat_synergy_component.h"
#include "components/combat_unit_component.h"
#include "data/constants.h"
#include "data/containers/game_data_container.h"
#include "data/synergy_data.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"
#include "systems/ability_system.h"
#include "utility/entity_helper.h"

namespace simulation
{
void SynergySystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kBattleStarted>(this, &Self::OnBattleStarted);
}

void SynergySystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    AddSynergyDataToEntity(entity);
}

void SynergySystem::PreBattleStarted(const Entity& entity)
{
    AddSynergyDataToEntity(entity);
}

void SynergySystem::AddSynergyDataToEntity(const Entity& entity)
{
    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Only apply to entities that have combat class and affinity
    if (!entity.Has<CombatSynergyComponent, AbilitiesComponent>())
    {
        return;
    }

    // Once per entity
    const EntityID entity_id = entity.GetID();
    if (entities_activated_.count(entity_id) > 0)
    {
        return;
    }
    entities_activated_.insert(entity_id);
    LogDebug(entity_id, "SynergySystem::AddSynergyDataToEntity");

    // Set the innate for the entities

    std::vector<std::shared_ptr<const AbilityData>> out_innate_abilities;
    if (GetInnateAbilitiesFromSynergies(entity, out_innate_abilities))
    {
        auto& abilities_component = entity.Get<AbilitiesComponent>();

        abilities_component.AddDataInnateAbilities(out_innate_abilities);
    }
}

void SynergySystem::OnBattleStarted(const Event&)
{
    auto& ability_system = world_->GetSystem<AbilitySystem>();

    // Maps from
    // Key: Team
    // Value: vector of active combat units that belong to this owner
    EnumMap<Team, std::vector<EntityID>> owners_combat_units_map{};

    // Activate on battle start in order
    // Order the entities
    for (const auto& entity : world_->GetAll())
    {
        owners_combat_units_map.GetOrAdd(entity->GetTeam()).push_back(entity->GetID());
    }

    if (AreDebugLogsEnabled())
    {
        // Debug the current values for this team
        const SynergiesStateContainer& synergies_state_container = world_->GetSynergiesStateContainer();
        for (const auto& [team, _] : owners_combat_units_map)
        {
            const auto& synergies_vector = synergies_state_container.GetTeamAllSynergies(team, true);
            LogDebug("OnBattleStarted: team = {}", team);
            for (const SynergyOrder& synergy_order : synergies_vector)
            {
                LogDebug(
                    "- synergy = {}, synergy_stacks = {}",
                    synergy_order.combat_synergy,
                    synergy_order.synergy_stacks);
            }
        }
    }

    // Spawn synergy entities with team abilities granted
    for (const auto& [team, _] : owners_combat_units_map)
    {
        const Entity* entity = EntityFactory::SpawnSynergyEntity(*world_, team);
        AddTeamAbilities(*entity);

        AbilityStateActivatorContext activator_context;
        activator_context.sender_entity_id = entity->GetID();
        activator_context.sender_combat_unit_entity_id = entity->GetID();
        activator_context.ability_type = AbilityType::kNone;

        // Fire that are on battle start
        ability_system.ChooseAndTryToActivateInnateAbilities(
            activator_context,
            entity->GetID(),
            ActivationTriggerType::kOnBattleStart);
    }

    for (const auto& [team, entities_ids] : owners_combat_units_map)
    {
        for (const EntityID entity_id : entities_ids)
        {
            if (!world_->HasEntity(entity_id))
            {
                continue;
            }
            const Entity& entity = world_->GetByID(entity_id);

            AbilityStateActivatorContext activator_context;
            activator_context.sender_entity_id = entity.GetID();
            activator_context.sender_combat_unit_entity_id = entity.GetID();
            activator_context.ability_type = AbilityType::kNone;

            // Fire the omega abilities that are on battle start
            ability_system.ChooseAndTryToActivateInnateAbilities(
                activator_context,
                entity.GetID(),
                ActivationTriggerType::kOnBattleStart);
        }
    }
}

void SynergySystem::AddEntity(const Entity& entity)
{
    LogDebug(entity.GetID(), "SynergySystem::AddEntity - Added Entity to synergy system");
    AddCombatSynergies(entity);
}

void SynergySystem::AddSynergiesFromAugment(const Entity& entity, const AugmentInstanceData& instance)
{
    const AugmentTypeID& augment_type_id = instance.type_id;
    const auto& game_data_container = world_->GetGameDataContainer();
    if (!game_data_container.HasAugmentData(augment_type_id))
    {
        world_->LogWarn("SynergySystem::AddSynergiesFromAugment: unknown augment {}. Skipping.", augment_type_id);
        return;
    }

    const auto augment_data = game_data_container.GetAugmentData(augment_type_id);

    if (!augment_data)
    {
        return;
    }

    // Affinities
    for (const auto& [combat_affinity, stacks] : augment_data->combat_affinities)
    {
        for (int index = 0; index < stacks; ++index)
        {
            const CombatSynergyBonus combat_synergy{combat_affinity};
            AddCombatSynergyFromAugment(entity, combat_synergy);
        }
    }

    // Combat classes
    for (const auto& [combat_class, stacks] : augment_data->combat_classes)
    {
        for (int index = 0; index < stacks; ++index)
        {
            const CombatSynergyBonus combat_synergy{combat_class};
            AddCombatSynergyFromAugment(entity, combat_synergy);
        }
    }
}

void SynergySystem::AddSynergiesFromAugments(const Entity& entity)
{
    LogDebug("SynergySystem::AddSynergiesFromAugments");

    // Affinities
    {
        EnumMap<CombatAffinity, int> affinities;
        world_->GetAugmentHelper().GetAllAugmentsCombatAffinities(entity, &affinities);
        for (const auto& [affinity, stacks] : affinities)
        {
            for (int index = 0; index < stacks; ++index)
            {
                const CombatSynergyBonus combat_synergy{affinity};
                AddCombatSynergyFromAugment(entity, combat_synergy);
            }
        }
    }

    // Combat classes
    {
        EnumMap<CombatClass, int> combat_classes;
        world_->GetAugmentHelper().GetAllAugmentsCombatClasses(entity, &combat_classes);
        for (const auto& [combat_class, stacks] : combat_classes)
        {
            for (int index = 0; index < stacks; ++index)
            {
                const CombatSynergyBonus combat_synergy{combat_class};
                AddCombatSynergyFromAugment(entity, combat_synergy);
            }
        }
    }
}

void SynergySystem::AddTeamAbilities(const Entity& entity) const
{
    if (!EntityHelper::IsASynergy(entity))
    {
        LogErr("SynergySystem::AddSynergyAbilities team abilities should be added only to synergy entity");
        return;
    }

    std::vector<std::shared_ptr<const AbilityData>> out_innate_abilities;
    GetTeamAbilitiesFromSynergies(entity.GetTeam(), out_innate_abilities);

    auto& abilities_component = entity.Get<AbilitiesComponent>();
    abilities_component.AddDataInnateAbilities(out_innate_abilities);
}

void SynergySystem::RefreshCombatSynergies()
{
    LogDebug("SynergySystem::RefreshCombatSynergies");

    // Remove all the classes affinities for the team
    world_->GetSynergiesStateContainerPtr()->Clear();

    // Then read all them back
    for (const auto& entity : world_->GetAll())
    {
        if (entity->Has<CombatSynergyComponent>())
        {
            AddCombatSynergies(*entity);
            AddSynergiesFromAugments(*entity);
        }
    }
}

bool SynergySystem::GetInnateAbilitiesFromSynergies(
    const Entity& entity,
    std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const
{
    if (!entity.Has<CombatSynergyComponent>())
    {
        return false;
    }

    // Figure out the synergy stacks
    const Team team = entity.GetTeam();

    const SynergiesStateContainer& synergies_state_container = world_->GetSynergiesStateContainer();
    const auto& game_data = world_->GetGameDataContainer();

    for (const CombatAffinity combat_affinity : world_->GetSynergiesHelper().GetAllCombatAffinitiesOfEntity(entity))
    {
        const int combat_affinity_synergy_stacks =
            synergies_state_container.GetTeamSynergyStacksForCombatAffinity(team, combat_affinity);

        if (auto synergy_data = game_data.GetCombatAffinitySynergyData(combat_affinity))
        {
            AddUnitAbilitiesForSynergy(combat_affinity_synergy_stacks, *synergy_data, out_innate_abilities);
        }
    }

    for (const CombatClass combat_class : world_->GetSynergiesHelper().GetAllCombatClassesOfEntity(entity))
    {
        const int combat_class_synergy_stacks =
            synergies_state_container.GetTeamSynergyStacksForCombatClass(team, combat_class);

        if (auto synergy_data = game_data.GetCombatClassSynergyData(combat_class))
        {
            AddUnitAbilitiesForSynergy(combat_class_synergy_stacks, *synergy_data, out_innate_abilities);
        }
    }

    return true;
}

void SynergySystem::GetTeamAbilitiesFromSynergies(
    const Team team,
    std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const
{
    const auto& synergies_state_container = world_->GetSynergiesStateContainer();
    const auto& team_synergies = synergies_state_container.GetTeamAllSynergies(team, true);
    const auto& synergies_helper = world_->GetSynergiesHelper();
    for (const SynergyOrder& synergy : team_synergies)
    {
        const auto synergy_data = synergies_helper.FindSynergyData(synergy.combat_synergy);
        if (synergy_data)
        {
            AddTeamAbilitiesForSynergyThresholds(synergy.synergy_stacks, *synergy_data, out_innate_abilities);
        }
    }
}

void SynergySystem::AddUnitAbilitiesForSynergy(
    const int synergy_stacks,
    const SynergyData& synergy_data,
    std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const
{
    const bool added_threshold_abilities =
        AddUnitAbilitiesForSynergyThresholds(synergy_stacks, synergy_data, out_innate_abilities);

    // Add the intrinsic abilities
    if (synergy_data.disable_intrinsic_abilities_on_first_threshold && added_threshold_abilities)
    {
        LogDebug(
            "SynergySystem::AddInnateAbilitiesForSynergy - disable_intrinsic_abilities_on_first_threshold = "
            "true, "
            "NOT ADDING IntrinsicAbilities for combat_synergy = {}",
            synergy_data.combat_synergy);
    }
    else
    {
        VectorHelper::AppendVector(out_innate_abilities, synergy_data.intrinsic_abilities);
    }
}

bool SynergySystem::AddUnitAbilitiesForSynergyThresholds(
    const int synergy_stacks,
    const SynergyData& synergy_data,
    std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const
{
    // Add the synergy thresholds abilities
    if (synergy_stacks <= 0)
    {
        return false;
    }
    if (synergy_data.unit_threshold_abilities.empty())
    {
        return false;
    }

    int active_synergy_threshold = synergy_data.FindMaxThresholdForStacks(synergy_stacks);
    if (active_synergy_threshold == 0)
    {
        // Did not find anything useful.
        return false;
    }

    const auto& units_abilities = synergy_data.unit_threshold_abilities;

    // Must exist in the map
    if (!units_abilities.contains(active_synergy_threshold))
    {
        LogErr(
            "SynergySystem::AddInnateAbilitiesForSynergyThresholds - can't find threshold = {} inside "
            "unit_threshold_abilities",
            active_synergy_threshold);
        return false;
    }

    // Merge with the output
    const auto& threshold_abilities_data = units_abilities.at(active_synergy_threshold);
    VectorHelper::AppendVector(out_innate_abilities, threshold_abilities_data);

    LogDebug(
        "SynergySystem::AddInnateAbilitiesForSynergyThresholds - Adding abilities for threshold = {}",
        active_synergy_threshold);

    return true;
}

bool SynergySystem::AddTeamAbilitiesForSynergyThresholds(
    const int synergy_stacks,
    const SynergyData& synergy_data,
    std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const
{
    // Add the synergy thresholds abilities
    if (synergy_stacks <= 0)
    {
        return false;
    }
    if (synergy_data.team_threshold_abilities.empty())
    {
        return false;
    }

    int active_synergy_threshold = synergy_data.FindMaxThresholdForStacks(synergy_stacks);
    if (active_synergy_threshold == 0)
    {
        // Did not find anything useful.
        return false;
    }

    const auto& team_abilities = synergy_data.team_threshold_abilities;

    // Must exist in the map
    if (team_abilities.count(active_synergy_threshold) == 0)
    {
        LogErr(
            "SynergySystem::AddTeamAbilitiesForSynergyThresholds - can't find threshold = {} inside "
            "team_threshold_abilities",
            active_synergy_threshold);
        return false;
    }

    // Merge with the output
    const auto& threshold_abilities_data = team_abilities.at(active_synergy_threshold);
    VectorHelper::AppendVector(out_innate_abilities, threshold_abilities_data);
    LogDebug(
        "SynergySystem::AddTeamAbilitiesForSynergyThresholds - Adding abilities for threshold = {}",
        active_synergy_threshold);

    return true;
}

void SynergySystem::AddCombatSynergy(const Entity& entity, const CombatSynergyBonus& base_combat_synergy)
{
    static constexpr std::string_view method_name = "SynergySystem::AddCombatSynergy";

    const EntityID entity_id = entity.GetID();
    if (base_combat_synergy.IsEmpty())
    {
        LogDebug(entity_id, "{} - Can't add, CombatAffinity/CombatClass is empty", method_name);
        return;
    }

    if (!entity.Has<CombatUnitComponent>())
    {
        LogErr(entity_id, "{} - Entity doesn't have CombatUnitComponent", method_name);
        return;
    }

    const auto& combat_unit_component = entity.Get<CombatUnitComponent>();
    const CombatUnitTypeID& type_id = combat_unit_component.GetTypeID();
    const Team team = entity.GetTeam();

    const std::shared_ptr<SynergiesStateContainer>& synergies_state_container = world_->GetSynergiesStateContainerPtr();
    constexpr bool is_from_augment = false;
    const bool is_synergy_added =
        synergies_state_container->AddCombatSynergy(team, entity_id, is_from_augment, type_id, base_combat_synergy);
    if (!is_synergy_added)
    {
        LogErr(entity_id, "{} - Can't add, see error above ^^^^^", method_name);
    }
}

void SynergySystem::AddCombatSynergyFromAugment(const Entity& entity, const CombatSynergyBonus& base_combat_synergy)
{
    static constexpr std::string_view method_name = "SynergySystem::AddCombatSynergyFromAugment";

    const EntityID entity_id = entity.GetID();
    if (base_combat_synergy.IsEmpty())
    {
        LogDebug(entity_id, "{} - Can't add, CombatAffinity/CombatClass is empty", method_name);
        return;
    }

    if (!entity.Has<CombatUnitComponent>())
    {
        LogErr(entity_id, "{} - Entity doesn't have CombatUnitComponent", method_name);
        return;
    }
    LogDebug(entity_id, "{} - base_combat_synergy = {}", method_name, base_combat_synergy);

    const auto& combat_unit_component = entity.Get<CombatUnitComponent>();

    const CombatUnitTypeID& type_id = combat_unit_component.GetTypeID();
    const Team team = entity.GetTeam();

    const std::shared_ptr<SynergiesStateContainer>& synergies_state_container = world_->GetSynergiesStateContainerPtr();
    constexpr bool is_from_augment = true;
    const bool synergy_added =
        synergies_state_container->AddCombatSynergy(team, entity_id, is_from_augment, type_id, base_combat_synergy);
    if (!synergy_added)
    {
        LogErr(entity_id, "{} - Can't add, see error above ^^^^^", method_name);
    }
}

void SynergySystem::AddCombatSynergies(const Entity& entity)
{
    static constexpr std::string_view method_name = "SynergySystem::AddCombatSynergies";
    if (!entity.Has<CombatSynergyComponent>())
    {
        LogErr(entity.GetID(), "{} - Entity doesn't have CombatSynergyComponent", method_name);
        return;
    }

    // Don't include augments here as it is handled separate inside AddCombatSynergyFromAugment
    constexpr bool include_augments = false;
    for (const CombatClass combat_class :
         world_->GetSynergiesHelper().GetAllCombatClassesOfEntity(entity, include_augments))
    {
        LogDebug(entity.GetID(), "{} - Adding {} combat class", method_name, combat_class);
        AddCombatSynergy(entity, combat_class);
    }

    for (const CombatAffinity combat_affinity :
         world_->GetSynergiesHelper().GetAllCombatAffinitiesOfEntity(entity, include_augments))
    {
        LogDebug(entity.GetID(), "{} - Adding {} combat affinity", method_name, combat_affinity);
        AddCombatSynergy(entity, combat_affinity);
    }
}

}  // namespace simulation
