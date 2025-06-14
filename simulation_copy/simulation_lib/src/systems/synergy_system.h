#pragma once

#include <unordered_set>

#include "data/synergy_data.h"
#include "ecs/system.h"

namespace simulation
{
class Event;
class AugmentInstanceData;
/* -------------------------------------------------------------------------------------------------------
 * SynergySystem
 *
 * This class handles synergies
 * --------------------------------------------------------------------------------------------------------
 */
class SynergySystem : public System
{
    typedef System Super;
    typedef SynergySystem Self;

public:
    void Init(World* world) override;
    void PreBattleStarted(const Entity& entity) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSynergy;
    }

    // When a combat unit is added, it should also be added to synergy combat classes & combat
    // affinities
    void AddEntity(const Entity& entity);

    // When a synergy entity for a team is added, team abilities should be added to it depending of synergy combat
    // classes & combat affinities of the whole team
    void AddTeamAbilities(const Entity& entity) const;

    // When a combat unit is removed, it should also be removed from synergy combat classes & combat
    // affinities
    void RefreshCombatSynergies();

    void AddSynergiesFromAugment(const Entity& entity, const AugmentInstanceData& instance);
    void AddSynergiesFromAugments(const Entity& entity);

private:
    // Helper function that adds the synergy data to the entity
    void AddSynergyDataToEntity(const Entity& entity);

    // Listen to world events
    void OnBattleStarted(const Event& event);

    // Helper function to get all the innate abilities for units
    bool GetInnateAbilitiesFromSynergies(
        const Entity& entity,
        std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const;

    // Helper function to get all the abilities for a team
    void GetTeamAbilitiesFromSynergies(
        const Team team,
        std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const;

    // Helper function to add the synergy threshold abilities.
    void AddUnitAbilitiesForSynergy(
        const int synergy_stacks,
        const SynergyData& synergy_data,
        std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const;

    // This just adds unit abilities for the synergy thresholds
    bool AddUnitAbilitiesForSynergyThresholds(
        const int synergy_stacks,
        const SynergyData& synergy_data,
        std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const;

    // This just adds team abilities for the synergy thresholds
    bool AddTeamAbilitiesForSynergyThresholds(
        const int synergy_stacks,
        const SynergyData& synergy_data,
        std::vector<std::shared_ptr<const AbilityData>>& out_innate_abilities) const;

    // General method to add a combat class or affinity
    void AddCombatSynergy(const Entity& entity, const CombatSynergyBonus& base_combat_synergy);

    // Method to add a combat class or affinity from augment
    void AddCombatSynergyFromAugment(const Entity& entity, const CombatSynergyBonus& base_combat_synergy);

    // Method that gets the entity's combat synergies and adds them to synergy vector if they are valid
    void AddCombatSynergies(const Entity& entity);

    // Keep track of entities that had AddSynergyDataToEntity called
    std::unordered_set<EntityID> entities_activated_;
};

}  // namespace simulation
