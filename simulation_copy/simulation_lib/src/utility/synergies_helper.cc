#include "synergies_helper.h"

#include "components/augment_component.h"
#include "components/combat_synergy_component.h"
#include "data/containers/augments_data_container.h"
#include "data/containers/combat_affinity_synergies_data_container.h"
#include "data/containers/combat_class_synergies_data_container.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"
#include "utility/augment_helper.h"
#include "utility/entity_helper.h"

namespace simulation
{

SynergiesHelper::SynergiesHelper(World* world) : world_(world) {}

std::shared_ptr<Logger> SynergiesHelper::GetLogger() const
{
    return world_->GetLogger();
}

void SynergiesHelper::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int SynergiesHelper::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

EnumSet<CombatClass> SynergiesHelper::GetAllCombatClassesOfEntity(const Entity& entity, const bool include_augments)
    const
{
    // Add unit own combat classes
    CombatClass combat_class = CombatClass::kNone;
    CombatClass dominant_combat_class = CombatClass::kNone;
    if (entity.Has<CombatSynergyComponent>())
    {
        const auto& synergies_component = entity.Get<CombatSynergyComponent>();
        combat_class = synergies_component.GetCombatClass();
        dominant_combat_class = synergies_component.GetDominantCombatClass();
    }

    // Gather combat classes from augments
    std::vector<AugmentInstanceData> equipped_augments;
    if (include_augments && entity.Has<AugmentComponent>())
    {
        equipped_augments = entity.Get<AugmentComponent>().GetEquippedAugments();
    }

    return GetAllCombatClasses(combat_class, dominant_combat_class, equipped_augments);
}

EnumSet<CombatClass> SynergiesHelper::GetAllCombatClassesOfCombatUnit(
    const CombatUnitTypeID& unit_type_id,
    const std::vector<AugmentInstanceData>& equipped_augments) const
{
    const auto& game_data_container = world_->GetGameDataContainer();

    const std::shared_ptr<const CombatUnitData> combat_unit_data_ptr =
        game_data_container.GetCombatUnitData(unit_type_id);
    if (!combat_unit_data_ptr)
    {
        return {};
    }

    return GetAllCombatClasses(
        combat_unit_data_ptr->type_data.combat_class,
        combat_unit_data_ptr->type_data.dominant_combat_class,
        equipped_augments);
}

EnumSet<CombatClass> SynergiesHelper::GetAllCombatClasses(
    const CombatClass combat_class,
    const CombatClass dominant_combat_class,
    const std::vector<AugmentInstanceData>& equipped_augments) const
{
    EnumSet<CombatClass> combat_classes;

    // Add unit own combat classes
    if (combat_class != CombatClass::kNone)
    {
        combat_classes.Add(combat_class);

        // Add dominant if it exists and different
        if (dominant_combat_class != CombatClass::kNone && dominant_combat_class != combat_class)
        {
            combat_classes.Add(dominant_combat_class);
        }
    }

    // Gather combat classes from augments
    if (!equipped_augments.empty())
    {
        EnumMap<CombatClass, int> combat_classes_from_augments;
        world_->GetAugmentHelper().GetAllAugmentsCombatClasses(equipped_augments, &combat_classes_from_augments);
        combat_classes.Add(combat_classes_from_augments.GetKeys());
    }

    return combat_classes;
}

EnumSet<CombatAffinity> SynergiesHelper::GetAllCombatAffinitiesOfEntity(
    const Entity& entity,
    const bool include_augments) const
{
    // Add unit own combat affinities
    CombatAffinity combat_affinity = CombatAffinity::kNone;
    CombatAffinity dominant_combat_affinity = CombatAffinity::kNone;
    if (entity.Has<CombatSynergyComponent>())
    {
        const auto& synergies_component = entity.Get<CombatSynergyComponent>();
        combat_affinity = synergies_component.GetCombatAffinity();
        dominant_combat_affinity = synergies_component.GetDominantCombatAffinity();
    }

    // Gather combat affinities from augments
    std::vector<AugmentInstanceData> equipped_augments;
    if (include_augments && entity.Has<AugmentComponent>())
    {
        equipped_augments = entity.Get<AugmentComponent>().GetEquippedAugments();
    }

    return GetAllCombatAffinities(combat_affinity, dominant_combat_affinity, equipped_augments);
}

EnumSet<CombatAffinity> SynergiesHelper::GetAllCombatAffinitiesOfCombatUnit(
    const CombatUnitTypeID& unit_type_id,
    const std::vector<AugmentInstanceData>& equipped_augments) const
{
    const auto& game_data_container = world_->GetGameDataContainer();

    const std::shared_ptr<const CombatUnitData> combat_unit_data_ptr =
        game_data_container.GetCombatUnitData(unit_type_id);
    if (!combat_unit_data_ptr)
    {
        return {};
    }

    return GetAllCombatAffinities(
        combat_unit_data_ptr->type_data.combat_affinity,
        combat_unit_data_ptr->type_data.dominant_combat_affinity,
        equipped_augments);
}

EnumSet<CombatAffinity> SynergiesHelper::GetAllCombatAffinities(
    const CombatAffinity combat_affinity,
    const CombatAffinity dominant_combat_affinity,
    const std::vector<AugmentInstanceData>& equipped_augments) const
{
    EnumSet<CombatAffinity> combat_affinities;

    // Add unit own combat affinities
    if (combat_affinity != CombatAffinity::kNone)
    {
        // Add base combat affinity
        combat_affinities.Add(combat_affinity);

        // Add dominant if it exists and different
        if (dominant_combat_affinity != CombatAffinity::kNone && dominant_combat_affinity != combat_affinity)
        {
            combat_affinities.Add(dominant_combat_affinity);
        }
    }

    // Gather combat affinities from augments
    if (!equipped_augments.empty())
    {
        EnumMap<CombatAffinity, int> combat_affinities_from_augments;
        world_->GetAugmentHelper().GetAllAugmentsCombatAffinities(equipped_augments, &combat_affinities_from_augments);
        combat_affinities.Add(combat_affinities_from_augments.GetKeys());
    }

    return combat_affinities;
}

bool SynergiesHelper::HasAnyCombatClass(const Entity& entity, const EnumSet<CombatClass> search_combat_classes) const
{
    const EnumSet<CombatClass> combat_classes = GetAllCombatClassesOfEntity(entity);
    const EnumSet<CombatClass> intersection = combat_classes.Intersection(search_combat_classes);
    return !intersection.IsEmpty();
}

bool SynergiesHelper::HasCombatClassOrCompositeFrom(const Entity& entity, const CombatClass search_combat_class) const
{
    const EnumSet<CombatClass> combat_classes = GetAllCombatClassesOfEntity(entity);
    const EnumSet<CombatClass> all_combat_classes = ComplementWithPrimarySynergies(combat_classes);
    return all_combat_classes.Contains(search_combat_class);
}

bool SynergiesHelper::HasAnyCombatAffinity(const Entity& entity, const EnumSet<CombatAffinity> search_combat_affinities)
    const
{
    const EnumSet<CombatAffinity> combat_affinities = GetAllCombatAffinitiesOfEntity(entity);
    const EnumSet<CombatAffinity> intersection = combat_affinities.Intersection(search_combat_affinities);
    return !intersection.IsEmpty();
}

bool SynergiesHelper::CheckSynergiesInAugments() const
{
    const auto& game_data = world_->GetGameDataContainer();

    bool all_valid = true;
    game_data.GetAugmentsDataContainer().ForEach(
        [&](const AugmentData& augment_data)
        {
            for (auto& [combat_affinity, count] : augment_data.combat_affinities)
            {
                const auto synergy_data = game_data.GetCombatAffinitySynergyData(combat_affinity);
                if (!synergy_data)
                {
                    LogErr(
                        "Augment {} has synergy {} which could not be found in synergies data.",
                        augment_data.type_id,
                        combat_affinity);
                    all_valid = false;
                    continue;
                }
            }

            for (auto& [combat_class, count] : augment_data.combat_classes)
            {
                const auto synergy_data = game_data.GetCombatClassSynergyData(combat_class);
                if (!synergy_data)
                {
                    LogErr(
                        "Augment {} has synergy {} which could not be found in synergies data.",
                        augment_data.type_id,
                        combat_class);
                    all_valid = false;
                    continue;
                }
            }
        });

    return all_valid;
}

std::vector<CombatSynergyBonus> SynergiesHelper::GetAllPartsOfCombatSynergy(
    const GameDataContainer& game_data,
    const CombatSynergyBonus combat_synergy)
{
    // Add Base
    std::vector<CombatSynergyBonus> combat_synergy_bonuses{combat_synergy};

    // Add components if any
    if (const auto data = FindCombatSynergyData(game_data, combat_synergy))
    {
        VectorHelper::AppendVector(combat_synergy_bonuses, data->combat_synergy_components);
    }

    return combat_synergy_bonuses;
}

std::vector<CombatSynergyBonus> SynergiesHelper::GetAllPartsOfCombatSynergy(
    const CombatSynergyBonus combat_synergy) const
{
    return GetAllPartsOfCombatSynergy(world_->GetGameDataContainer(), combat_synergy);
}

bool SynergiesHelper::CheckCombatSynergyComposition(
    const GameDataContainer& game_data,
    const CombatSynergyBonus combat_synergy,
    const CombatSynergyBonus search_combat_synergy)
{
    // Matches the combat_synergy class
    if (combat_synergy == search_combat_synergy)
    {
        return true;
    }

    if (const auto synergy_data = FindCombatSynergyData(game_data, combat_synergy))
    {
        return synergy_data->ComponentsContainsCombatSynergy(search_combat_synergy);
    }

    return false;
}

bool SynergiesHelper::CheckCombatSynergyComposition(
    const CombatSynergyBonus combat_synergy,
    const CombatSynergyBonus search_combat_synergy) const
{
    return CheckCombatSynergyComposition(world_->GetGameDataContainer(), combat_synergy, search_combat_synergy);
}

EnumSet<CombatClass> SynergiesHelper::ComplementWithPrimarySynergies(
    const GameDataContainer& game_data,
    EnumSet<CombatClass> combat_classes)
{
    EnumSet<CombatClass> with_components = combat_classes;
    for (const CombatClass combat_class : combat_classes)
    {
        if (const auto data = game_data.GetCombatClassSynergyData(combat_class))
        {
            with_components.Add(data->GetUniqueComponentsAsCombatClasses());
        }
    }

    return with_components;
}

EnumSet<CombatClass> SynergiesHelper::ComplementWithPrimarySynergies(EnumSet<CombatClass> combat_classes) const
{
    return ComplementWithPrimarySynergies(world_->GetGameDataContainer(), combat_classes);
}

EnumSet<CombatAffinity> SynergiesHelper::ComplementWithPrimarySynergies(
    const GameDataContainer& game_data,
    EnumSet<CombatAffinity> combat_affinities)
{
    EnumSet<CombatAffinity> with_components = combat_affinities;
    for (const CombatAffinity combat_affinity : combat_affinities)
    {
        if (const auto data = game_data.GetCombatAffinitySynergyData(combat_affinity))
        {
            with_components.Add(data->GetUniqueComponentsAsCombatAffinities());
        }
    }

    return with_components;
}

EnumSet<CombatAffinity> SynergiesHelper::ComplementWithPrimarySynergies(EnumSet<CombatAffinity> combat_affinities) const
{
    return ComplementWithPrimarySynergies(world_->GetGameDataContainer(), combat_affinities);
}

std::vector<CombatClass> SynergiesHelper::GetAllPartsOfCombatClass(
    const GameDataContainer& game_data,
    const CombatClass combat_class)
{
    // Add Base
    std::vector<CombatClass> combat_classes{combat_class};

    // Add components if any
    if (const auto data = game_data.GetCombatClassSynergyData(combat_class))
    {
        for (const auto& component : data->combat_synergy_components)
        {
            combat_classes.emplace_back(component.GetClass());
        }
    }

    return combat_classes;
}

std::vector<CombatClass> SynergiesHelper::GetAllPartsOfCombatClass(const CombatClass combat_class) const
{
    return GetAllPartsOfCombatClass(world_->GetGameDataContainer(), combat_class);
}

std::vector<CombatAffinity> SynergiesHelper::GetAllPartsOfCombatAffinity(
    const GameDataContainer& game_data,
    const CombatAffinity combat_affinity)
{
    // Add Base
    std::vector<CombatAffinity> combat_affinities{combat_affinity};

    // Add components if any
    if (const auto data = game_data.GetCombatAffinitySynergyData(combat_affinity))
    {
        for (const auto& component : data->combat_synergy_components)
        {
            combat_affinities.emplace_back(component.GetAffinity());
        }
    }

    return combat_affinities;
}

std::vector<CombatAffinity> SynergiesHelper::GetAllPartsOfCombatAffinity(const CombatAffinity combat_affinity) const
{
    return GetAllPartsOfCombatAffinity(world_->GetGameDataContainer(), combat_affinity);
}

CombatSynergyBonus SynergiesHelper::Bond(
    const GameDataContainer& game_data,
    const CombatSynergyBonus first,
    const CombatSynergyBonus second,
    Logger& logger)
{
    static constexpr std::string_view method_name = "SynergiesHelper::BondCombatSynergies";

    // Ensure both first and second are primary synergies
    auto first_data = FindCombatSynergyData(game_data, first);
    if (first_data && !first_data->IsPrimary())
    {
        logger.LogErr("{} - {} is not a primary synergy", method_name, first);
        return {};
    }

    const auto second_data = FindCombatSynergyData(game_data, second);
    if (second_data && !second_data->IsPrimary())
    {
        logger.LogErr("{} - {} is not a primary synergy", method_name, second);
        return {};
    }

    // Return first not empty
    if (first.IsEmpty())
    {
        return second;
    }
    if (second.IsEmpty())
    {
        return first;
    }

    // Search through all of the synergies to check any match
    CombatSynergyBonus composite{};

    auto callback = [&](const SynergyData& synergy_data)
    {
        const auto& combat_synergy_components = synergy_data.combat_synergy_components;
        if (combat_synergy_components.size() < 2)
        {
            return false;
        }

        const CombatSynergyBonus first_component = combat_synergy_components[0];
        const CombatSynergyBonus second_component = combat_synergy_components[1];
        if ((first_component == first && second_component == second) ||
            (first_component == second && second_component == first))
        {
            composite = synergy_data.combat_synergy;
            return true;  // Break the loop
        }

        return false;  // Continue iteration
    };

    bool can_make_composite = false;
    if (first.IsCombatAffinity())
    {
        can_make_composite = game_data.GetCombatAffinitySynergiesDataContainer().ForEachWithBreak(callback);
    }
    else if (first.IsCombatClass())
    {
        can_make_composite = game_data.GetCombatClassSynergiesDataContainer().ForEachWithBreak(callback);
    }

    if (!can_make_composite)
    {
        // Nothing found
        logger.LogErr("{} - Could not find any composite synergy for {} and {}", method_name, first, second);
    }

    return composite;
}

CombatSynergyBonus SynergiesHelper::Bond(const CombatSynergyBonus first, const CombatSynergyBonus second) const
{
    return Bond(world_->GetGameDataContainer(), first, second, *world_->GetLogger());
}

std::shared_ptr<const SynergyData> SynergiesHelper::FindCombatSynergyData(
    const GameDataContainer& game_data,
    const CombatSynergyBonus& combat_synergy)
{
    if (combat_synergy.IsCombatAffinity())
    {
        return game_data.GetCombatAffinitySynergyData(combat_synergy.GetAffinity());
    }

    return game_data.GetCombatClassSynergyData(combat_synergy.GetClass());
}

std::shared_ptr<const SynergyData> SynergiesHelper::FindSynergyData(const CombatSynergyBonus& combat_synergy) const
{
    return FindCombatSynergyData(world_->GetGameDataContainer(), combat_synergy);
}
}  // namespace simulation
