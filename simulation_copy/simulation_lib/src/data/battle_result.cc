#include "battle_result.h"

#include "data/loaders/json_keys.h"
#include "utility/enum.h"
#include "utility/version.h"

namespace simulation
{
nlohmann::json BattleEntityResult::ToJSONObject() const
{
    nlohmann::json state_json;
    state_json["EntityID"] = entity_id;
    state_json["UniqueID"] = unique_id;
    state_json[JSONKeys::kPosition] = position.ToJSONObject();
    state_json[JSONKeys::kTeam] = Enum::TeamToString(team);
    state_json["Health"] = current_health.AsInt();
    state_json["MaxHealth"] = max_health.AsInt();
    state_json["Energy"] = current_energy.AsInt();
    state_json["HistoryData"] = HistoryDataToJSONObject(history_data);
    state_json["FaintedTimeStep"] = fainted_time_step;

    return state_json;
}

nlohmann::json BattleEntityResult::HistoryDataToJSONObject(const StatsHistoryData& history_data)
{
    nlohmann::json json_data;
    json_data["TotalDamageReceived"] = history_data.total_damage_received.AsInt();
    json_data["PhysicalDamageReceived"] = history_data.physical_damage_received.AsInt();
    json_data["EnergyDamageReceived"] = history_data.energy_damage_received.AsInt();
    json_data["PureDamageReceived"] = history_data.pure_damage_received.AsInt();

    json_data["TotalDamageSent"] = history_data.total_damage_sent.AsInt();
    json_data["PhysicalDamageSent"] = history_data.physical_damage_sent.AsInt();
    json_data["EnergyDamageSent"] = history_data.energy_damage_sent.AsInt();
    json_data["PureDamageSent"] = history_data.pure_damage_sent.AsInt();

    json_data["CountFaintVanquishes"] = history_data.count_faint_vanquishes;
    json_data["CountFaintAssists"] = history_data.count_faint_assists;

    return json_data;
}

nlohmann::json BattleWorldResult::ToJSONObject() const
{
    nlohmann::json json;
    json["WinningTeam"] = Enum::TeamToString(winning_team);
    json["DurationTimeSteps"] = duration_time_steps;

    static constexpr std::string_view key_end_state = "EndState";
    json[key_end_state] = nlohmann::json::array();
    for (const BattleEntityResult& entity_state : combat_units_end_state)
    {
        json[key_end_state].push_back(entity_state.ToJSONObject());
    }

    return json;
}

nlohmann::json BattleMultipleWorldResults::ToJSONObject() const
{
    nlohmann::json root_object = nlohmann::json::object();

    nlohmann::json results_array = nlohmann::json::array();
    for (const BattleWorldResult& result : results)
    {
        results_array.push_back(result.ToJSONObject());
    }
    root_object["Version"] = GetVersion();
    root_object["Results"] = results_array;

    return root_object;
}

}  // namespace simulation
