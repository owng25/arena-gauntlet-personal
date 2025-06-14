#pragma once

#include <vector>

#include "ecs/world.h"

namespace simulation::tool
{

// One unit state loaded from battle file
struct BattleCombatUnitState
{
    simulation::CombatUnitTypeID type_id;
    simulation::CombatUnitInstanceData instance;
    simulation::HexGridPosition position;
};

struct DroneAugmentState
{
    simulation::Team team = Team::kNone;
    simulation::DroneAugmentTypeID type_id;
};

// State of the board loaded from battle file
struct BattleBoardState
{
    int version = 0;
    simulation::BattleConfig battle_config;
    std::vector<BattleCombatUnitState> combat_units;
    std::vector<DroneAugmentState> drone_augments;
};
}  // namespace simulation::tool
