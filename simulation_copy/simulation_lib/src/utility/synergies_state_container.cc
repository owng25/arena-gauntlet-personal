#include "synergies_state_container.h"

#include "components/combat_synergy_component.h"
#include "data/containers/game_data_container.h"
#include "ecs/entity.h"
#include "utility/vector_helper.h"

namespace simulation
{
std::shared_ptr<SynergiesStateContainer> SynergiesStateContainer::Create(
    std::shared_ptr<Logger> logger,
    std::shared_ptr<const GameDataContainer> game_data)
{
    // NOTE: can't access private constructor with make_shared
    auto container = std::make_shared<SynergiesStateContainer>();

    container->SetLogger(std::move(logger));
    container->game_data_ = std::move(game_data);

    return container;
}

const std::vector<SynergyOrder>& SynergiesStateContainer::GetTeamAllSynergies(
    const Team team,
    const bool is_battle_started) const
{
    // Does not exist
    static std::vector<SynergyOrder> empty_vector;
    if (!combat_all_.Contains(team))
    {
        return empty_vector;
    }

    // Game did not start yet or we have no synergies
    std::vector<SynergyOrder>& all_synergies_order = combat_all_.Get(team);
    if (is_battle_started && !all_synergies_order.empty())
    {
        return all_synergies_order;
    }

    // Start fresh
    all_synergies_order.clear();

    // Affinities
    for (const auto& combat_affinity : GetTeamAllCombatAffinitySynergies(team))
    {
        all_synergies_order.push_back(combat_affinity);
    }

    // Classes
    for (const auto& combat_class : GetTeamAllCombatClassSynergies(team))
    {
        all_synergies_order.push_back(combat_class);
    }

    // Sort
    SortVectorSynergyOrders(all_synergies_order);

    return all_synergies_order;
}

bool SynergiesStateContainer::AddCombatSynergy(
    const Team team,
    const EntityID entity_id,
    const bool is_from_augment,
    const CombatUnitTypeID& add_type_id,
    const CombatSynergyBonus& base_combat_synergy,
    TeamsSynergiesOrderType& out_team_combat_synergies)
{
    if (base_combat_synergy.IsEmpty())
    {
        logger_->LogInfo(
            "SynergiesStateContainer::AddCombatSynergy - entity_id = {}, CombatSynergyBonus does not have any Value!",
            entity_id);
        return false;
    }

    // Since we moved to simplified synergies we only add the base combat synergy and ignore the components
    const std::vector<CombatSynergyBonus> combat_synergies_to_add{base_combat_synergy};

    // Iterate over existing synergies order
    std::vector<SynergyOrder>& out_combat_synergy_data = out_team_combat_synergies.GetOrAdd(team);

    // Fill all cache
    combat_all_.GetOrAdd(team) = {};

    // Add all the combat synergies
    std::unordered_set<int> processed_synergies;
    for (const CombatSynergyBonus& synergy_to_add : combat_synergies_to_add)
    {
        const int synergy_to_add_int = synergy_to_add.GetIntValue();

        // Already processed.
        // Some Combat Synergies can be duplicate
        // For example Berserker is Fighter + Fighter
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/108232948/Synergy+Stack
        if (processed_synergies.count(synergy_to_add_int) > 0)
        {
            continue;
        }

        // Mark as processed
        processed_synergies.insert(synergy_to_add_int);

        // Check for existing synergy count
        const bool found = AddCheckExistingCombatSynergy(
            entity_id,
            is_from_augment,
            add_type_id,
            base_combat_synergy,
            synergy_to_add,
            combat_synergies_to_add,
            out_combat_synergy_data);

        // Only add if a value does not exist already
        if (!found)
        {
            // Count the stacks
            const int stacks_to_add = VectorHelper::CountValue(combat_synergies_to_add, synergy_to_add);

            SynergyOrder synergy_order;
            synergy_order.combat_synergy = synergy_to_add;
            if (!is_from_augment)
            {
                synergy_order.combat_units.emplace_back(
                    SynergyOrderCombatUnit{.type_id = add_type_id, .base_combat_synergy = base_combat_synergy});
            }
            synergy_order.synergy_stacks = stacks_to_add;
            out_combat_synergy_data.emplace_back(synergy_order);

            logger_->LogDebug(
                "SynergiesStateContainer::AddCombatSynergy - entity_id = {}, is_from_augment = {}. ADDED - type_id = "
                "{}, "
                "combat_synergy = {}, total_new_stacks = {}",
                entity_id,
                is_from_augment,
                add_type_id,
                synergy_to_add,
                synergy_order.synergy_stacks);
        }
    }

    // Only sort descending if we have 2 or more elements
    if (out_combat_synergy_data.size() > 1)
    {
        SortVectorSynergyOrders(out_combat_synergy_data);
    }

    return true;
}

bool SynergiesStateContainer::AddCheckExistingCombatSynergy(
    const EntityID entity_id,
    const bool is_from_augment,
    const CombatUnitTypeID& add_type_id,
    const CombatSynergyBonus& base_combat_synergy,
    const CombatSynergyBonus& synergy_to_add,
    const std::vector<CombatSynergyBonus>& combat_synergies_to_add,
    std::vector<SynergyOrder>& out_combat_synergy_data) const
{
    SynergyOrder* p_synergy_order = nullptr;

    // Find existing ones
    for (SynergyOrder& synergy_order : out_combat_synergy_data)
    {
        if (synergy_to_add == synergy_order.combat_synergy)
        {
            p_synergy_order = &synergy_order;
            break;
        }
    }

    if (p_synergy_order == nullptr)
    {
        return false;
    }

    SynergyOrder& synergy_order = *p_synergy_order;

    // Count the stacks
    const int original_stacks_to_add = VectorHelper::CountValue(combat_synergies_to_add, synergy_to_add);
    int stacks_to_add = original_stacks_to_add;
    int num_occurrences = 0;

    if (!is_from_augment)
    {
        const std::string add_combat_unit_line_name = GetCorrectLineNameForTypeID(add_type_id);

        // In case it's not a duplicated unit doesn't exist on the grid with same combat
        // synergy Increase order for the CombatSynergy
        for (const SynergyOrderCombatUnit& existing_combat_unit : synergy_order.combat_units)
        {
            // Exists with the same type id
            if (existing_combat_unit.type_id == add_type_id)
            {
                num_occurrences++;

                // We can't increase the stack number anyways
                break;
            }

            const std::string existing_combat_unit_line_name =
                GetCorrectLineNameForTypeID(existing_combat_unit.type_id);

            // Check synergy overlap
            // https://illuvium.atlassian.net/wiki/spaces/AB/pages/108200183/Synergy+Overlap
            if (existing_combat_unit_line_name == add_combat_unit_line_name &&
                existing_combat_unit.type_id.stage != add_type_id.stage)
            {
                // Check how many stacks we need to subtract
                int stacks_to_subtract = 0;
                if (existing_combat_unit.base_combat_synergy == synergy_to_add)
                {
                    // Just base is common
                    stacks_to_subtract = 1;
                }

                // Reduce how many stacks we want to add for this synergy because the same line
                // already exists
                stacks_to_add = std::max(0, stacks_to_add - stacks_to_subtract);
            }
        }
    }

    if (num_occurrences == 0)
    {
        synergy_order.synergy_stacks += stacks_to_add;
        logger_->LogDebug(
            "SynergiesStateContainer::AddCombatSynergy - entity_id = {}, is_from_augment = {}, UPDATED - "
            "add_type_id: {{{}}}, combat_synergy = {}, "
            "stacks_to_add = {}, total_new_stacks = {}",
            entity_id,
            is_from_augment,
            add_type_id,
            synergy_to_add,
            stacks_to_add,
            synergy_order.synergy_stacks);
    }

    if (!is_from_augment)
    {
        // Store duplicated units as well
        synergy_order.combat_units.emplace_back(
            SynergyOrderCombatUnit{.type_id = add_type_id, .base_combat_synergy = base_combat_synergy});
    }

    // Found
    return true;
}

std::string SynergiesStateContainer::GetCorrectLineNameForTypeID(const CombatUnitTypeID& type_id) const
{
    if (!game_data_->HasCombatUnitData(type_id))
    {
        return type_id.line_name;
    }

    const auto& combat_unit_data = game_data_->GetCombatUnitData(type_id);
    if (type_id.line_name == "Lynx")
    {
        // example: <line_name><Class><Affinity>, LynxFighter, LynxEmpath, LynxFighterEarth, LynxBulwarkNature
        return fmt::format(
            "{}{}{}",
            type_id.line_name,
            Enum::CombatClassToString(combat_unit_data->type_data.combat_class),
            Enum::CombatAffinityToString(combat_unit_data->type_data.combat_affinity));
    }

    // Default to line name
    return type_id.line_name;
}

void SynergiesStateContainer::SortVectorSynergyOrders(std::vector<SynergyOrder>& out_synergies_order)
{
    std::sort(
        std::begin(out_synergies_order),
        std::end(out_synergies_order),
        [](const SynergyOrder& a, const SynergyOrder& b)
        {
            if (a.synergy_stacks == b.synergy_stacks)
            {
                // Split by combat units that have this synergy
                if (a.combat_units.size() == b.combat_units.size())
                {
                    return a.combat_synergy.GetIntSortAbsoluteValue() > b.combat_synergy.GetIntSortAbsoluteValue();
                }

                return a.combat_units.size() > b.combat_units.size();
            }

            return a.synergy_stacks > b.synergy_stacks;
        });
}

}  // namespace simulation
