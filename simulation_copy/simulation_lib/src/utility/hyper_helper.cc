#include "utility/hyper_helper.h"

#include "components/combat_synergy_component.h"
#include "components/position_component.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"

namespace simulation
{

bool HyperHelper::AreInHyperRange(const Entity& first, const Entity& second, const HyperConfig& config)
{
    // Check if in range for hyper
    const auto& first_position_component = first.Get<PositionComponent>();
    const auto& second_position_component = second.Get<PositionComponent>();
    return ArePositionsInHyperRange(
        first_position_component.GetPosition(),
        second_position_component.GetPosition(),
        config);
}

bool HyperHelper::ArePositionsInHyperRange(
    const HexGridPosition& first,
    const HexGridPosition& second,
    const HyperConfig& config)
{
    // Get vector between positions
    const HexGridPosition vector_to_other = first - second;

    // Add 1 to the radius because range=0 means right next to each other.
    return vector_to_other.Length() < config.enemies_range_units + 1;
}

int HyperHelper::CalculateHyperGrowthEffectiveness(const Entity& entity)
{
    int effectiveness = 0;
    const World& world = *entity.GetOwnerWorld();

    world.SafeWalkAllEnemiesOfEntity(
        entity,
        [&](const Entity& opponent_entity)
        {
            int contribution = 0;
            if (!GetHyperContributionOfEnemyCombatUnit(entity, opponent_entity, &contribution))
            {
                return;
            }

            effectiveness += contribution;
        });

    return effectiveness;
}

int HyperHelper::CalculateHyperGrowthEffectiveness(const World& world, const EntityID entity_id)
{
    if (const std::shared_ptr<Entity>& entity_ptr = world.GetByIDPtr(entity_id))
    {
        if (entity_ptr->IsActive())
        {
            return CalculateHyperGrowthEffectiveness(*entity_ptr);
        }
    }

    return 0;
}

bool HyperHelper::GetHyperContributionOfEnemyCombatUnit(
    const Entity& entity,
    const Entity& enemy_entity,
    int* out_contribution)
{
    const World& world = *entity.GetOwnerWorld();
    const HyperConfig& hyper_config = world.GetHyperConfig();
    const HyperData& hyper_data = world.GetGameDataContainer().GetHyperData();

    if (!AreInHyperRange(entity, enemy_entity, hyper_config))
    {
        return false;
    }

    // Get dominant combat affinity
    const auto& combat_synergy_component = entity.Get<CombatSynergyComponent>();
    const CombatAffinity dominant_combat_affinity = combat_synergy_component.GetDominantCombatAffinity();

    // Get enemy dominant combat affinity
    const auto& enemy_combat_synergy_component = enemy_entity.Get<CombatSynergyComponent>();
    const CombatAffinity enemy_dominant_combat_affinity = enemy_combat_synergy_component.GetDominantCombatAffinity();

    *out_contribution =
        hyper_data.GetCombatAffinityEffectiveness(dominant_combat_affinity, enemy_dominant_combat_affinity);

    return true;
}

}  // namespace simulation
