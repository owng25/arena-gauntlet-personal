#!/usr/bin/env python3
# NOTE: This script is standalone does not include any libraries

import json
import os
import argparse
import copy
from pathlib import Path

BATTLE_FILE_VERSION = 23
KEY_WEAPON_TYPE = "WeaponType"
KEY_SUIT_TYPE = "SuitType"

KEY_SORT_BY_UNIQUE_ID = "SortByUniqueID"
KEY_GRID_WIDTH = "GridWidth"
KEY_GRID_HEIGHT = "GridHeight"
KEY_GRID_SCALE = "GridScale"
KEY_ENABLE_EVOLUTION = "EnableEvolution"
KEY_USE_MAX_STAGE_PLACEMENT_RADIUS = "UseMaxStagePlacementRadius"
KEY_ENCOUNTER_MODS = "EncounterMods"

DEFAULT_BATTLE_CONFIG =	 {
    "RandomSeed": 0,
    "SortByUniqueID": True,
    "EnableEvolution": False,
    "UseMaxStagePlacementRadius": False,
    "GridWidth": 105,
    "GridHeight": 111,
    "GridScale": 10,
    "MiddleLineWidth": 0,
    "AugmentsConfig":
    {
        "MaxAugments": 2
    },
    "ConsumablesConfig":
    {
        "MaxConsumables": 2
    },
    "OverloadConfig":
    {
        "EnableOverloadSystem": True,
        "StartSecondsApplyOverloadDamage": 45,
        "IncreaseOverloadDamagePercentage": 5
    },
    "LevelingConfig":
    {
        "EnableLevelingSystem": True,
        "LevelingGrowRatePercentage": 1
    },
    "EncounterMods": {}
}

# Read a single JSON file
def read_file(file_name):
    f = open(file_name)
    data = json.load(f)
    f.close()
    return data


def write_file(filename, data):
    with open(str(filename), "wb") as f:
        f.write(data.encode("utf-8"))

def get_combat_units_list(battle_data_placements):
    combat_units_data = []
    for src_placement in battle_data_placements:

        # Position has the same structure
        dst_combat_unit = {}
        dst_combat_unit["Position"] = src_placement["Position"]
        dst_combat_unit["TypeID"] = src_placement["TypeID"]

        # Fill the instance
        dst_combat_unit["Instance"] = {
            "ID": src_placement["ID"],
            "BondedID": src_placement.get("BondedID", ""),
            "Level": src_placement.get("Level", 1),
            "Finish": src_placement.get("Finish", "None"),
            "DominantCombatAffinity": src_placement.get("DominantCombatAffinity", "None"),
            "DominantCombatClass": src_placement.get("DominantCombatClass", "None"),
            "BondedCombatUnitIndex": src_placement.get("BondedCombatUnitIndex", -1),
            "EquippedAugments": src_placement.get("EquippedAugments", []),
            "EquippedConsumables": src_placement.get("EquippedConsumables", []),
            "StatsOverrides": src_placement.get("StatsOverrides", {}),
            "RandomPercentageModifierStats": src_placement.get("RandomPercentageModifierStats", {}),
            "EquippedWeapon": src_placement.get("EquippedWeapon", {
                    "ID": "",
                    "TypeID":
                    {
                        "Name": "",
                        "Stage": 0,
                        "Variation": "",
                        "CombatAffinity": "None"
                    }
                }),
            "EquippedSuit": src_placement.get("EquippedSuit", {
                    "TypeID":
                    {
                        "Name": "",
                        "Stage": 0,
                        "Variation": ""
                    },
                    "ID": ""
                }),

        }

        combat_units_data.append(dst_combat_unit)

    return combat_units_data

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source_file", type=str, help="Source file")
    parser.add_argument("destination_file_base_name", type=str, help="Destination file base name")
    args = parser.parse_args()

    source_file = Path(args.source_file)

    source_json_data = read_file(source_file)

    # Override with values from source, it is shared between all battles
    battle_config = copy.deepcopy(DEFAULT_BATTLE_CONFIG)
    battle_config_keys_source = [
        KEY_SORT_BY_UNIQUE_ID,
        KEY_GRID_WIDTH,
        KEY_GRID_HEIGHT,
        KEY_GRID_SCALE,
        KEY_ENABLE_EVOLUTION,
        KEY_USE_MAX_STAGE_PLACEMENT_RADIUS,
        KEY_ENCOUNTER_MODS
    ]
    for json_key in battle_config_keys_source:
        if json_key not in source_json_data:
            continue

        battle_config[json_key] = source_json_data[json_key]


    count_battle = len(source_json_data["BattleData"])
    destination_file_base_name = args.destination_file_base_name
    for i, battle_json in enumerate(source_json_data["BattleData"]):

        combat_units_data = get_combat_units_list(battle_json["Placements"])

        # Output structure
        output_data = {
            "Version": BATTLE_FILE_VERSION,
            "CombatUnits": combat_units_data,
            "BattleConfig": battle_config
        }

        # Output formatted data
        output_json_string = json.dumps(output_data, sort_keys=True, indent=4)


        if count_battle == 1:
            destination_file = Path(f"Client_{destination_file_base_name}.json")
        else:
            destination_file = Path(f"Client_{destination_file_base_name}_{i}.json")

        # print(output_json_string + "\n")
        print(f"Written output to: {destination_file}")
        write_file(destination_file, output_json_string)
