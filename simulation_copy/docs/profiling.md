# [easy_profiler](https://github.com/yse/easy_profiler)
We use [easy_profiler](https://github.com/yse/easy_profiler) for profiling. In order to user this you need two parts of this product - core and GUI.
- Core part will be installed with conan package manager with [conan_profile_windows_profiling](/conan/conan_profile_windows_profiling) profile.
- GUI part can be obtained on their [github page](https://github.com/yse/easy_profiler). You can either built it yourself or dowload pre built version at releases page (we currently use release `2.1.0`). Note that archives that marked as `core` will not contain GUI executable.

# Building with profiling enabled

To profile the code, build the library with `ENABLE_PROFILING=ON` and use easy_profiler to capture data. There are a few ways to to do that build depending how do you prefer to build the project.

## Build using CMake

These commands will build the project into `BuildWin64` directory.
```bash
mkdir ./BuildWin64
conan install . --profile=conan/conan_profile_windows_profiling --install-folder ./BuildWin64 --build outdated
cmake -G "Visual Studio 17 2022" -S . -B ./BuildWin64 -DENABLE_PROFILING=ON -DENABLE_TESTING=ON -DENABLE_CLI=ON
cmake --build ./BuildWin64 --config RelWithDebInfo
```
An important difference between this way and the next one is that we have to explicitly enable `ENABLE_PROFILING` option here.

## Build using `WindowsBuild.cmd`
```bash
./WindowsBuild.cmd --enable-profiling
```

## Build using `Conan`
This is the same what previous script does but it is shorter than the clean CMake version and is portable to other systems.

```
mkdir ./BuildWin64
conan install . --profile=conan/conan_profile_windows_profiling --install-folder ./BuildWin64 --build outdated
conan build . --source-folder . --build-folder ./BuildWin64
```

To profile the code, build the library with `ENABLE_PROFILING=ON` and use easy_profiler to capture data.

# Dumping `simulation-cli` profiling data

When you have built simulation command-line tool with profiling enable, you can run it as usual and it will create `simulation-cli.prof` file with near `simulation-cli` executable.

The script below is a minimal setup for profiling. It assumes that gameclient and simulation local repositories have the same parent directory and that gameclient contains `Saved/Simulation/Battles/big_battle.json` file.
```bash
./BuildWin64/bin/simulation-cli set --enable_debug_logs false
./BuildWin64/bin/simulation-cli set --json_data_path ../../../IlluviumGame/Content/LocalTestData
./BuildWin64/bin/simulation-cli set --battle_files_path ../../../IlluviumGame/Saved/Simulation/Battles
./BuildWin64/bin/simulation-cli run big_battle
```

Once you have `simulation-cli.prof` you can open it with `profiler_gui` to inspect it.

# Unit tests profiling
By default, unit tests will be profiled.

The unit tests will dump profiling data to the file `simulation.prof` after execution.

An important difference between unit tests and simulation-cli is that unit tests will save `.prof` file into current working directory, not in the directory of an executable.

# How to setup simulation profiling in a new program
To gather specific data, it would be necessary to consume the library within a program environment and use the `World::StartProfiling()` and `World::StopProfiling()` methods to start and stop profiling. You can specify a filename as the parameter to `World::StopProfiling()`. The output file can be opened in the easy_profiler GUI.

# Convert to JSON
Easy profiler also has `profiler_converter.exe` which converts profiling dumps to JSON format. Supposedly it will allow to view this file in other flamegraph reader (or at least write easy converter to common flamegraph format).
