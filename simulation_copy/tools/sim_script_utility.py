"""Some code shared between scripts nearby"""
from pathlib import Path
import json
from typing import Iterable


def write_json(data: dict, path: Path):
    """Writes dictionary in JSON format"""
    with open(file=path, mode="w", encoding="utf-8") as file:
        json.dump(data, file, indent=4)


def read_json(path: Path) -> dict:
    """Reads file in JSON format and makes dictionary"""
    with open(file=path, mode="r", encoding="utf-8") as file:
        return json.load(file)


def get_jsons_in_directory(path: Path, pattern: str = "*.json", recursive: bool = True) -> Iterable[dict]:
    if recursive:
        for file_path in path.rglob(pattern):
            yield read_json(file_path)
    else:
        for file_path in path.glob(pattern):
            yield read_json(file_path)
