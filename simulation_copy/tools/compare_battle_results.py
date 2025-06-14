#!/usr/bin/env python3
"""Tool to compare to directories with combat results
generated with run_battle_files.py"""

from pathlib import Path
import argparse
import json


def compare_battle_results(root_a: Path, root_b: Path):
    """
    Compares two directories with battle results
    generated with run_battle_files.py
    to ensure that combat results are the same
    """

    def read_file(path: Path) -> bytes:
        with open(file=path, mode="rb") as file_handle:
            return file_handle.read()

    def read_duration(path: Path) -> float:
        with open(file=path, mode="r", encoding="utf-8") as file_handle:
            json_data = json.load(file_handle)
            return json_data["duration"]

    all_same = True
    files_count = 0
    duration_a = 0.0
    duration_b = 0.0

    different_results: list[str] = list()

    for file_a in root_a.rglob("*stdout.txt"):
        rel_path = file_a.relative_to(root_a)

        content_a = read_file(file_a)
        content_b = read_file(root_b / rel_path)

        duration_a += read_duration(root_a / rel_path.parent / "duration.json")
        duration_b += read_duration(root_b / rel_path.parent / "duration.json")

        if content_a != content_b:
            different_results.append(rel_path.parent.stem)

        files_count += 1

    all_same = len(different_results) == 0
    print(f"Files count: {files_count}")
    print(f"All same: {all_same}")
    if not all_same:
        different_results.sort()
        print(f"{len(different_results)} different results: [{', '.join(different_results)}]")
    print(f"Duration for {root_a.as_posix()}: {duration_a:.2f}")
    print(f"Duration for {root_b.as_posix()}: {duration_b:.2f}")


def main():
    """Entry point"""
    parser = argparse.ArgumentParser()
    parser.description = (
        "Compares two directories with battle results generated with run_battle_files.py"
        " to ensure that combat results are the same"
    )
    parser.add_argument("--targets", nargs=2, required=True, help="Paths to directories to compare")
    cli_parameters = parser.parse_args()
    compare_battle_results(
        root_a=Path(cli_parameters.targets[0]),
        root_b=Path(cli_parameters.targets[1]),
    )


if __name__ == "__main__":
    main()
