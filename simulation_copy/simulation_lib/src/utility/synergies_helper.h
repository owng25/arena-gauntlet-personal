#pragma once

#include "data/combat_synergy_bonus.h"
#include "data/constants.h"
#include "utility/enum_set.h"
#include "utility/logger_consumer.h"

namespace simulation
{
class CombatUnitTypeID;
}

namespace simulation
{
class AugmentInstanceData;
class World;
class Entity;
class SynergyData;
class GameDataContainer;

/* -------------------------------------------------------------------------------------------------------
 * SynergiesHelper
 *
 * This defines world synergies helper functions
 * Augments can now give some synergies to combet unit so this is a classes which from both
 * synergy and augments data containers to get some information
 * --------------------------------------------------------------------------------------------------------
 */
class SynergiesHelper final : public LoggerConsumer
{
public:
    SynergiesHelper() = default;
    explicit SynergiesHelper(World* world);

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;
    int GetTimeStepCount() const override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSynergy;
    }

    //
    // Own functions
    //

    // Does this entity have this combat class?
    bool HasCombatClass(const Entity& entity, const CombatClass search_combat_class) const
    {
        return HasAnyCombatClass(entity, MakeEnumSet(search_combat_class));
    }

    // Does this entity has any of combat classes specified in the set?
    bool HasAnyCombatClass(const Entity& entity, const EnumSet<CombatClass> search_combat_classes) const;

    // Does this entity have this combat class or have composite class created from this class ?
    bool HasCombatClassOrCompositeFrom(const Entity& entity, const CombatClass search_combat_class) const;

    // Does this entity have this combat affinity?
    bool HasCombatAffinity(const Entity& entity, const CombatAffinity search_combat_affinity) const
    {
        return HasAnyCombatAffinity(entity, MakeEnumSet(search_combat_affinity));
    }

    // Does this entity has any of combat affinities specified in the set?
    bool HasAnyCombatAffinity(const Entity& entity, const EnumSet<CombatAffinity> search_combat_affinities) const;

    // Does this entity have this combat synergy?
    bool HasCombatSynergy(const Entity& entity, const CombatSynergyBonus combat_synergy) const
    {
        // CombatAffinity
        if (combat_synergy.IsCombatAffinity())
        {
            return HasCombatAffinity(entity, combat_synergy.GetAffinity());
        }

        // CombatClass
        return HasCombatClass(entity, combat_synergy.GetClass());
    }

    EnumSet<CombatClass> GetAllCombatClassesOfEntity(const Entity& entity, const bool include_augments = true) const;
    EnumSet<CombatClass> GetAllCombatClassesOfCombatUnit(
        const CombatUnitTypeID& unit_type_id,
        const std::vector<AugmentInstanceData>& equipped_augments) const;
    EnumSet<CombatClass> GetAllCombatClasses(
        const CombatClass combat_class,
        const CombatClass dominant_combat_class,
        const std::vector<AugmentInstanceData>& equipped_augments) const;

    EnumSet<CombatAffinity> GetAllCombatAffinitiesOfEntity(const Entity& entity, const bool include_augments = true)
        const;
    EnumSet<CombatAffinity> GetAllCombatAffinitiesOfCombatUnit(
        const CombatUnitTypeID& unit_type_id,
        const std::vector<AugmentInstanceData>& equipped_augments) const;
    EnumSet<CombatAffinity> GetAllCombatAffinities(
        const CombatAffinity combat_affinity,
        const CombatAffinity dominant_combat_affinity,
        const std::vector<AugmentInstanceData>& equipped_augments) const;

    // Gets all the primary/composite parts for this combat_synergy
    // If combat_synergy is just a primary synergy this function just returns: combat_synergy
    // If combat_synergy just a composite synergy it returns: combat_synergy + the synergy components of combat_synergy
    static std::vector<CombatSynergyBonus> GetAllPartsOfCombatSynergy(
        const GameDataContainer& game_data,
        const CombatSynergyBonus combat_synergy);
    std::vector<CombatSynergyBonus> GetAllPartsOfCombatSynergy(const CombatSynergyBonus combat_synergy) const;

    // Does this CombatSynergyBonus contains this search_combat_synergy?
    // Example:
    // combat_synergy = Magma, Magma is Earth + Fire
    // search_combat_synergy = Earth
    // For this case it would return true because Earth is part of the Magma
    static bool CheckCombatSynergyComposition(
        const GameDataContainer& game_data,
        const CombatSynergyBonus combat_synergy,
        const CombatSynergyBonus search_combat_synergy);
    bool CheckCombatSynergyComposition(
        const CombatSynergyBonus combat_synergy,
        const CombatSynergyBonus search_combat_synergy) const;

    // Ensures that all augments have valid synergies.
    bool CheckSynergiesInAugments() const;

    // Complements the set with primary combat class for each composite one in the input set
    static EnumSet<CombatClass> ComplementWithPrimarySynergies(
        const GameDataContainer& game_data,
        EnumSet<CombatClass> combat_classes);
    EnumSet<CombatClass> ComplementWithPrimarySynergies(EnumSet<CombatClass> combat_classes) const;

    // Complements the set with primary combat affinities for each composite one in the input set
    static EnumSet<CombatAffinity> ComplementWithPrimarySynergies(
        const GameDataContainer& game_data,
        EnumSet<CombatAffinity> combat_affinities);
    EnumSet<CombatAffinity> ComplementWithPrimarySynergies(EnumSet<CombatAffinity> combat_affinities) const;

    // Overload for CombatClass (see GetAllPartsOfCombatSynergy).
    static std::vector<CombatClass> GetAllPartsOfCombatClass(
        const GameDataContainer& game_data,
        const CombatClass combat_class);
    std::vector<CombatClass> GetAllPartsOfCombatClass(const CombatClass combat_class) const;

    // Overload for CombatAffinity (see GetAllPartsOfCombatSynergy).
    static std::vector<CombatAffinity> GetAllPartsOfCombatAffinity(
        const GameDataContainer& game_data,
        const CombatAffinity combat_affinity);
    std::vector<CombatAffinity> GetAllPartsOfCombatAffinity(const CombatAffinity combat_affinity) const;

    // Bond first + second and return the result
    // NOTE: For bonding to work both first and second need to be primary synergies
    //  https://illuvium.atlassian.net/wiki/spaces/AB/pages/3440924/Bond
    static CombatSynergyBonus Bond(
        const GameDataContainer& game_data,
        const CombatSynergyBonus first,
        const CombatSynergyBonus second,
        Logger& logger);
    CombatSynergyBonus Bond(const CombatSynergyBonus first, const CombatSynergyBonus second) const;

    // Find synergy data by combat synergy bonus
    static std::shared_ptr<const SynergyData> FindCombatSynergyData(
        const GameDataContainer& game_data,
        const CombatSynergyBonus& combat_synergy);
    std::shared_ptr<const SynergyData> FindSynergyData(const CombatSynergyBonus& combat_synergy) const;

private:
    // Owner world of this helper
    World* world_ = nullptr;
};

}  // namespace simulation
