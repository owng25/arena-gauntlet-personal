#!/usr/bin/env python3
"""Generates random combat class JSON"""

from pathlib import Path
import random
import argparse
import string
import re as regular_expressions
from dataclasses import dataclass, field
from sim_script_utility import *
from sim_grid_config import *
import itertools


@dataclass
class Synergy:
    name: str
    components: dict[str, int] = field(default_factory=dict)
    thresholds: list[int] = field(default_factory=list)


@dataclass
class SynergiesInfo:
    """Reads synergies JSON files and stores required information"""

    all_combat_classes: list[Synergy] = field(default_factory=list)
    all_combat_affinities: list[Synergy] = field(default_factory=list)

    all_synergies: dict[str, Synergy] = field(default_factory=dict)
    all_synergies_names: list[str] = field(default_factory=list)

    @classmethod
    def read_from(cls, synergy_data_dir: Path) -> "SynergiesInfo":
        if not (synergy_data_dir.exists() and synergy_data_dir.is_dir()):
            return SynergiesInfo()

        all_classes: list[Synergy] = list()
        all_affinities: list[Synergy] = list()
        all_synergies: dict[str, Synergy] = dict()
        for synergy_json in get_jsons_in_directory(synergy_data_dir):
            components_key = None
            synergy = None
            if "CombatClass" in synergy_json:
                name = synergy_json["CombatClass"]
                synergy = Synergy(name)
                components_key = "CombatClassComponents"
                all_classes.append(synergy)
            else:
                name = synergy_json["CombatAffinity"]
                synergy = Synergy(name)
                components_key = "CombatAffinityComponents"
                all_affinities.append(synergy)

            all_synergies[synergy.name] = synergy

            # read synergy thresholds
            synergy.thresholds = [int(threshold) for threshold in synergy_json["SynergyThresholds"]]
            assert len(synergy.thresholds) >= 3

            # read components of composite synergies
            for component in synergy_json[components_key]:
                if component in synergy.components:
                    synergy.components[component] += 1
                else:
                    synergy.components[component] = 1

        return SynergiesInfo(
            all_classes,
            all_affinities,
            all_synergies,
            list(all_synergies.keys()),
        )

    def get_random_synergy(self, rnd: random.Random, different_from: str | None = None) -> str | None:
        choose_from = self.all_synergies_names
        if different_from is not None:
            choose_from.remove(different_from)
        index = rnd.randint(0, len(choose_from))
        if index < len(self.all_synergies_names):
            return self.all_synergies_names[index]
        return None

    def find_synergy(self, name: str) -> Synergy | None:
        return self.all_synergies.get(name, None)

    def all_involved_synergies(self, synergies: Iterable[str]) -> set[str]:
        result = set()
        for name in synergies:
            if name.lower() != "none":
                result.add(name)
                result.update(*self.find_synergy(name).components)
        return result


@dataclass
class CombatUnitTemplate:
    unit_type: str
    line_type: str
    stage: str
    combat_affinity: str
    combat_class: str
    fusion_path: str
    variation: str
    size: int


@dataclass
class CombatUnitsInfo:
    all_combat_units: list[CombatUnitTemplate]
    synergy_to_units: dict[str, list[CombatUnitTemplate]]

    @staticmethod
    def load(synergies: SynergiesInfo, path: Path, combat_unit_line_pattern: str | None) -> "CombatUnitsInfo":
        """Reads all combat unit templates in specified directory"""
        all_combat_units: list[CombatUnitTemplate] = list()
        synergy_to_units: dict[str, list[CombatUnitTemplate]] = dict()

        def register_combat_unit_synergy(synergy_name: str, template: CombatUnitTemplate):
            if synergy_name in synergy_to_units:
                synergy_to_units[synergy_name].append(template)
            else:
                synergy_to_units[synergy_name] = [template]

        line_pattern = None
        if not combat_unit_line_pattern is None:
            line_pattern = regular_expressions.compile(combat_unit_line_pattern)

        for combat_unit_path in path.glob("*.json"):
            combat_unit_json: dict = read_json(combat_unit_path)

            if combat_unit_json["UnitType"] != "Illuvial":
                continue

            combat_unit_line_type = combat_unit_json["Line"]
            if (not line_pattern is None) and (not line_pattern.fullmatch(combat_unit_line_type)):
                print(f"{combat_unit_line_type} filtered out")
                continue

            combat_unit_template = CombatUnitTemplate(
                unit_type="Illuvial",
                line_type=combat_unit_line_type,
                stage=combat_unit_json["Stage"],
                combat_affinity=combat_unit_json["CombatAffinity"],
                combat_class=combat_unit_json["CombatClass"],
                fusion_path=combat_unit_json["Path"],
                variation=combat_unit_json["Variation"],
                size=combat_unit_json["SizeUnits"],
            )

            all_combat_units.append(combat_unit_template)

            register_combat_unit_synergy(combat_unit_template.combat_affinity, combat_unit_template)
            register_combat_unit_synergy(combat_unit_template.combat_class, combat_unit_template)

        for synergy_name, units_list in synergy_to_units.items():
            if synergy_name == "None":
                continue
            synergy = synergies.find_synergy(synergy_name)
            assert not synergy is None
            if len(synergy.components) == 0:
                continue
            for base_synergy_name, count in synergy.components.items():
                if count == 0:
                    continue
                if base_synergy_name in synergy_to_units:
                    synergy_to_units[base_synergy_name].extend(units_list)
                else:
                    synergy_to_units[base_synergy_name] = list(units_list)

        return CombatUnitsInfo(all_combat_units, synergy_to_units)


def filter_dict(data_source: dict, allowed_keys: Iterable) -> dict:
    return {key: value for key, value in data_source.items() if key in allowed_keys}


def load_all_weapons_type_ids(path: Path) -> list[dict]:
    weapons: list[dict] = list()
    keys = set(["Name", "Stage", "CombatAffinity", "Variation"])
    for weapon in get_jsons_in_directory(path):
        weapons.append(filter_dict(weapon, keys))
    return weapons


def load_all_suits_type_ids(path: Path) -> list[dict]:
    suits: list[dict] = list()
    keys = set(["Name", "Stage", "Variation"])
    for suit in get_jsons_in_directory(path):
        suits.append(filter_dict(suit, keys))
    return suits


def load_all_consumables_type_ids(path: Path) -> list[dict]:
    consumables: list[dict] = list()
    keys = set(["Name", "Stage"])
    for consumable in get_jsons_in_directory(path):
        consumables.append(filter_dict(consumable, keys))
    return consumables


generated_unique_ids: set[str] = set()


def make_unique_instance_id(rnd: random.Random) -> str:
    while True:
        attempt = "".join(rnd.choices(string.ascii_letters, k=10))
        if generated_unique_ids in generated_unique_ids:
            continue
        generated_unique_ids.add(attempt)
        return attempt


@dataclass
class AugmentInfo:
    type_id: dict
    num_abilities: int
    combat_classes: list[str]
    combat_affinities: list[str]

    def make_random_instance(self, rnd: random.Random) -> dict:
        result = {"ID": make_unique_instance_id(rnd), "TypeID": self.type_id}
        if self.num_abilities > 0:
            result["SelectedAbilityIndex"] = rnd.randrange(0, self.num_abilities)
        return result


def load_augments_info(path) -> list[AugmentInfo]:
    augments: list[AugmentInfo] = list()
    keys = set(["Name", "Stage", "Variation"])
    for augment in get_jsons_in_directory(path):
        type_id = filter_dict(augment, keys)
        num_abilities = len(augment.get("Abilities", []))
        augments.append(
            AugmentInfo(
                type_id=type_id,
                num_abilities=num_abilities,
                combat_classes=augment.get("CombatClasses", []),
                combat_affinities=augment.get("CombatAffinities", []),
            )
        )
    return augments


def load_all_encounter_mods_type_ids(path) -> list[dict]:
    encounter_mods: list[dict] = list()
    keys = set(["Name", "Stage"])
    for encounter_mod in get_jsons_in_directory(path):
        encounter_mods.append(filter_dict(encounter_mod, keys))
    return encounter_mods


def make_random_augments(
    unit_template: CombatUnitTemplate,
    dominant_class: str,
    dominant_affinity: str,
    synergies: SynergiesInfo,
    rnd: random.Random,
    augments: list[AugmentInfo],
) -> list[AugmentInfo]:
    indices = set()
    count = rnd.randint(0, 2)
    while len(indices) < count:
        index = rnd.randrange(0, len(augments))

        if index not in indices:
            augment = augments[index]
            augment_synergies = synergies.all_involved_synergies(
                itertools.chain(augment.combat_classes, augment.combat_affinities)
            )
            unit_synergies = synergies.all_involved_synergies(
                (unit_template.combat_class, unit_template.combat_affinity, dominant_class, dominant_affinity)
            )

            if not augment_synergies.intersection(unit_synergies):
                indices.add(index)

    return [augments[index].make_random_instance(rnd) for index in indices]


def make_random_weapon(rnd: random.Random, weapons: list[dict]) -> dict:
    return {
        "ID": make_unique_instance_id(rnd),
        "TypeID": rnd.choice(weapons),
    }


def make_random_suit(rnd: random.Random, suits: list[dict]) -> dict:
    return {
        "ID": make_unique_instance_id(rnd),
        "TypeID": rnd.choice(suits),
    }


def make_random_encounter_mod_instance(rnd: random.Random, encounter_mod_types: list[dict]) -> dict:
    return {
        "ID": make_unique_instance_id(rnd),
        "TypeID": rnd.choice(encounter_mod_types),
    }


def make_random_encounter_mod_instances(rnd: random.Random, encounter_mod_types: list[dict]) -> list[dict]:
    count = rnd.randint(0, min(len(encounter_mod_types), 3))
    return [make_random_encounter_mod_instance(rnd, encounter_mod_types) for _ in range(count)]


def qr_to_dict(qr: tuple[int, int]) -> dict[str, int]:
    q, r = qr
    return {"Q": q, "R": r}


def make_random_team_combat_units(
    synergies: SynergiesInfo,
    combat_units_info: CombatUnitsInfo,
    weapons: list[dict],
    suits: list[dict],
    consumables: list[dict],
    augments: list[AugmentInfo],
    rnd: random.Random,
    team_main_synergy: str | None,
    unique_only: bool,
    is_blue_team: bool,
    max_team_size,
) -> list:
    """Generates an array of n random combat units"""

    # Team size in range [1; predefined positions count]
    team_size = rnd.randint(1, max_team_size)

    synergy_min_threshold = 0

    # List of combat units that will add to selected synergy
    synergy_units_pool = list()

    all_units_pool = combat_units_info.all_combat_units.copy()

    # Get_random_synergy may return None and it is intentional -
    # sometimes we want to have teams without focus on synergy
    if not team_main_synergy is None:
        synergy_units_pool = list(combat_units_info.synergy_to_units[team_main_synergy])
        # Avoid making teams less than minimal threshould for selected synergy
        synergy_min_threshold = min(synergies.all_synergies[team_main_synergy].thresholds)
        if synergy_min_threshold > team_size and synergy_min_threshold <= max_team_size:
            team_size = synergy_min_threshold

    # How many combat units were picked from synergy pool
    num_picked_from_synergy_pool = 0

    def get_random_template() -> CombatUnitTemplate:
        nonlocal synergy_units_pool, num_picked_from_synergy_pool
        selected = None
        if len(synergy_units_pool) == 0:
            selected = rnd.choice(all_units_pool)
        else:
            # Pick combat unit from the pool
            index = rnd.randrange(start=0, stop=len(synergy_units_pool))
            selected = synergy_units_pool[index]
            num_picked_from_synergy_pool += 1

            if num_picked_from_synergy_pool >= synergy_min_threshold:
                # No need to pick from synergy pool anymore since we have reached synergy threshold.
                # An intention here is to have more diverse teams
                synergy_units_pool = list()
            else:
                # Remove units of same line type as duplicate will not add to synergy counter)
                synergy_units_pool = [x for x in synergy_units_pool if x.line_type != selected.line_type]

        if unique_only:
            all_units_pool.remove(selected)

        return selected

    def make_dominant_synergy(owner_synergy: str) -> str | None:
        if owner_synergy == "None":
            return owner_synergy

        synergy = synergies.find_synergy(owner_synergy)
        assert not synergy is None
        if len(synergy.components) == 0:
            return owner_synergy

        return rnd.choice(list(synergy.components.keys()))

    # to select only one consumable per team
    consumable_used = False

    def make_consumables():
        nonlocal consumable_used
        if consumable_used:
            return []
        consumable_used = True
        return [
            {
                "ID": make_unique_instance_id(rnd),
                "TypeID": rnd.choice(consumables),
            }
        ]

    def make_one() -> tuple[CombatUnitTemplate, dict]:
        """Generates rnadom combat unit JSON"""

        combat_unit_template = get_random_template()
        dominant_combat_class = make_dominant_synergy(combat_unit_template.combat_class)
        dominant_combat_affinity = make_dominant_synergy(combat_unit_template.combat_affinity)

        return (
            combat_unit_template,
            {
                "TypeID": {
                    "UnitType": "Illuvial",
                    "LineType": combat_unit_template.line_type,
                    "Stage": combat_unit_template.stage,
                    "Path": combat_unit_template.fusion_path,
                    "Variation": combat_unit_template.variation,
                },
                "Instance": {
                    "ID": make_unique_instance_id(rnd),
                    "DominantCombatClass": dominant_combat_class,
                    "DominantCombatAffinity": dominant_combat_affinity,
                    "EvolutionPrimaryCombatClass": "None",
                    "EvolutionPrimaryCombatAffinity": "None",
                    "Level": 1,
                    "EquippedAugments": make_random_augments(
                        combat_unit_template, dominant_combat_class, dominant_combat_affinity, synergies, rnd, augments
                    ),
                    "EquippedConsumables": make_consumables(),
                },
            },
        )

    num_illuvials: int = team_size
    with_ranger: bool = rnd.randint(0, 1) == 1
    if with_ranger:
        num_illuvials -= 1

    team_units = []
    max_unit_size = 3  # For rangers
    for _ in range(num_illuvials):
        unit_template, unit_instance = make_one()
        max_unit_size = max(max_unit_size, unit_template.size)
        team_units.append(unit_instance)

    if with_ranger:
        ranger_unit = {
            "TypeID": {
                "UnitType": "Ranger",
                "LineType": "FemaleRanger",
                "Stage": 0,
            },
            "Instance": {
                "ID": make_unique_instance_id(rnd),
                "DominantCombatClass": "None",
                "DominantCombatAffinity": "None",
                "EvolutionPrimaryCombatClass": "None",
                "EvolutionPrimaryCombatAffinity": "None",
                "Level": 1,
                "EquippedWeapon": make_random_weapon(rnd, weapons),
                "EquippedSuit": make_random_suit(rnd, suits),
            },
        }

        # Bonding is not used atm so disable for now
        # use_bonding = rnd.randint(0, 1) == 1
        # if use_bonding:
        #     select_list = [unit for unit in team_units if unit["Instance"]["DominantCombatAffinity"] != "None"]
        #     if len(select_list) != 0:
        #         ranger_unit["Instance"]["BondedID"] = rnd.choice(select_list)["Instance"]["ID"]

        team_units.append(ranger_unit)

    # Shuffle positions and iterate over them.
    # This approach avoids picking the same position twice.
    shuffled_positions = list(get_predefined_team_positions(is_blue_team, max_unit_size))
    rnd.shuffle(shuffled_positions)

    # Assign units positions
    for index, unit in enumerate(team_units):
        unit["Position"] = qr_to_dict(shuffled_positions[index])

    return team_units


GRID_CONFIG = GridConfig(DEFAULT_GRID_WIDTH, DEFAULT_GRID_HEIGHT, DEFAULT_MIDDLE_LINE_WIDTH)

red_team_positions_cache: dict[int, list] = dict()
blue_team_positions_cache: dict[int, list] = dict()


def get_predefined_team_positions(blue_team: bool, unit_size: int) -> list:
    cache = blue_team_positions_cache if blue_team else red_team_positions_cache
    if not unit_size in cache:
        getter = GRID_CONFIG.fill_blue_side if blue_team else GRID_CONFIG.fill_red_side
        cache[unit_size] = list(getter(unit_size))

    return cache[unit_size]


def generate_random_battles(
    synergies: SynergiesInfo,
    combat_units_templates: CombatUnitsInfo,
    weapons: list[dict],
    suits: list[dict],
    consumables: list[dict],
    augments: list[AugmentInfo],
    encounter_mods: list[dict],
    count: int,
    output_dir: Path,
    rnd: random.Random,
):
    """Makes 'count' random battle files"""
    output_dir.mkdir(parents=True, exist_ok=True)
    unique_team_units_only = True
    max_team_size = 10

    def make_team(team_main_synergy, is_blue_team):
        return make_random_team_combat_units(
            synergies,
            combat_units_templates,
            weapons,
            suits,
            consumables,
            augments,
            rnd,
            team_main_synergy,
            unique_team_units_only,
            is_blue_team,
            max_team_size,
        )

    duplicates_in_battles: list[int] = list()
    for idx in range(count):
        combat_units = []

        team_a_main_synergy = synergies.get_random_synergy(rnd)
        combat_units.extend(make_team(team_a_main_synergy, is_blue_team=True))

        # Prefer main synergy for the second team different from the first team
        team_b_main_synergy = synergies.get_random_synergy(rnd, different_from=team_a_main_synergy)
        combat_units.extend(make_team(team_b_main_synergy, is_blue_team=False))

        unique_templates: set[str] = set(str(x["TypeID"]) for x in combat_units)
        if len(unique_templates) != len(combat_units):
            duplicates_in_battles.append(idx)

        result_json = {
            "Version": 20,
            "CombatUnits": combat_units,
            "BattleConfig": {
                "EncounterMods": {
                    "Red": {"Instances": make_random_encounter_mod_instances(rnd, encounter_mods)},
                    "Blue": {"Instances": make_random_encounter_mod_instances(rnd, encounter_mods)},
                },
            },
        }
        write_json(result_json, output_dir / f"{idx}.json")

    # print("Duplicates in battles: [{}]".format(", ".join(str(x) for x in duplicates_in_battles)))


def main():
    """An entry point"""
    parser = argparse.ArgumentParser()
    parser.description = (
        "Generates --count random combat files using combat units from "
        "--json_data_dir. --seed option controls random number generation "
        "(i.e. results will be the same when json files and --seed the same. "
        "Results will be stored in the path specified by --output_dir in form "
        "x.json, where x is an index of generated combat."
    )

    parser.add_argument("--count", type=int, required=True, help="Count of battle files to generate")
    parser.add_argument(
        "--json_data_dir",
        type=Path,
        required=True,
        help="Path to directory with json files.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        required=False,
        default=0,
        help="Seed for random generator. Zero by default",
    )
    parser.add_argument(
        "--output_dir",
        type=Path,
        required=True,
        help="Path to a directory where to store results",
    )
    parser.add_argument(
        "--combat_unit_line_filter",
        type=str,
        required=False,
        default=None,
        help="Regular expression to select allowed combat units by line type. By default selects them all.",
    )
    cli_parameters = parser.parse_args()
    data_dir = cli_parameters.json_data_dir
    synergies = SynergiesInfo.read_from(data_dir / "SynergyData")
    combat_units_templates = CombatUnitsInfo.load(
        synergies, data_dir / "CombatUnitData", cli_parameters.combat_unit_line_filter
    )

    rnd = random.Random(x=cli_parameters.seed)

    weapons = load_all_weapons_type_ids(data_dir / "WeaponData")
    rnd.shuffle(weapons)

    suits = load_all_suits_type_ids(data_dir / "SuitData")
    rnd.shuffle(suits)

    consumables = load_all_consumables_type_ids(data_dir / "ConsumableData")
    rnd.shuffle(consumables)

    augments = load_augments_info(data_dir / "AugmentData")
    rnd.shuffle(augments)

    encounter_mods = load_all_encounter_mods_type_ids(data_dir / "EncounterModData")
    rnd.shuffle(encounter_mods)

    generate_random_battles(
        synergies=synergies,
        combat_units_templates=combat_units_templates,
        weapons=weapons,
        suits=suits,
        consumables=consumables,
        augments=augments,
        encounter_mods=encounter_mods,
        count=cli_parameters.count,
        output_dir=cli_parameters.output_dir,
        rnd=rnd,
    )


if __name__ == "__main__":
    main()
