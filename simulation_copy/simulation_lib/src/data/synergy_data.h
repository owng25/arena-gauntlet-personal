#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "data/ability_data.h"
#include "data/combat_synergy_bonus.h"
#include "utility/vector_helper.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * SynergyData
 *
 * This class holds the data that defines a synergy. A synergy is an effect (or effects) granted
 * when multiple Combat Units in the same team share a Combat Class or an Combat Affinity. This
 * class captures details like what ability is granted, how many Combat Units are required for the
 * effect to be granted, and how the effect is scaled when more members of the team share the
 * ability.
 * --------------------------------------------------------------------------------------------------------
 */
class SynergyData
{
public:
    // Create a new instance
    static std::shared_ptr<SynergyData> Create()
    {
        return std::make_shared<SynergyData>();
    }

    std::shared_ptr<SynergyData> CreateDeepCopy() const
    {
        // Copy everything
        auto copy = std::make_shared<SynergyData>(*this);

        // Create a copy of the abilities
        copy->unit_threshold_abilities.clear();
        for (const auto& [threshold, abilities_data] : unit_threshold_abilities)
        {
            for (const std::shared_ptr<const AbilityData>& ability_data : abilities_data)
            {
                copy->unit_threshold_abilities[threshold].emplace_back(ability_data->CreateDeepCopy());
            }
        }

        copy->team_threshold_abilities.clear();
        for (const auto& [threshold, abilities_data] : team_threshold_abilities)
        {
            for (const std::shared_ptr<const AbilityData>& ability_data : abilities_data)
            {
                copy->team_threshold_abilities[threshold].emplace_back(ability_data->CreateDeepCopy());
            }
        }

        copy->intrinsic_abilities.clear();
        for (const std::shared_ptr<const AbilityData>& ability_data : intrinsic_abilities)
        {
            copy->intrinsic_abilities.emplace_back(ability_data->CreateDeepCopy());
        }

        copy->hyper_abilities.clear();
        for (const std::shared_ptr<const AbilityData>& ability_data : hyper_abilities)
        {
            copy->hyper_abilities.emplace_back(ability_data->CreateDeepCopy());
        }

        return copy;
    }

    // Whether this is a composition of multiple affinities/classes
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/228491267/Composite+Synergy
    bool IsComposite() const
    {
        return !combat_synergy_components.empty();
    }

    // A Synergy that has only a single element.
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/228425744/Primary+Synergy
    bool IsPrimary() const
    {
        return combat_synergy_components.empty();
    }

    // Is this value a combat class or a combat affinity?
    bool IsCombatClass() const
    {
        return combat_synergy.IsCombatClass();
    }
    bool IsCombatAffinity() const
    {
        return combat_synergy.IsCombatAffinity();
    }

    // Gets the Combat Class/Affinity that this Synergy data is for
    CombatClass GetCombatClass() const
    {
        return combat_synergy.GetClass();
    }
    CombatAffinity GetCombatAffinity() const
    {
        return combat_synergy.GetAffinity();
    }

    // Gets the Components as a vector of CombatClasses
    std::vector<CombatClass> GetComponentsAsCombatClasses() const
    {
        std::vector<CombatClass> combat_classes;
        for (const CombatSynergyBonus& synergy_bonus : combat_synergy_components)
        {
            if (synergy_bonus.IsCombatClass())
            {
                combat_classes.push_back(synergy_bonus.GetClass());
            }
        }
        return combat_classes;
    }

    // Gets the Components as a EnumSet of CombatClasses
    EnumSet<CombatClass> GetUniqueComponentsAsCombatClasses() const
    {
        EnumSet<CombatClass> combat_classes;
        for (const CombatSynergyBonus& synergy_bonus : combat_synergy_components)
        {
            if (synergy_bonus.IsCombatClass())
            {
                combat_classes.Add(synergy_bonus.GetClass());
            }
        }
        return combat_classes;
    }

    // Gets the Components as a vector of CombatClasses
    std::vector<CombatAffinity> GetComponentsAsCombatAffinities() const
    {
        std::vector<CombatAffinity> combat_affinities;
        for (const CombatSynergyBonus& synergy_bonus : combat_synergy_components)
        {
            if (synergy_bonus.IsCombatAffinity())
            {
                combat_affinities.push_back(synergy_bonus.GetAffinity());
            }
        }
        return combat_affinities;
    }

    // Gets the Components as a EnumSet of CombatClasses
    EnumSet<CombatAffinity> GetUniqueComponentsAsCombatAffinities() const
    {
        EnumSet<CombatAffinity> combat_affinities;
        for (const CombatSynergyBonus& synergy_bonus : combat_synergy_components)
        {
            if (synergy_bonus.IsCombatAffinity())
            {
                combat_affinities.Add(synergy_bonus.GetAffinity());
            }
        }
        return combat_affinities;
    }

    // Does the components include this CombatSynergy?
    bool ComponentsContainsCombatSynergy(const CombatSynergyBonus search_combat_synergy) const
    {
        return VectorHelper::ContainsValue(combat_synergy_components, search_combat_synergy);
    }

    // Find the max synergy threshold value for given stacks number
    int FindMaxThresholdForStacks(int stacks) const
    {
        for (int index = static_cast<int>(synergy_thresholds.size()) - 1; index >= 0; index--)
        {
            const int synergy_threshold = synergy_thresholds[static_cast<size_t>(index)];
            if (stacks >= synergy_threshold)
            {
                return synergy_threshold;
            }
        }
        return 0;
    }

    // The combat synergy, can be either a combat class or a combat affinity
    CombatSynergyBonus combat_synergy;

    // Components of the combat_synergy. If the vector is empty then it's a primary combat affinity/class.
    std::vector<CombatSynergyBonus> combat_synergy_components;

    // How many Combat Units are required to reach each threshold, the length of this array
    // implies how many stages there are for this Synergy. For example if you get a
    // synergy effect at 3 Combat Units and 5 Combat Units this vector would hold: [3,5].
    //
    // See: https://illuvium.atlassian.net/wiki/spaces/AB/pages/108167405/Synergy+Threshold
    std::vector<int> synergy_thresholds;

    // The list of innate abilities that depend on the synergy threshold and will be given to each combat unit with
    // this particular synergy
    // Key: Synergy threshold value. This value should also exist inside synergy_thresholds
    // Value: the abilities data for the this specific threshold
    std::unordered_map<int, std::vector<std::shared_ptr<const AbilityData>>> unit_threshold_abilities;

    // The list of innate abilities that depend on the synergy threshold and will be given to
    // synergy entity (one instance for the whole team)
    // Key: Synergy threshold value. This value should also exist inside synergy_thresholds
    // Value: the abilities data for the this specific threshold
    std::unordered_map<int, std::vector<std::shared_ptr<const AbilityData>>> team_threshold_abilities;

    // The list of innate abilities that always apply to the unit with this combat_synergy,
    // irrelevant of the synergy stacks or thresholds.
    std::vector<std::shared_ptr<const AbilityData>> intrinsic_abilities;

    // Hyper related abilities
    std::vector<std::shared_ptr<const AbilityData>> hyper_abilities;

    // Should the intrinsic_abilities be disabled if the first threshold is reached?
    bool disable_intrinsic_abilities_on_first_threshold = false;

    // We consider Synergies equal if and only if they have the same name
    // It is a simple convention to allow us to easily sort them
    bool operator==(const SynergyData& other) const
    {
        return combat_synergy == other.combat_synergy;
    }
};
}  // namespace simulation
