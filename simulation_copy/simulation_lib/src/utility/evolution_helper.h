#pragma once

#include <unordered_set>
#include <vector>

#include "data/combat_unit_type_id.h"
#include "data/containers/combat_units_data_container.h"

namespace simulation
{
class CombatUnitData;

class EvolutionHelper
{
public:
    static void FindPossibleEvolutionPaths(
        const CombatUnitsDataContainer& combat_units_data_container,
        const CombatUnitTypeID& type_id,
        std::vector<const CombatUnitData*>* out_evolution_paths);

    static std::vector<const CombatUnitData*> FindPossibleEvolutionPaths(
        const CombatUnitsDataContainer& combat_units_data_container,
        const CombatUnitTypeID& type_id)
    {
        std::vector<const CombatUnitData*> evolution_paths;
        FindPossibleEvolutionPaths(combat_units_data_container, type_id, &evolution_paths);
        return evolution_paths;
    }

    // DFS evolution evolution graph down starting from specified combat unit.
    // Invokes callback on each element of the graph including the root one.
    // Returns false if could not find data for any of visited units.
    // Note: does not walks over ancestors.
    // Callback signature: void (const CombatUnitData&)
    template <typename Callback>
    static bool WalkDownEvolutionTree(
        const CombatUnitsDataContainer& combat_units_data_container,
        const CombatUnitTypeID& root_type_id,
        Callback&& callback)
    {
        std::vector<const CombatUnitData*> queue;
        std::unordered_set<const CombatUnitData*> visited;

        {
            auto root_unit_data = combat_units_data_container.Find(root_type_id);
            queue.push_back(root_unit_data.get());
            if (queue.back() == nullptr) return false;
        }

        while (!queue.empty())
        {
            const CombatUnitData* current_unit_data = queue.back();
            queue.pop_back();

            // Already visited
            if (visited.count(current_unit_data) != 0) continue;

            visited.insert(current_unit_data);

            callback(*current_unit_data);

            FindPossibleEvolutionPaths(combat_units_data_container, current_unit_data->type_id, &queue);
        }

        return true;
    }

    static bool FindMaximumPossibleRadiusInEvolutionGraph(
        const CombatUnitsDataContainer& combat_units_data_container,
        const CombatUnitTypeID& root_type_id,
        int* out_radius)
    {
        *out_radius = 0;
        const auto visit_node = [&](const CombatUnitData& unit_data)
        {
            *out_radius = (std::max)(*out_radius, unit_data.radius_units);
        };
        return WalkDownEvolutionTree(combat_units_data_container, root_type_id, visit_node);
    }
};

}  // namespace simulation
