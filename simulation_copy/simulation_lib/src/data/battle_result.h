#pragma once

#include <string>
#include <vector>

#include "constants.h"
#include "data/enums.h"
#include "nlohmann/json.hpp"
#include "stats_data.h"
#include "utility/hex_grid_position.h"

namespace simulation
{

// Holds the entity result at the end of the battle.
struct BattleEntityResult
{
    // The entity id
    EntityID entity_id = kInvalidEntityID;

    // The unique id
    std::string unique_id;

    // The whom this entity belongs?
    Team team = Team::kNone;

    // Last known position
    HexGridPosition position;

    // Keep initial max health value in results data, so it will be easier to compare
    FixedPoint max_health = 0_fp;

    // Last known current stats of this entity
    FixedPoint current_health = 0_fp;
    FixedPoint current_energy = 0_fp;

    // Keep history of some stats
    StatsHistoryData history_data;

    // The time step when this unit fainted
    int fainted_time_step = 0;

    // Convert to a JSON object this struct
    nlohmann::json ToJSONObject() const;

    // Convert StatsHistoryData to a JSON object
    static nlohmann::json HistoryDataToJSONObject(const StatsHistoryData& history_data);
};

// Holds the world result at the end of the battle.
struct BattleWorldResult
{
    // The team that won
    Team winning_team = Team::kNone;

    // How many steps it took to reach the battle finished state
    int duration_time_steps = 0;

    // The state of all combat units at the end of the battle
    std::vector<BattleEntityResult> combat_units_end_state;

    // Convert to a JSON object this struct
    nlohmann::json ToJSONObject() const;
};

// Holds a vector of BattleWorldResult
struct BattleMultipleWorldResults
{
    std::vector<BattleWorldResult> results;

    // Convert to a JSON object this struct
    nlohmann::json ToJSONObject() const;
};

}  // namespace simulation
