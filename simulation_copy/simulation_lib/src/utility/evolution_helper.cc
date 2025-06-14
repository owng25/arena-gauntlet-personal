#include "evolution_helper.h"

#include "data/containers/combat_units_data_container.h"

namespace simulation
{
void EvolutionHelper::FindPossibleEvolutionPaths(
    const CombatUnitsDataContainer& combat_units_data_container,
    const CombatUnitTypeID& type_id,
    std::vector<const CombatUnitData*>* out_evolution_paths)
{
    const auto on_unit_data = [&](const CombatUnitData& other_unit_data)
    {
        const auto& other_type_id = other_unit_data.type_id;
        if (other_type_id.type != CombatUnitType::kIlluvial) return;
        if (other_type_id.line_name != type_id.line_name) return;
        if (other_type_id.line_name != "Lynx" && other_type_id.path != type_id.path) return;
        if (other_type_id.stage != type_id.stage + 1) return;

        out_evolution_paths->push_back(&other_unit_data);
    };

    combat_units_data_container.ForEach(on_unit_data);
}

}  // namespace simulation
