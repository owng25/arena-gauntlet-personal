#!/usr/bin/env python3
""" Converts JSON battle file from unreal format to game server format """

import argparse
from pathlib import Path
from typing import Iterable
from sim_script_utility import *

NONE_STR = "None"


# This class remembers unique objects (augments, synergies, weapons, etc) encountered in combat units
class SimDataCache:
    def __init__(self, json_data_dir: Path):
        self.__json_data_dir = json_data_dir

        self.__synergy_to_path: dict[str, Path] = dict()
        self.__primary_synergies: set[str] = set()
        self.__composite_synergies_components: dict[str, list[str]] = {}
        for synergy_path in (self.json_data_dir / "SynergyData").rglob("*.json"):
            synergy_data = read_json(synergy_path)

            synergy_name = None
            synergy_components = None
            if "CombatAffinity" in synergy_data:
                synergy_name = synergy_data["CombatAffinity"]
                synergy_components = synergy_data["CombatAffinityComponents"]
            else:
                synergy_name = synergy_data["CombatClass"]
                synergy_components = synergy_data["CombatClassComponents"]

            if len(synergy_components) == 0:
                self.__primary_synergies.add(synergy_name)
            else:
                self.__composite_synergies_components[synergy_name] = synergy_components
            self.__synergy_to_path[synergy_name] = synergy_path

        self.__unique_augments: set[str] = set()
        self.__unique_synergies: set[str] = set()

        # weapons
        self.__weapon_identifier_paths: dict[str, Path] = dict()
        self.__unique_weapons: set[str] = set()
        self.__init_item_id_to_path_dict("WeaponData", self.__weapon_identifier_paths, self.__make_weapon_identifier)

        # suits
        self.__suit_identifier_paths: dict[str, Path] = dict()
        self.__unique_suits: set[str] = set()
        self.__init_item_id_to_path_dict("SuitData", self.__suit_identifier_paths, self.__make_suit_identifier)

        # consumables
        self.__consumable_identifier_paths: dict[str, Path] = dict()
        self.__unique_consumables: set[str] = set()
        self.__init_item_id_to_path_dict(
            "ConsumableData", self.__consumable_identifier_paths, self.__make_consumable_identifier
        )

        # augments
        self.__augment_identifier_paths: dict[str, Path] = dict()
        self.__unique_augments: set[str] = set()
        self.__init_item_id_to_path_dict("AugmentData", self.__augment_identifier_paths, self.__make_augment_identifier)

        # encounter mods
        self.__encounter_mods_identifier_paths: dict[str, Path] = dict()
        self.__unique_encounter_mods: set[str] = set()
        self.__init_item_id_to_path_dict(
            "EncounterModData", self.__encounter_mods_identifier_paths, self.__make_encounter_mod_identifier
        )

        # maps unique combat unit identifier to path with combat unit data
        self.__combat_unit_identifier_paths: dict[str, Path] = dict()

        # maps unique combat unit identifier to loaded data from file
        self.__combat_units_data: dict[str, dict] = dict()

        self.__placed_combat_units: list[dict] = list()

        self.__init_combat_units_identifiers()

    @property
    def json_data_dir(self) -> Path:
        return self.__json_data_dir

    @property
    def hyper_data_dir(self) -> Path:
        return self.__json_data_dir / "HyperData"

    def add_combat_units(self, combat_units: Iterable[dict]):
        for combat_unit in combat_units:
            self.add_combat_unit(combat_unit)

    def add_combat_unit(self, combat_unit: dict):
        identifier: str = SimDataCache.__make_combat_unit_identifier(combat_unit["TypeID"])
        self.__register_combat_unit_data(identifier)

        # combat unit instance
        cu_instance = combat_unit["Instance"]

        key_equipped_weapon = "EquippedWeapon"
        if key_equipped_weapon in cu_instance:
            weapon = cu_instance[key_equipped_weapon]
            self.__register_weapon(weapon)

        key_equipped_suit = "EquippedSuit"
        if key_equipped_suit in cu_instance:
            suit = cu_instance[key_equipped_suit]
            self.__register_suit(suit)

        key_equipped_augments = "EquippedAugments"
        if key_equipped_augments in cu_instance:
            for augment in cu_instance["EquippedAugments"]:
                self.__register_augment(augment)

        key_equipped_consumables = "EquippedConsumables"
        if key_equipped_consumables in cu_instance:
            for consumable in cu_instance["EquippedConsumables"]:
                self.__register_consumable(consumable)

        self.__register_synergy(cu_instance["DominantCombatClass"])
        self.__register_synergy(cu_instance["DominantCombatAffinity"])

        # Game client sometimes uses 'position' key instead of 'Position'
        # game server will not accept that so have to fix it here.
        if "position" in combat_unit:
            combat_unit["Position"] = combat_unit["position"]
            del combat_unit["position"]
        cu_position = combat_unit["Position"]
        combat_unit["Team"] = SimDataCache.__determine_team_by_position(cu_position["R"])

        combat_unit.update(combat_unit["Instance"])
        del combat_unit["Instance"]

        self.__placed_combat_units.append(combat_unit)

    def get_placed_combat_units(self) -> list[dict]:
        return self.__placed_combat_units

    def get_combat_units_data(self) -> list[dict]:
        return [unit_data for _, unit_data in self.__combat_units_data.items()]

    def get_weapons_data(self) -> list[dict]:
        return [read_json(self.__weapon_identifier_paths[identifier]) for identifier in self.__unique_weapons]

    def get_suits_data(self) -> list[dict]:
        return [read_json(self.__suit_identifier_paths[identifier]) for identifier in self.__unique_suits]

    def get_consumables_data(self) -> list[dict]:
        return [read_json(self.__consumable_identifier_paths[identifier]) for identifier in self.__unique_consumables]

    def get_augments_data(self) -> list[dict]:
        return [read_json(self.__augment_identifier_paths[identifier]) for identifier in self.__unique_augments]

    def get_synergies_data(self) -> list[dict]:
        return [read_json(self.__synergy_to_path[synergy]) for synergy in self.__unique_synergies]

    def get_encounter_mods_data(self) -> list[dict]:
        return [
            read_json(self.__encounter_mods_identifier_paths[encounter_mod])
            for encounter_mod in self.__unique_encounter_mods
        ]

    def get_hyper_data(self) -> dict:
        return read_json(self.hyper_data_dir / "HyperData.json")

    def get_hyper_config(self) -> dict:
        return read_json(self.hyper_data_dir / "HyperConfig.json")

    @staticmethod
    def __make_combat_unit_identifier(data: dict) -> str | None:
        tokens = []

        def add_value(key: str):
            if key in data:
                token = str(data[key])
                if len(token) > 0:
                    tokens.append(token)

        add_value("Path")
        add_value("Variation")
        add_value("Line")
        add_value("LineType")

        if data["UnitType"] != "Ranger":
            add_value("Stage")

        identifier = "_".join(tokens)
        assert len(identifier) > 0
        return identifier

    @staticmethod
    def __make_weapon_identifier(data: dict) -> str | None:
        name = data.get("Name", NONE_STR)
        if name == NONE_STR:
            return None
        stage = data["Stage"]
        combat_affinity = data["CombatAffinity"]
        variation = data.get("Variation", "")
        return f"{name}_stage{stage}_{combat_affinity}_{variation}".lower()

    @staticmethod
    def __make_suit_identifier(data: dict) -> str | None:
        name = data.get("Name", NONE_STR)
        if name == NONE_STR:
            return None
        stage = data["Stage"]
        variation = data.get("Variation", "")
        return f"{name}_stage{stage}_{variation}".lower()

    @staticmethod
    def __make_consumable_identifier(data: dict) -> str | None:
        name = data.get("Name", NONE_STR)
        if name == NONE_STR:
            return None
        stage = data["Stage"]
        return f"{name}_stage{stage}".lower()

    @staticmethod
    def __make_augment_identifier(data: dict) -> str | None:
        return SimDataCache.__make_suit_identifier(data)

    @staticmethod
    def __make_encounter_mod_identifier(data: dict) -> str | None:
        name = data.get("Name", NONE_STR)
        if name == NONE_STR:
            return None
        stage = data["Stage"]
        return f"{name}_stage{stage}".lower()

    @staticmethod
    def __register_by_identifier(data: dict, unique_ids: set[str], make_identifier) -> bool:
        identifier = make_identifier(data)
        if identifier is not None and identifier not in unique_ids:
            unique_ids.add(identifier)
            return True
        return False

    def __register_weapon(self, weapon: dict):
        type_id = weapon["TypeID"]
        # Weapons from game client battle files may be just empty entries
        if len(type_id["Name"]) != 0:
            if self.__register_by_identifier(type_id, self.__unique_weapons, self.__make_weapon_identifier):
                self.__register_synergy(type_id["CombatAffinity"])

    def __register_suit(self, suit: dict):
        type_id = suit["TypeID"]
        # Suits from game client battle files may be just empty entries
        if len(type_id["Name"]) != 0:
            self.__register_by_identifier(type_id, self.__unique_suits, self.__make_suit_identifier)

    def __register_consumable(self, consumable: dict):
        type_id = consumable["TypeID"]
        self.__register_by_identifier(type_id, self.__unique_consumables, self.__make_consumable_identifier)

    def __register_augment(self, augment: dict):
        type_id = augment["TypeID"]
        self.__register_by_identifier(type_id, self.__unique_augments, self.__make_augment_identifier)

    def register_encounter_mod(self, encounter_mod_instance: dict):
        type_id = encounter_mod_instance["TypeID"]
        self.__register_by_identifier(type_id, self.__unique_encounter_mods, self.__make_encounter_mod_identifier)

    def __register_synergy(self, synergy: str):
        if synergy != NONE_STR and synergy not in self.__unique_synergies:
            self.__unique_synergies.add(synergy)
            if synergy in self.__composite_synergies_components:
                for synergy_component in self.__composite_synergies_components[synergy]:
                    self.__unique_synergies.add(synergy_component)

    def __init_item_id_to_path_dict(self, subdir: str, dest_dict: dict[str, Path], identifier_maker):
        for file_path in (self.json_data_dir / subdir).rglob("*.json"):
            file_json = read_json(file_path)
            identifier = identifier_maker(file_json)
            dest_dict[identifier] = file_path

    def __init_combat_units_identifiers(self):
        combat_units_data_dir = self.json_data_dir / "CombatUnitData"
        print(f"{combat_units_data_dir.as_posix()}")
        for combat_unit_path in combat_units_data_dir.rglob("*.json"):
            identifier = SimDataCache.__make_combat_unit_identifier(read_json(combat_unit_path))
            self.__combat_unit_identifier_paths[identifier] = combat_unit_path

    def __register_combat_unit_data(self, identifier: str):
        if identifier in self.__combat_units_data:
            return

        cu_data = read_json(self.__combat_unit_identifier_paths[identifier])
        self.__combat_units_data[identifier] = cu_data
        is_ranger = cu_data["UnitType"] == "Ranger"
        if not is_ranger:
            self.__register_synergy(cu_data["CombatClass"])
            self.__register_synergy(cu_data["CombatAffinity"])

    @staticmethod
    def __determine_team_by_position(r: int) -> str:
        if r > 0:
            return "Blue"
        return "Red"


def main():
    """An entry point"""
    parser = argparse.ArgumentParser()
    parser.description = (
        "Converts simulation battle file from game client format to game server format. "
        "In order to do that it needs battle file itself and path to JSON data where it can get data "
        "to augment battle file"
    )

    parser.add_argument(
        "--json_data_dir",
        type=Path,
        required=True,
        help="Path to directory with json files.",
    )
    parser.add_argument(
        "--battle_file",
        type=Path,
        required=True,
        help="Path to battle file which must be converted.",
    )
    parser.add_argument(
        "--output_file",
        type=Path,
        required=True,
        help="Path to resulting file.",
    )
    cli_parameters = parser.parse_args()
    src_file_path: Path = cli_parameters.battle_file
    dst_file_path: Path = cli_parameters.output_file
    json_data_dir: Path = cli_parameters.json_data_dir

    battle_file_json = read_json(src_file_path)

    sim_data_cache = SimDataCache(json_data_dir)
    sim_data_cache.add_combat_units(battle_file_json["CombatUnits"])

    battle_config = {
        **battle_file_json.get("BattleConfig", {}),
    }

    for _, encounter_mod_instances in battle_config["EncounterMods"].items():
        for encounter_mod_instance in encounter_mod_instances["Instances"]:
            sim_data_cache.register_encounter_mod(encounter_mod_instance)

    data = {
        "Log": False,
        **battle_config,
        "TimeStepLimit": 2000,
        "DistanceScanFrequencyTimeSteps": 1,
        "BattleData": [
            {"Placements": sim_data_cache.get_placed_combat_units()},
        ],
        "GameData": {
            "Weapons": sim_data_cache.get_weapons_data(),
            "Suits": sim_data_cache.get_suits_data(),
            "Synergies": sim_data_cache.get_synergies_data(),
            "Augments": sim_data_cache.get_augments_data(),
            "Consumables": sim_data_cache.get_consumables_data(),
            "HyperData": sim_data_cache.get_hyper_data(),
            "HyperConfig": sim_data_cache.get_hyper_config(),
            "CombatUnits": sim_data_cache.get_combat_units_data(),
            "EncounterMods": sim_data_cache.get_encounter_mods_data(),
            "WorldEffectsConfig": read_json(json_data_dir / "WorldEffectsConfig" / "WorldEffectsConfig.json"),
        },
    }

    write_json(data, dst_file_path)
    print(f"Converting {src_file_path} -> to {dst_file_path}")


if __name__ == "__main__":
    main()
