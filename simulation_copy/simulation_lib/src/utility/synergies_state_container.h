#pragma once

#include <memory>
#include <vector>

#include "data/constants.h"
#include "data/synergy_order.h"
#include "utility/enum_map.h"
#include "utility/logger.h"

namespace simulation
{

class GameDataContainer;

class Entity;

// Helper class that allows us to keep track of the teams synergies at runtime
class SynergiesStateContainer final
{
public:
    // Create a new SynergiesDataContainer from data
    static std::shared_ptr<SynergiesStateContainer> Create(
        std::shared_ptr<Logger> logger,
        std::shared_ptr<const GameDataContainer> game_data_container);

    // Copyable or Movable
    SynergiesStateContainer() = default;
    SynergiesStateContainer(const SynergiesStateContainer&) = default;
    SynergiesStateContainer& operator=(const SynergiesStateContainer&) = default;
    SynergiesStateContainer(SynergiesStateContainer&&) = default;
    SynergiesStateContainer& operator=(SynergiesStateContainer&&) = default;

    void SetLogger(std::shared_ptr<Logger> logger)
    {
        logger_ = std::move(logger);
    }

    // Gets all combat affinities + combat classes from the grid in descending order
    const std::vector<SynergyOrder>& GetTeamAllSynergies(const Team team, const bool is_battle_started) const;

    // Gets all combat classes from the grid in descending order
    const std::vector<SynergyOrder>& GetTeamAllCombatClassSynergies(const Team team) const
    {
        static std::vector<SynergyOrder> empty_vector;
        if (!combat_classes_.Contains(team))
        {
            return empty_vector;
        }

        return combat_classes_.Get(team);
    }

    // Gets all combat affinities from the grid in descending order
    const std::vector<SynergyOrder>& GetTeamAllCombatAffinitySynergies(const Team team) const
    {
        static std::vector<SynergyOrder> empty_vector;
        if (!combat_affinities_.Contains(team))
        {
            return empty_vector;
        }

        return combat_affinities_.Get(team);
    }

    // Gets the synergy stacks for the combat_synergy
    int GetTeamSynergyStacksForCombatSynergy(const Team team, const CombatSynergyBonus combat_synergy) const
    {
        // CombatAffinity
        if (combat_synergy.IsCombatAffinity())
        {
            return GetTeamSynergyStacksForCombatAffinity(team, combat_synergy.GetAffinity());
        }

        // CombatClass
        return GetTeamSynergyStacksForCombatClass(team, combat_synergy.GetClass());
    }

    // Gets the synergy stacks for the combat_class
    int GetTeamSynergyStacksForCombatClass(const Team team, const CombatClass combat_class) const
    {
        if (!combat_classes_.Contains(team))
        {
            return 0;
        }

        const std::vector<SynergyOrder>& combat_class_data = combat_classes_.Get(team);
        for (const SynergyOrder& synergy_order : combat_class_data)
        {
            if (synergy_order.combat_synergy == combat_class)
            {
                return synergy_order.synergy_stacks;
            }
        }

        return 0;
    }

    // Gets the synergy stacks for the combat_affinity
    int GetTeamSynergyStacksForCombatAffinity(const Team team, const CombatAffinity combat_affinity) const
    {
        if (!combat_affinities_.Contains(team))
        {
            return 0;
        }

        const std::vector<SynergyOrder>& combat_affinity_data = combat_affinities_.Get(team);
        for (const SynergyOrder& synergy_order : combat_affinity_data)
        {
            if (synergy_order.combat_synergy == combat_affinity)
            {
                return synergy_order.synergy_stacks;
            }
        }

        return 0;
    }

    // Public method to add a combat class or affinity
    bool AddCombatSynergy(
        const Team team,
        const EntityID entity_id,
        const bool is_from_augment,
        const CombatUnitTypeID& add_type_id,
        const CombatSynergyBonus& base_combat_synergy)
    {
        // CombatClass
        if (base_combat_synergy.IsCombatClass())
        {
            return AddCombatSynergy(
                team,
                entity_id,
                is_from_augment,
                add_type_id,
                base_combat_synergy,
                combat_classes_);
        }

        // CombatAffinity
        return AddCombatSynergy(team, entity_id, is_from_augment, add_type_id, base_combat_synergy, combat_affinities_);
    }

    // Reset all the data
    void Clear()
    {
        combat_affinities_.Clear();
        combat_classes_.Clear();
        combat_all_.Clear();
    }

    // Helper function to sort a vector of synergy orders
    static void SortVectorSynergyOrders(std::vector<SynergyOrder>& out_synergies_order);

private:
    using TeamsSynergiesOrderType = EnumMap<Team, std::vector<SynergyOrder>>;

    // General method to add a combat class or affinity
    bool AddCombatSynergy(
        const Team team,
        const EntityID entity_id,
        const bool is_from_augment,
        const CombatUnitTypeID& add_type_id,
        const CombatSynergyBonus& combat_synergy,
        TeamsSynergiesOrderType& out_team_combat_synergies);

    // Helper function that goes over the existing synergies for synergy_to_add and updates the
    // count appropriately inside out_combat_synergy_data.
    // Return true if updated an existing synergy and false otherwise
    bool AddCheckExistingCombatSynergy(
        const EntityID entity_id,
        const bool is_from_augment,
        const CombatUnitTypeID& add_type_id,
        const CombatSynergyBonus& base_combat_synergy,
        const CombatSynergyBonus& synergy_to_add,
        const std::vector<CombatSynergyBonus>& combat_synergies_to_add,
        std::vector<SynergyOrder>& out_combat_synergy_data) const;

    // Get the correct line name for the type id
    // TODO(vampy): This is a hack to solve https://illuvium.atlassian.net/browse/ARENA-6738
    std::string GetCorrectLineNameForTypeID(const CombatUnitTypeID& type_id) const;

    // Keeps track of the formed CombatClasses based on the combat units on the grid
    // Key: Team
    // Value: Descending Ordered CombatClasses Vector
    TeamsSynergiesOrderType combat_classes_;

    // Keeps track of the formed combat affinities based on the combat units on the grid
    // Key: Team
    // Value: Descending Ordered CombatAffinities Vector
    TeamsSynergiesOrderType combat_affinities_;

    // Helper to Keep track of all CombatClasses and CombatAffinities.
    // NOTE: Because this is used as a cache, this is marked as mutable.
    // Cache is build in GetTeamAllSynergies
    //
    // Key: Team
    // Value: Descending Order of CombatAffinities/CombatClasses;
    mutable TeamsSynergiesOrderType combat_all_;

    // Logger to use for this container
    std::shared_ptr<Logger> logger_ = nullptr;

    // The game data
    std::shared_ptr<const GameDataContainer> game_data_;
};

}  // namespace simulation
