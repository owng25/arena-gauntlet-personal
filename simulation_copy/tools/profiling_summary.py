#!/usr/bin/env python3
"""Analyzes easy profiler files"""

from pathlib import Path
import argparse
import json
from dataclasses import dataclass, field
from typing import Callable, Iterable


@dataclass
class BlockDescriptor:
    """Information about one block"""

    block_id: int
    name: str
    src_file: str
    src_line: int

    @staticmethod
    def read_from_dict(data: dict) -> "BlockDescriptor":
        """Reads data block descriptor from dictionary"""
        block_id = data["id"]
        name = data["name"]
        src_file = data["sourceFile"]
        src_line = data["sourceLine"]
        return BlockDescriptor(block_id, name, src_file, src_line)


@dataclass
class MergedFrame:
    """Contains merged data from all frames with the same call stack"""

    block_id: int
    num_entries: int = 0
    total_duration: int = 0

    # Map from block id to merged frame for that block in this context
    children: dict[int, " MergedFrame"] = field(default_factory=dict)


@dataclass
class BlockEntry:
    """Profiling data for one entry"""

    name: str
    entry_id: int
    descriptor_id: int
    start_time: int
    stop_time: int
    children: list["BlockEntry"]

    @staticmethod
    def read_from_dict(data: dict) -> "BlockEntry":
        """Reads one profiling entry from dictionary"""
        name = data["name"]
        entry_id = data["id"]
        descriptor_id = data["descriptor"]
        start_time = data["start"]
        stop_time = data["stop"]
        children: list[BlockEntry] = list()
        for child_data in data.get("children", []):
            children.append(BlockEntry.read_from_dict(child_data))
        return BlockEntry(name, entry_id, descriptor_id, start_time, stop_time, children)

    @property
    def duration(self):
        """Total duration of this frame"""
        return self.stop_time - self.start_time

    @property
    def self_duration(self):
        """Duration - sum children duration"""
        return self.duration - sum(child.duration for child in self.children)


@dataclass
class ThreadData:
    """Profiling data for one thread"""

    thread_id: int
    name: str
    children: list[BlockEntry]

    @staticmethod
    def read_from_dict(data: dict) -> "ThreadData":
        """Reads thread information from dictionary"""
        thread_id = data["threadId"]
        name = data["threadName"]
        children: list[BlockEntry] = list()
        for child_data in data.get("children", []):
            children.append(BlockEntry.read_from_dict(child_data))
        return ThreadData(thread_id, name, children)


@dataclass
class ProfilingData:
    """Profiling data read from file"""

    block_descriptors: dict[int, BlockDescriptor]
    threads: list[ThreadData]

    def find_thread(self, name: str) -> ThreadData | None:
        """Finds thread by name"""
        for thread in self.threads:
            if thread.name == name:
                return thread
        return None

    @staticmethod
    def read_from_dict(data: dict) -> "ProfilingData":
        """Reads profiling data from dictinary"""
        block_descriptors = dict()
        for block_descriptor_json in data["blockDescriptors"]:
            block_descriptor = BlockDescriptor.read_from_dict(block_descriptor_json)
            block_descriptors[block_descriptor.block_id] = block_descriptor

        threads = []
        for thread_data in data.get("threads", []):
            threads.append(ThreadData.read_from_dict(thread_data))

        return ProfilingData(block_descriptors, threads)

    @staticmethod
    def read_from_file(path: Path) -> "ProfilingData":
        """Reads profiling data from file"""
        with open(file=path, mode="r", encoding="utf-8") as file:
            profiling_json: dict = json.load(file)
            return ProfilingData.read_from_dict(profiling_json)


@dataclass
class BlockAccStats:
    """Accumulated block stats"""

    descriptor: BlockDescriptor
    total_time: int = 0
    self_time: int = 0
    count: int = 0


def analyze(json_files: Iterable[Path]):
    """Analyzes the set of json files"""
    thread_total_duration = 0
    block_descriptors: dict[int, BlockDescriptor] = dict()
    next_block_desc_id = 0
    block_desc_guid_to_id: dict[str, int] = dict()
    blocks_acc_stats: dict[int, BlockAccStats] = dict()
    block_children: dict[int, set[int]] = dict()

    def make_block_desc_guid(block_desc: BlockDescriptor) -> str:
        return f"{block_desc.src_file}:{block_desc.src_line}->{block_desc.name}"

    def make_new_block_desc_id() -> int:
        nonlocal next_block_desc_id
        while next_block_desc_id in block_descriptors:
            next_block_desc_id += 1

        result = next_block_desc_id
        next_block_desc_id += 1
        return result

    def add_blocks(profiling_data: ProfilingData):
        block_desc_remap: dict[int, int] = dict()

        for new_block_id, new_bd in profiling_data.block_descriptors.items():
            bd_guid = make_block_desc_guid(new_bd)
            if bd_guid in block_desc_guid_to_id:
                block_desc_id = block_desc_guid_to_id[bd_guid]
                block_desc_remap[new_block_id] = block_desc_id
                new_bd.block_id = block_desc_id
            else:
                block_desc_id = make_new_block_desc_id()
                block_descriptors[block_desc_id] = new_bd
                bd_guid = make_block_desc_guid(new_bd)
                block_desc_guid_to_id[bd_guid] = block_desc_id
                block_desc_remap[new_block_id] = block_desc_id
                new_bd.block_id = block_desc_id

        return block_desc_remap

    def register_block_child(local_block_id: int, child_local_id: int):
        block_id = block_desc_remap[local_block_id]
        child_id = block_desc_remap[child_local_id]
        if block_id in block_children:
            block_children[block_id].add(child_id)
        else:
            block_children[block_id] = set([child_id])

    def get_block_acc_stats(local_block_id: int) -> BlockAccStats:
        block_id = block_desc_remap[local_block_id]
        if block_id in blocks_acc_stats:
            return blocks_acc_stats[block_id]

        block_stats = BlockAccStats(block_descriptors[block_id])
        blocks_acc_stats[block_id] = block_stats
        return block_stats

    for json_prof_path in json_files:
        print(f"{json_prof_path.stem} ", end="", flush=True)
        profiling_data = ProfilingData.read_from_file(json_prof_path)
        block_desc_remap = add_blocks(profiling_data)

        main_thread = profiling_data.find_thread("Main")
        if main_thread is None:
            raise RuntimeError(f"there is not main thread in {json_prof_path.stem}")
        walk_queue = list(main_thread.children)

        while len(walk_queue) > 0:
            entry = walk_queue.pop()
            block_stats: BlockAccStats = get_block_acc_stats(entry.descriptor_id)
            block_stats.count += 1
            block_stats.total_time += entry.duration
            block_stats.self_time += entry.self_duration

            for child in entry.children:
                register_block_child(
                    local_block_id=entry.descriptor_id,
                    child_local_id=child.descriptor_id,
                )
            walk_queue.extend(entry.children)

        for child in main_thread.children:
            thread_total_duration += child.duration

    def print_stats_by(
        getter: Callable[[BlockAccStats], int],
        percent_of_getter: Callable[[BlockAccStats], int] | None = None,
    ):
        for acc_stats in sorted(blocks_acc_stats.values(), key=getter, reverse=True):
            value = getter(acc_stats)
            percents_str = ""
            if not percent_of_getter is None:
                percent_of = percent_of_getter(acc_stats)
                percents = 0.0
                if percent_of > 0:
                    percents = 100 * (value / percent_of)
                percents_str = f" ({percents:.2f}%)"

            print(f"   {acc_stats.descriptor.name}: {value}{percents_str}")

    print("By call count:")
    print_stats_by(lambda x: x.count)

    print("By total time:")
    print_stats_by(lambda x: x.total_time, lambda _: thread_total_duration)

    print("By self time:")
    print_stats_by(lambda x: x.self_time, lambda x: x.total_time)

    print(f"Total thread time: {thread_total_duration}")


def cli_get_files_list() -> Iterable[Path]:
    """Parses command line arguments and returns files list"""
    parser = argparse.ArgumentParser()
    parser.description = "Prints profiling summary for one file or the set of files"

    command_sub_parser = parser.add_subparsers(dest="command", required=True)

    files_command_parser: argparse.ArgumentParser = command_sub_parser.add_parser(
        "files", help="Print summary for specified profiling trace file(s)."
    )
    files_command_parser.add_argument(
        "files",
        type=Path,
        nargs="+",
        help="List of paths to JSON files with profiling data.",
    )

    default_glob_expression = "*.prof.json"
    dirs_command_parser: argparse.ArgumentParser = command_sub_parser.add_parser(
        "dirs",
        help="Print summary for profiling trace file(s) found in specified directories"
        f"--glob can be used to specify globbing expression. Default glob expression is {default_glob_expression}",
    )
    dirs_arg_group = dirs_command_parser.add_argument_group()
    dirs_arg_group.add_argument("dirs", type=Path, nargs="+", help="List of paths to directories with traces.")
    dirs_arg_group.add_argument(
        "--glob",
        type=str,
        required=False,
        default=default_glob_expression,
        help="List of directories with traces.",
    )

    def walk_dirs(dirs: Iterable[Path], glob_expression: str) -> Iterable[Path]:
        for directory in dirs:
            yield from directory.rglob(glob_expression)

    cli_parameters = parser.parse_args()
    if cli_parameters.command == "files":
        files_list: Iterable[Path] = cli_parameters.files
        return files_list
    elif cli_parameters.command == "dirs":
        return walk_dirs(cli_parameters.dirs, cli_parameters.glob)
    else:
        raise Exception(f"Unknown command {cli_parameters.command}")


if __name__ == "__main__":
    analyze(cli_get_files_list())
