#include "utility/augment_helper.h"

#include "components/augment_component.h"
#include "components/combat_synergy_component.h"
#include "components/stats_component.h"
#include "data/augment/augment_data.h"
#include "data/containers/game_data_container.h"
#include "systems/synergy_system.h"
#include "utility/entity_helper.h"

namespace simulation
{
AugmentHelper::AugmentHelper(World* world) : world_(world) {}

std::shared_ptr<Logger> AugmentHelper::GetLogger() const
{
    return world_->GetLogger();
}

void AugmentHelper::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int AugmentHelper::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

bool AugmentHelper::CanAddMoreAugments(const EntityID entity_id) const
{
    if (!world_->HasEntity(entity_id))
    {
        return false;
    }

    const auto& entity = world_->GetByID(entity_id);
    return CanAddMoreAugments(entity);
}

bool AugmentHelper::CanAddMoreAugments(const Entity& entity) const
{
    const bool is_battle_started = world_->IsBattleStarted();
    if (is_battle_started)
    {
        LogErr(entity.GetID(), "AugmentHelper::CanAddAnyAugment - failed to apply augment, battle already started");
        return false;
    }

    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    if (!entity.Has<AugmentComponent>())
    {
        return false;
    }

    const auto& augment_component = entity.Get<AugmentComponent>();
    return CanAddMoreAugments(augment_component.GetEquippedAugmentsSize());
}

bool AugmentHelper::CanAddMoreAugments(const size_t equipped_augments_size) const
{
    return equipped_augments_size < world_->GetAugmentsConfig().max_augments;
}

bool AugmentHelper::CanAddAugment(const EntityID entity_id, const AugmentTypeID& augment_type_id) const
{
    if (!world_->HasEntity(entity_id))
    {
        return false;
    }

    const auto& entity = world_->GetByID(entity_id);
    return CanAddAugment(entity, augment_type_id);
}

bool AugmentHelper::CanAddAugment(const Entity& entity, const AugmentTypeID& augment_type_id) const
{
    if (!CanAddMoreAugments(entity))
    {
        return false;
    }

    const auto& game_data_container = world_->GetGameDataContainer();
    const auto augment_data = game_data_container.GetAugmentData(augment_type_id);
    if (!augment_data)
    {
        return false;
    }

    const SynergiesHelper& synergies_helper = world_->GetSynergiesHelper();

    // Ensure that incoming augment does not duplicate any existing combat affinity of an entity
    const EnumSet<CombatAffinity> entity_combat_affinities = synergies_helper.GetAllCombatAffinitiesOfEntity(entity);
    for (auto& [combat_affinity, count] : augment_data->combat_affinities)
    {
        if (count > 0 && entity_combat_affinities.Contains(combat_affinity))
        {
            return false;
        }
    }

    // Ensure that incoming augment does not duplicate any existing combat class of an entity
    const EnumSet<CombatClass> entity_combat_classes = synergies_helper.GetAllCombatClassesOfEntity(entity);
    for (auto& [combat_class, count] : augment_data->combat_classes)
    {
        if (count > 0 && entity_combat_classes.Contains(combat_class))
        {
            return false;
        }
    }

    return true;
}

bool AugmentHelper::CanAddAugmentToCombatUnit(
    const CombatUnitTypeID& unit_type_id,
    const std::vector<AugmentInstanceData>& equipped_augments,
    const AugmentTypeID& add_augment_type_id) const
{
    if (!CanAddMoreAugments(equipped_augments.size()))
    {
        return false;
    }

    const auto& game_data_container = world_->GetGameDataContainer();
    const auto augment_data = game_data_container.GetAugmentData(add_augment_type_id);
    if (!augment_data)
    {
        return false;
    }

    const SynergiesHelper& synergies_helper = world_->GetSynergiesHelper();

    // Ensure that incoming augment does not duplicate any existing combat affinity of an entity
    const EnumSet<CombatAffinity> entity_combat_affinities =
        synergies_helper.GetAllCombatAffinitiesOfCombatUnit(unit_type_id, equipped_augments);
    for (auto& [combat_affinity, count] : augment_data->combat_affinities)
    {
        if (count > 0 && entity_combat_affinities.Contains(combat_affinity))
        {
            return false;
        }
    }

    // Ensure that incoming augment does not duplicate any existing combat class of an entity
    const EnumSet<CombatClass> entity_combat_classes =
        synergies_helper.GetAllCombatClassesOfCombatUnit(unit_type_id, equipped_augments);
    for (auto& [combat_class, count] : augment_data->combat_classes)
    {
        if (count > 0 && entity_combat_classes.Contains(combat_class))
        {
            return false;
        }
    }

    return true;
}

bool AugmentHelper::CanRemoveAugment(const EntityID entity_id) const
{
    if (!world_->HasEntity(entity_id))
    {
        return false;
    }

    const auto& entity = world_->GetByID(entity_id);
    return CanRemoveAugment(entity);
}

bool AugmentHelper::CanRemoveAugment(const Entity& entity) const
{
    const bool is_battle_started = world_->IsBattleStarted();
    if (is_battle_started)
    {
        LogErr(entity.GetID(), "AugmentHelper::CanRemoveAugment - failed to apply augment, battle already started");
        return false;
    }

    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    if (!entity.Has<AugmentComponent>())
    {
        return false;
    }

    const auto& augment_component = entity.Get<AugmentComponent>();
    return augment_component.GetEquippedAugmentsSize() > 0;
}

bool AugmentHelper::HasAugment(const EntityID entity_id, const AugmentInstanceData& instance) const
{
    if (!world_->HasEntity(entity_id))
    {
        return false;
    }

    const auto& entity = world_->GetByID(entity_id);
    return HasAugment(entity, instance);
}

bool AugmentHelper::HasAugment(const Entity& entity, const AugmentInstanceData& instance) const
{
    if (!entity.Has<AugmentComponent>())
    {
        return false;
    }

    const auto& augment_component = entity.Get<AugmentComponent>();
    return augment_component.HasAugment(instance);
}

bool AugmentHelper::AddAugment(const Entity& entity, const AugmentInstanceData& instance) const
{
    static constexpr std::string_view method_name = "AugmentHelper::AddAugment";

    const AugmentTypeID& augment_type_id = instance.type_id;
    if (!CanAddAugment(entity, augment_type_id))
    {
        LogErr(entity.GetID(), "{} - can't apply augment: {}", method_name, augment_type_id);
        return false;
    }

    // Add augment
    auto& augment_component = entity.Get<AugmentComponent>();
    augment_component.AddAugment(instance);

    world_->GetSystem<SynergySystem>().AddSynergiesFromAugment(entity, instance);

    LogDebug(entity.GetID(), "{} - Added augment: {}", method_name, instance);
    return true;
}

bool AugmentHelper::RemoveAugment(const Entity& entity, const AugmentInstanceData& instance) const
{
    static constexpr std::string_view method_name = "AugmentHelper::RemoveAugment";
    const AugmentTypeID& augment_type_id = instance.type_id;

    if (!CanRemoveAugment(entity))
    {
        LogErr(entity.GetID(), "{} - can't remove augment: {}", method_name, augment_type_id);
        return false;
    }

    // Remove augment
    auto& augment_component = entity.Get<AugmentComponent>();
    augment_component.RemoveAugment(instance);

    LogDebug(entity.GetID(), "{} - Removed augment: {}", method_name, instance);
    return true;
}

void AugmentHelper::GetAllAugmentsCombatAffinities(
    const Entity& entity,
    EnumMap<CombatAffinity, int>* out_combat_affinities_stacks) const
{
    if (!entity.Has<AugmentComponent>())
    {
        return;
    }

    const auto& augment_component = entity.Get<AugmentComponent>();
    const std::vector<AugmentInstanceData>& equipped_augments = augment_component.GetEquippedAugments();
    GetAllAugmentsCombatAffinities(equipped_augments, out_combat_affinities_stacks);
}

void AugmentHelper::GetAllAugmentsCombatAffinities(
    const std::vector<AugmentInstanceData>& equipped_augments,
    EnumMap<CombatAffinity, int>* out_combat_affinities_stacks) const
{
    const auto& game_data_container = world_->GetGameDataContainer();

    for (const AugmentInstanceData& augment_instance : equipped_augments)
    {
        const AugmentTypeID& augment_type_id = augment_instance.type_id;
        const auto augment_data = game_data_container.GetAugmentData(augment_type_id);
        if (!augment_data)
        {
            world_->LogWarn(
                "AugmentHelper::GetAllAugmentsCombatAffinities: unknown augment {}. Skipping.",
                augment_instance);
            continue;
        }

        for (const auto& [combat_affinity, stacks_count] : augment_data->combat_affinities)
        {
            if (stacks_count != 0)
            {
                out_combat_affinities_stacks->GetOrAdd(combat_affinity) += stacks_count;
            }
        }
    }
}

void AugmentHelper::GetAllAugmentsCombatClasses(
    const Entity& entity,
    EnumMap<CombatClass, int>* out_combat_classes_stacks) const
{
    if (!entity.Has<AugmentComponent>())
    {
        return;
    }

    const auto& augment_component = entity.Get<AugmentComponent>();
    const std::vector<AugmentInstanceData>& equipped_augments = augment_component.GetEquippedAugments();
    GetAllAugmentsCombatClasses(equipped_augments, out_combat_classes_stacks);
}

void AugmentHelper::GetAllAugmentsCombatClasses(
    const std::vector<AugmentInstanceData>& equipped_augments,
    EnumMap<CombatClass, int>* out_combat_classes_stacks) const
{
    const auto& game_data_container = world_->GetGameDataContainer();

    for (const AugmentInstanceData& augment_instance : equipped_augments)
    {
        const AugmentTypeID& augment_type_id = augment_instance.type_id;
        const auto augment_data = game_data_container.GetAugmentData(augment_type_id);
        if (!augment_data)
        {
            world_->LogWarn(
                "AugmentHelper::GetAllAugmentsCombatClasses: unknown augment: {}. Skipping.",
                augment_instance);
            continue;
        }

        for (const auto& [combat_class, stacks_count] : augment_data->combat_classes)
        {
            if (stacks_count != 0)
            {
                out_combat_classes_stacks->GetOrAdd(combat_class) += stacks_count;
            }
        }
    }
}

AugmentType AugmentHelper::GetAugmentType(const AugmentTypeID& augment_type_id) const
{
    const auto& game_data_container = world_->GetGameDataContainer();
    const auto augment_data = game_data_container.GetAugmentData(augment_type_id);
    if (!augment_data)
    {
        return AugmentType::kNone;
    }

    return augment_data->augment_type;
}
}  // namespace simulation
