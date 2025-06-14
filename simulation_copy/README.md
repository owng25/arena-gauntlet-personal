- [Simulation](#simulation)
- [Packaging for Unreal](#packaging-for-unreal)
  - [Windows](#windows)
  - [Mac](#mac)
    - [Prebuilt](#prebuilt)
    - [Build yourself](#build-yourself)
  - [Linux](#linux)
  - [Debugging in unreal](#debugging-in-unreal)
- [Building](#building)
  - [Dependencies](#dependencies)
  - [CMake flags](#cmake-flags)
  - [Windows](#windows-1)
    - [Scripts](#scripts)
    - [CLI](#cli)
  - [Mac](#mac-1)
    - [Dependencies](#dependencies-1)
    - [Scripts](#scripts-1)
    - [CLI - INTEL](#cli---intel)
    - [CLI - ARM](#cli---arm)
  - [Linux](#linux-1)
    - [Dependencies](#dependencies-2)
    - [Scripts](#scripts-2)
    - [CLI](#cli-1)
      - [Clang](#clang)
      - [GCC (NOT SUPPORTED)](#gcc-not-supported)
- [Docker](#docker)
- [Profiling](#profiling)
- [Testing](#testing)
  - [Run](#run)
    - [Running a subset of the tests](#running-a-subset-of-the-tests)
    - [Other Command line arguments](#other-command-line-arguments)
  - [Coverage](#coverage)
    - [Clang](#clang-1)
    - [GCC](#gcc)
    - [Kcov](#kcov)
    - [MSVC (Windows)](#msvc-windows)
      - [OpenCppCoverage](#opencppcoverage)
- [Tools](#tools)
  - [Simulation CLI](#simulation-cli)
      - [usage example](#usage-example)
  - [Clang-Tidy](#clang-tidy)
    - [Whole project](#whole-project)
  - [Clang Static Analyzer](#clang-static-analyzer)
- [Contributing](#contributing)
  - [EditorConfig](#editorconfig)
  - [Style](#style)
    - [Allman (BSD Style) Brackets](#allman-bsd-style-brackets)
    - [Spaces vs. Tabs  \& Line Length](#spaces-vs-tabs---line-length)
    - [Forward declarations](#forward-declarations)
    - [Align consecutive assignments where practical](#align-consecutive-assignments-where-practical)
  - [Ensuring determinism](#ensuring-determinism)

# Simulation

# Packaging for Unreal

## Windows
1. Execute `UnrealWindowsPackage.cmd` script
2. Copy the generated `IlluviumSimulationEngine/` directory to the unreal game client into `Source/`

## Mac

### Prebuilt
Go to the latest commit [action here](https://github.com/IlluviumGame/simulation/actions) and download the library for the mac version

### Build yourself
1. Execute the `UnrealMacPackage.sh` script
2. Copy the generated `IlluviumSimulationEngine/` directory to the unreal game client into `Source/`

## Linux
0. Make sure you have the dependencies installed as described [Linux Dependencies section](#dependencies-2)
1. Execute `UnrealLinuxPackage.sh` script
2. Copy the generated `IlluviumSimulationEngine/` directory to the unreal game client into `Source/`

## Debugging in unreal

Instructions only for windows but should be similar for mac:
1. Open [conan_profile_windows_unreal](./conan_profile_windows_unreal) and add:

Debug build needs to recompile unreal engine in a specific way:
```
#
[settings]
    build_type=Debug
    compiler.runtime_type=Debug
```

ReleaseWithDebugInformation, most useful, no need to recompile engine works with DebugGame target in unreal:
```
[conf]
    tools.build:cxxflags=["/Zi"]
    tools.build:sharedflags=["/DEBUG"]

```
2. Do the package [steps above](#windows) for the library

3. **(only if Debug Build NOT ReleaseWithDebugInformation).**

  In the unreal project, open [IlluviumSimulationEngine.Build.cs](https://github.com/IlluviumGame/gameclient-ue5/blob/main/Source/IlluviumSimulationEngine/IlluviumSimulationEngine.Build.cs) and make `bUseDebugLibrary` to be `bUseDebugLibrary = true;`

# Building

## Dependencies
- [Cmake >= 3.16](https://cmake.org/download/) - **Version 3.31.x**
- [Conan C++ package manager](https://conan.io/downloads.html) - **Version 1.66.0**

Please, install conan using this command: `pip install conan==1.66.0`. We currently do not support conan 2.0. If you have already installed conan 2.0, you can reintall it using:

```bash
pip uninstall conan
pip install conan==1.66.0

# First time you install it
conan profile new default --detect
```

## CMake flags

You can either pass these flags on the command line when running `cmake` or you can use [CMake GUI](https://cmake.org/runningcmake/) to configure them.
- `-DENABLE_TESTING=[On/Off]` - build the library with tests. (Default `On`)
- `-DENABLE_UNITY_BUILD=[On/Off]` - enable unity builds (bundles files together for faster compilation). (Default `Off` but for the scripts this is to `On`)
- `-DENABLE_UE=[On/Off]` - enable building against UE. Only really useful on Linux (Default `Off`)
    - `-DUE_PATH=<UE PATH>` - Unreal Engine Path. (Default `~/UnrealEngine/`)
- `-DENABLE_PROFILING=[On/Off]` - build the library with profiling. (Default `Off`)
- `-DENABLE_CLI=[On/Off]` - build the library with the cli tool. (Default `On`)
- `-DENABLE_VISUALIZATION=[On/Off]` - build the library with visualization support enabled. (Default `Off`)

## Windows

Must have these dependencies installed  [Visual Studio 2022](https://visualstudio.microsoft.com/vs/).

### Scripts
- [`WindowsBuild.cmd`](./WindowsBuild.cmd) - will generate the make files and build the project inside `BuildWind64/`.
- [`WindowsPackage.cmd`](./WindowsPackage.cmd)
  - Requires the Build script to be ran first.
  - Copies the files for packaging into `IlluviumSimulationEngine/`.
- [`WindowsClean.cmd`](./WindowsClean.cmd) - Removes the following directories: `BuildWind64/`, `IlluviumSimulationEngine/`.

### CLI
```bash
# Go into build directory
mkdir build && cd build

# Install dependencies
conan install .. --profile=../conan/conan_profile_windows --build outdated

# Generate VS project file by using conan
conan build .. --source-folder .. --build-folder .

# Build with cmake
cmake --build . --config Release

# Or build release with debug info
cmake --build . --config RelWithDebInfo
```

**Full debug build with tests:**
```bash
# Go into build directory
mkdir build-debug && cd build-debug

# Install dependencies
conan install .. --profile=../conan/conan_profile_windows_debug --build outdated

# Generate VS project file by using conan
conan build .. --source-folder .. --build-folder .

# You can now use the cmake build commands
cmake --build . --config Debug

# Build, run tests and redirect to the tests.log file
cmake --build . --config Debug; .\bin\libIlluviumSimulation-Win64-Tests.exe > tests.log
```

To clean:
<!-- https://stackoverflow.com/questions/9680420/looking-for-a-cmake-clean-command-to-clear-up-cmake-output -->
```bash
# Just Clean Release build
cmake --build . --target clean --config Release

# Clean and build Release afterwords
cmake --build . --clean-first --config Release
```

## Mac

### Dependencies

Install xcode command line tools.
```
xcode-select --install
```

Install the rest of the dependencies using [homebrew](https://brew.sh/).
```bash
brew install cmake conan clang-format mold
```

### Scripts
- [`MacBuild.sh`](./MacBuild.sh) - will generate the VS project files and build the project inside `BuildMac/`.
- [`MacPackage.sh`](./MacPackage.sh)
  - Requires the Build script to be ran first.
  - Copies the files for packaging into `IlluviumSimulationEngine/`.
- [`MacClean.sh`](./MacClean.sh) - Removes all the mac build directories.

### CLI - INTEL

```bash
# Go into build directory
mkdir build && cd build

# Install dependencies
conan install .. --profile=../conan/conan_profile_mac_intel --build outdated

# Generate project
cmake -S .. -B . -G Xcode -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Build
cmake --build . --config Release

# Or build release with debug info
cmake --build . --config RelWithDebInfo
```

**Full debug build with tests:**
```bash
# Go into build directory
mkdir build-debug && cd build-debug

conan install .. --profile=../conan/conan_profile_mac_intel  --build outdated -s build_type=Debug
cmake -S .. -B . -G Xcode -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
cmake --build . --config Debug

# Build, run tests and redirect to the tests.log file
cmake --build . --config Debug && ./bin/libIlluviumSimulation-macOS-Tests > tests.logs
```

**Using ninja:**
```bash
# Generate ninja release project
cmake -S .. -B . -G Ninja -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Generate ninja debug project
cmake -S .. -B . -G Ninja -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Build
ninja

# Build, run tests and redirect to the tests.log file
ninja && ./bin/libIlluviumSimulation-macOS-Tests > tests.logs
```

### CLI - ARM

```bash
# Go into build directory
mkdir build && cd build

# Install dependencies
conan install .. --profile=../conan/conan_profile_mac_arm --build outdated

# Generate project
cmake -S .. -B . -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Build
cmake --build . --config Release

# Or build release with debug info
cmake --build . --config RelWithDebInfo
```

**Full debug build with tests:**
```bash
# Go into build directory
mkdir build-debug && cd build-debug

conan install .. --profile=../conan/conan_profile_mac_arm --build outdated -s build_type=Debug
cmake -S .. -B . -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
cmake --build . --config Debug

# Build, run tests and redirect to the tests.log file
cmake --build . --config Debug && ./bin/libIlluviumSimulation-macOS-Tests > tests.logs
```

**Using ninja:**
```bash
# Generate ninja release project
cmake -S .. -B . -G Ninja -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Generate ninja debug project
cmake -S .. -B . -G Ninja -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Build
ninja

# Build, run tests and redirect to the tests.log file
ninja && ./bin/libIlluviumSimulation-macOS-Tests > tests.logs
```

## Linux

### Dependencies
Must also have these dependencies installed:
- [ninja](https://ninja-build.org/) - faster build system
- clang (or gcc) and make
- libpthread - for gtest
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) - useful to run the code style formatting automatically
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) - checks source files for certain problems
- [mold (OPTIONAL)](https://github.com/rui314/mold) - Modern fast Linker
  - using this linker instead of the default one can significantly speed the linking time.
  - once installed instead of running `ninja` to build just run `mold -run ninja`

**NOTE:** If you are building for Unreal Engine, you also need to have the Unreal Engine source code with dependencies installed.

Un ubuntu 24.04 LTS for example:
```bash
sudo apt install clang clang-format ninja-build cmake build-essential  \
                  libssl-dev libpthread-stubs0-dev clang-tidy clang-tools \
                  libc++-dev libc++abi-dev lld libc++-dev libc++abi-dev \
                  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu

# Conan
sudo apt install python3 python3-pip
pip3 install conan==1.65.0 --break-system-packages

# Init conan
conan profile new default --detect
conan profile update settings.compiler.libcxx=libstdc++11 default

# [Optional] Add the python scripts to your path inside ~/.bashrc
export PATH="$HOME/.local/bin:$PATH"

# [Optional] Edit the settings.yml to support clang 18
code ~/.conan/settings.yml
```

### Scripts
- [`LinuxBuild.sh`](./LinuxBuild.sh) - will generate the make file and build the project inside `BuildLinux/`. Use `--enable-ue` option when building for Unreal Engine.
- [`LinuxPackage.sh`](./LinuxPackage.sh)
  - Requires the Build script to be ran first.
  - Copies the files for packaging into `IlluviumSimulationEngine/`.
- [`LinuxClean.sh`](./LinuxClean.sh) - Removes the following directories: `BuildLinux/`, `IlluviumSimulationEngine/`.
- [`UnrealAndroidPackage.sh`](./UnrealAndroidPackage.sh)  - Packages for Android for arm.

When building for Linux, the script expects to find Unreal Engine in `~/UnrealEngine`. If you have it installed elsewhere, specify the path with the `--ue-path` option.


### CLI

#### Clang
clang is required when building for Unreal Engine, but recommended otherwise for consistency.

Install clang on ubuntu 24.04 LTS for example:
```bash
sudo apt install clang clang-tidy clang-tools ninja-build
```

**Building (assumes clang compiler):**
```bash
# In case default Profile is not created
conan profile new default --detect
conan profile update settings.compiler.libcxx=libstdc++11 default

# Go into build directory
mkdir build && cd build

# Install dependencies with clang
export CC=/usr/bin/clang-14
export CXX=/usr/bin/clang++-14
# Change the compiler version here with your version of clang
conan install .. -s compiler=clang -s compiler.version=14 --build outdated

# Generate Release Project
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Compile
ninja
```

Alternatively you can install dependencies using a custom clang profile, see [here](https://docs.conan.io/en/1.35/reference/profiles.html#build-profiles-and-host-profiles):
```bash
cd build
conan install .. --profile=clang --build outdated
```

**Full debug build with tests:**
```bash
# Go into build directory
mkdir build-debug && cd build-debug

export CC=/usr/bin/clang-14
export CXX=/usr/bin/clang++-14

conan install .. -s build_type=Debug -s compiler=clang -s compiler.version=14 --build outdated
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
ninja

# Build, run tests and redirect to the tests.log file
ninja && ./bin/libIlluviumSimulation-Linux-x64-Tests > tests.log
```

Clean:
```bash
ninja clean
```

#### GCC (NOT SUPPORTED)

If you want to use the GCC compiler instead.

**Build:**
```bash
# Set the correct ABI version
conan profile update settings.compiler.libcxx=libstdc++11 default

# Go into build directory
mkdir build && cd build

# Install dependencies
conan install .. -s build_type=Release -s compiler=gcc -s compiler.version=11 -s compiler.libcxx=libstdc++11 --build outdated

# Generate Release Project
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Release-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Compile
ninja
```

**Full debug build:**
```bash
conan install .. -s build_type=Debug -s compiler=gcc -s compiler.version=11 -s compiler.libcxx=libstdc++11 --build outdated

cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
ninja
```

# Docker

* Prerequisite:

1. Have docker installed
2. Load the Json Data Files
   * Copy the `data` folder in [json-data](https://github.com/IlluviumGame/json-data) to `<this_repo>/simulation-json-data`

```bash
# Copy the json data from github.com/IlluviumGame/json-data/ (or a subset if you want to test only some files)
cp -r ../json-data/data    simulation-json-data

# Build the image
docker build . -t simulation

# Run the image (Replace the BATTLE_JSON with the ESCAPED json string)
#  NOTE 1: MUST start & end with single quote, and all double quotes must be escaped
#  NOTE 2: on Windows there is a character limit in Command Prompt -> must use powershell if the json is too long
#  NOTE 3: if it says ABORTED, it usually means there is an issue with the JSON format -> check that the printed stoud is what you expected as the input JSON
docker run -e BATTLE_JSON='{\"Version\": 20, ...}' simulation
docker run -e BATTLE_JSON='{ \"Version\": 20, \"CombatUnits\": [ { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Dodo\", \"Stage\": 3, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"oDJPxuNwmx\", \"DominantCombatClass\": \"Empath\", \"DominantCombatAffinity\": \"Air\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"sGeNpRxylf\", \"TypeID\": { \"Name\": \"HyperEnduranceMatrix\", \"Stage\": 2, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 1 }, { \"ID\": \"FwNezXCvGb\", \"TypeID\": { \"Name\": \"SynapticBooster\", \"Stage\": 3, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 1 } ], \"EquippedConsumables\": [ { \"ID\": \"TcnBEufnoN\", \"TypeID\": { \"Name\": \"DragonEgg\", \"Stage\": 2 } } ] }, \"Position\": { \"Q\": -26, \"R\": 32 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Grokko\", \"Stage\": 1, \"Path\": \"Nature\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"ednBgSUNdS\", \"DominantCombatClass\": \"Bulwark\", \"DominantCombatAffinity\": \"Nature\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"gBkfXCMVox\", \"TypeID\": { \"Name\": \"PhotonRipper\", \"Stage\": 3, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 0 } ], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": -43, \"R\": 32 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Taipan\", \"Stage\": 3, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"tEhIsyvyKq\", \"DominantCombatClass\": \"Psion\", \"DominantCombatAffinity\": \"Earth\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"xhzUlsuZSY\", \"TypeID\": { \"Name\": \"HyperkineticStimulator\", \"Stage\": 2, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 0 }, { \"ID\": \"acGZTKLlNo\", \"TypeID\": { \"Name\": \"RedemptionCore\", \"Stage\": 1, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 0 } ], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": 17, \"R\": 15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Lynx\", \"Stage\": 3, \"Path\": \"RogueAir\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"xrivUwxKBg\", \"DominantCombatClass\": \"Rogue\", \"DominantCombatAffinity\": \"Air\", \"Level\": 1, \"EquippedAugments\": [], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": -34, \"R\": 15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Lynx\", \"Stage\": 3, \"Path\": \"BulwarkFire\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"sZwzLPwkKV\", \"DominantCombatClass\": \"Bulwark\", \"DominantCombatAffinity\": \"Fire\", \"Level\": 1, \"EquippedAugments\": [], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": 0, \"R\": 15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"MonkeyEmpath\", \"Stage\": 2, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"TsMJmpeTNO\", \"DominantCombatClass\": \"Empath\", \"DominantCombatAffinity\": \"Earth\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"xGAXXWWEzK\", \"TypeID\": { \"Name\": \"Vanguard\", \"Stage\": 0, \"Variation\": \"Original\" } } ], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": -17, \"R\": 15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"MonkeyFighter\", \"Stage\": 1, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"rccspadwlF\", \"DominantCombatClass\": \"Fighter\", \"DominantCombatAffinity\": \"Earth\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"xrfTPqxqbc\", \"TypeID\": { \"Name\": \"Toxic\", \"Stage\": 0, \"Variation\": \"Original\" } }, { \"ID\": \"eTrRnLpAuz\", \"TypeID\": { \"Name\": \"FinalAbounding\", \"Stage\": 0, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 0 } ], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": 34, \"R\": 15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Thylacine\", \"Stage\": 3, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"azwVvzwemQ\", \"DominantCombatClass\": \"Fighter\", \"DominantCombatAffinity\": \"Water\", \"Level\": 1, \"EquippedAugments\": [], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": -9, \"R\": 32 } }, { \"TypeID\": { \"UnitType\": \"Ranger\", \"LineType\": \"FemaleRanger\", \"Stage\": 0 }, \"Instance\": { \"ID\": \"vVwewNRcjz\", \"DominantCombatClass\": \"None\", \"DominantCombatAffinity\": \"None\", \"Level\": 1, \"EquippedWeapon\": { \"ID\": \"gTWqwCoCkp\", \"TypeID\": { \"Name\": \"Naboot\", \"Stage\": 2, \"Variation\": \"Original\", \"CombatAffinity\": \"Fire\" } }, \"EquippedSuit\": { \"ID\": \"AyGoaFLivl\", \"TypeID\": { \"Name\": \"ProtonCuirass\", \"Stage\": 1, \"Variation\": \"Original\" } } }, \"Position\": { \"Q\": 25, \"R\": 32 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"PoodleMoth\", \"Stage\": 3, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"AJmGoiQCrE\", \"DominantCombatClass\": \"Rogue\", \"DominantCombatAffinity\": \"Fire\", \"Level\": 1, \"EquippedAugments\": [], \"EquippedConsumables\": [ { \"ID\": \"QTijZhkVzN\", \"TypeID\": { \"Name\": \"Floraball\", \"Stage\": 0 } } ] }, \"Position\": { \"Q\": -2, \"R\": -15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Pterodactyl\", \"Stage\": 2, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"QNsEGUfRBs\", \"DominantCombatClass\": \"Rogue\", \"DominantCombatAffinity\": \"Fire\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"alHIzXzqSn\", \"TypeID\": { \"Name\": \"Harbinger\", \"Stage\": 0, \"Variation\": \"Original\" } } ], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": 23, \"R\": -32 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Grokko\", \"Stage\": 1, \"Path\": \"Air\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"ujqpXkqMmS\", \"DominantCombatClass\": \"Bulwark\", \"DominantCombatAffinity\": \"Air\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"nIpsCMhbGb\", \"TypeID\": { \"Name\": \"RazorAssimilator\", \"Stage\": 3, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 0 }, { \"ID\": \"lIddYvUlws\", \"TypeID\": { \"Name\": \"ChargeCapacitor\", \"Stage\": 3, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 0 } ], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": 6, \"R\": -32 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Volante\", \"Stage\": 2, \"Path\": \"Nature\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"rZMtvnBMJy\", \"DominantCombatClass\": \"Rogue\", \"DominantCombatAffinity\": \"Nature\", \"Level\": 1, \"EquippedAugments\": [], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": 15, \"R\": -15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Grokko\", \"Stage\": 3, \"Path\": \"Water\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"VvuahTAMhr\", \"DominantCombatClass\": \"Bulwark\", \"DominantCombatAffinity\": \"Water\", \"Level\": 1, \"EquippedAugments\": [], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": -11, \"R\": -32 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"MonkeyRogue\", \"Stage\": 2, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"XUMRDLwSfG\", \"DominantCombatClass\": \"Rogue\", \"DominantCombatAffinity\": \"Earth\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"FcrMtGUzsI\", \"TypeID\": { \"Name\": \"Shock\", \"Stage\": 0, \"Variation\": \"Original\" } }, { \"ID\": \"msMvozuqUC\", \"TypeID\": { \"Name\": \"Nature\", \"Stage\": 0, \"Variation\": \"Original\" } } ], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": -36, \"R\": -15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Thylacine\", \"Stage\": 1, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"ODnJxLuzbM\", \"DominantCombatClass\": \"Rogue\", \"DominantCombatAffinity\": \"Water\", \"Level\": 1, \"EquippedAugments\": [], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": 40, \"R\": -32 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Tenrec\", \"Stage\": 3, \"Path\": \"Default\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"NAuRngZPXm\", \"DominantCombatClass\": \"Fighter\", \"DominantCombatAffinity\": \"Earth\", \"Level\": 1, \"EquippedAugments\": [ { \"ID\": \"JsVUrMaQDX\", \"TypeID\": { \"Name\": \"VitalityVeil\", \"Stage\": 3, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 1 }, { \"ID\": \"GqOFZahcgW\", \"TypeID\": { \"Name\": \"TemporalSafeguard\", \"Stage\": 1, \"Variation\": \"Original\" }, \"SelectedAbilityIndex\": 1 } ], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": -19, \"R\": -15 } }, { \"TypeID\": { \"UnitType\": \"Illuvial\", \"LineType\": \"Fliish\", \"Stage\": 2, \"Path\": \"Earth\", \"Variation\": \"Original\" }, \"Instance\": { \"ID\": \"QOMjCwXjiI\", \"DominantCombatClass\": \"Psion\", \"DominantCombatAffinity\": \"Earth\", \"Level\": 1, \"EquippedAugments\": [], \"EquippedConsumables\": [] }, \"Position\": { \"Q\": 57, \"R\": -32 } } ], \"BattleConfig\": { \"EncounterMods\": { \"Red\": { \"Instances\": [] }, \"Blue\": { \"Instances\": [ { \"ID\": \"NnowZLXBCZ\", \"TypeID\": { \"Name\": \"EnergyResist\", \"Stage\": 3 } }, { \"ID\": \"KwhaAOKOty\", \"TypeID\": { \"Name\": \"AttackDamage\", \"Stage\": 3 } }, { \"ID\": \"DvFPlaBWJG\", \"TypeID\": { \"Name\": \"AttackSpeed\", \"Stage\": 2 } } ] } } } }' simulation

# (Optional) - attach into the terminal
docker run -it simulation bash
```

# [Profiling](./docs/profiling.md)

# Testing

We use the [Google Test Framework](https://google.github.io/googletest/).

## Run

Easiest way to run/debug tests if to have logging enabled and run the tests binary redirecting its output to a file (for example `tests.log`)

This way you can open the `tests.log` file and search for key terms, like `failure` to go directly to the test that failed:

Example
```bash
cd build

# Windows
cmake --build . --config Debug
.\bin\libIlluviumSimulation-Win64-Tests.exe > tests.log

# Linux
ninja
./bin/libIlluviumSimulation-Linux-x64-Tests > tests.log
```

Another option is to build and run the tests in the same command (saving time):
```bash
cd build

# Windows
cmake --build . --config Debug; .\bin\libIlluviumSimulation-Win64-Tests.exe > tests.log

# Linux
ninja && ./bin/libIlluviumSimulation-Linux-x64-Tests > tests.log
```

You can also run tests with the [Visual Studio test explorer](https://docs.microsoft.com/en-us/visualstudio/test/run-unit-tests-with-test-explorer?view=vs-2022) but this is also slow if you have logs enabled.

### Running a subset of the tests

See [documentation](https://github.com/google/googletest/blob/master/docs/advanced.md#running-test-programs-advanced-options)

Example on linux:
```bash
cd build

# List all the test
./bin/libIlluviumSimulation-Linux-x64-Tests --gtest_list_tests

# Run just all the math test suite
./bin/libIlluviumSimulation-Linux-x64-Tests --gtest_filter=Math.*
```

### Other Command line arguments

```
--logger-disable - run the tests with the logger disabled
```

## Coverage

Dependencies: `lcov`, `gcov` (for gcc), `gcovr` (for gcc), `llvm-cov` (for clang)

Install dependencies, on ubuntu 24.04 LTS for example:
```bash
# gcov is provided by the gcc package
# llvm-cov is proved by the
sudo apt install lcov gcc llvm
```
### [Clang](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html)

```bash
# Step 1: Compile with coverage enabled.
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=On -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
ninja

# Step 2: Run the tests.
LLVM_PROFILE_FILE="default.profraw" ./bin/libIlluviumSimulation-Linux-x64-Tests > tests.log

# Step 3: Index the raw profile.
llvm-profdata merge -sparse default.profraw -o default.profdata

# Summary:
llvm-cov report -instr-profile=default.profdata --ignore-filename-regex="tests\/.*" ./bin/libIlluviumSimulation-Linux-x64-Tests ../

# Terminal output:
llvm-cov show -instr-profile=default.profdata --ignore-filename-regex="tests\/.*" ./bin/libIlluviumSimulation-Linux-x64-Tests ../

# HTML output:
llvm-cov show --format html -output-dir="CODE_COVERAGE/" -show-instantiation-summary -instr-profile=default.profdata --ignore-filename-regex="tests\/.*" ./bin/libIlluviumSimulation-Linux-x64-Tests ../

# Open CODE_COVERAGE/index.html with a browser
```

### GCC
```bash
# Step 1: Compile with coverage enabled.
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=On -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
ninja

# Step 2: Run the tests
./bin/libIlluviumSimulation-Linux-x64-Tests > tests.logs

# Step 3. Generate
lcov --capture --exclude="/usr/include/*" --exclude="/*/.conan/*" --exclude="/*/tests/*" --directory . --output-file gtest_coverage.info

# Debug info
lcov --list gtest_coverage.info

# HTML output:
genhtml gtest_coverage.info --output-directory CODE_COVERAGE

# Open CODE_COVERAGE/index.html with a browser
```

### [Kcov](https://simonkagstrom.github.io/kcov/)

Collect coverage statistics with kcov, which doesn't require special compiler flags.
```bash
kcov --exclude-path="/usr/include,~/.conan/" --exclude-pattern="tests/" kcov-coverage/ ./bin/libIlluviumSimulation-Linux-x64-Tests > tests.logs

# Open kcov-coverage/index.html with a browser
```

### MSVC (Windows)

#### [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage)


Example run from the CLI:
```bash
# NOTE: Must be ran with absolute paths
OpenCppCoverage.exe --sources C:\dev\illuvium\simulation -- C:\dev\illuvium\simulation\build-windows\bin\libIlluviumSimulation-Win64-Tests.exe
```

Or install the [VS plugin](https://github.com/OpenCppCoverage/OpenCppCoveragePlugin) from [here](https://marketplace.visualstudio.com/items?itemName=OpenCppCoverage.OpenCppCoveragePlugin) and run it from `Tools -> Run OpenCppCoverage`



# Tools

## [Simulation CLI](https://github.com/IlluviumGame/simulation/tree/main/tools/cli)

Simulation cli is a tool that allows to load and run battle files that were saved by game client.

More details here - [Simulation CLI](https://github.com/IlluviumGame/simulation/tools/cli)

#### usage example

```bash
# Print help
./simulation-cli.exe --help

# Set location of JSON data files
./simulation-cli.exe set --json_data_path "E:\Workplace\IlluviumGame\Content\LocalTestData"

# Set location of battle files folder
./simulation-cli.exe set --battle_files_path "E:\Workplace\IlluviumGame\Saved\Simulation\Battles"

# If <battle_files_path> points to gameclient folder with battle files its enough to provide only name
./simulation-cli.exe run 2022.11.08-15.44.42
```

## [Clang-Tidy](https://clang.llvm.org/extra/clang-tidy/)

C++ "linter" tool.

This works with GCC/Clang.

To run clang-tidy on a file/directory:
```bash
./scripts/clang-tidy.sh Data/ability_data.h
./scripts/clang-tidy.sh Data
```

**NOTE:** Not really needed if you run `clang-format-all.sh` as that will fix all the style problems `clang-tidy` will report.

### Whole project

```bash
cd build/

# Conan install steps
<other steps>

# Generate project step
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Compile
ninja

# Run helper script that comes with clang-tidy installation
run-clang-tidy
```

## [Clang Static Analyzer](https://clang-analyzer.llvm.org/)

The Clang Static Analyzer finds bugs in C/C++ programs at compile time.

What this entails is that it intercepts the calls of the compiler.
Put `scan-build` in front of the build commands.

```bash
cd build/

# Conan install steps
<other steps>

# Generate project setup
scan-build cmake  -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Compile
scan-build ninja
```

# Contributing
## [EditorConfig](https://editorconfig.org/)

We have an [`.editorconfig`](./.editorconfig) file that sets the same coding style config for different editors.

Make sure your editor supports it or has a [Plugin Installed](https://editorconfig.org/#download).

## Style

We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) but with the following modifications listed bellow.

Run the following script to format the project automatically with our style:
```bash
./scripts/clang-format-all.sh
```

**NOTE:** That you should have clang-format **version 12** or higher, as there is a bug in previous versions that do not format lambda functions correctly.

### Allman (BSD Style) Brackets

Instead of:
```cpp
while (x == y) {
    something();
    somethingelse();
}
```

Write `{` on a new line:
```cpp
while (x == y)
{
    something();
    somethingelse();
}
```

### Spaces vs. Tabs  & Line Length

Use only spaces, and indent 4 spaces at a time.

120 is the maximum line length.

### Forward declarations

Prefer the usage of forward declarations.

Use forward declaration only in header files (`.h`) and only from files in the same project.

### Align consecutive assignments where practical

When doing multiple assignments, it is preferable to align the assignments operators. This practice improves code readability. There may be times when doing so is not practical and could be harm readability. If you do otherwise, make sure the deviation is justified and beneficial to readability.

## Ensuring determinism

1. Never use **floats** as they are not deterministic on every platform/compiler.
2. Never iterate over an [`std::unordered_set`](https://en.cppreference.com/w/cpp/container/unordered_set) or [`std::unordered_map`](https://en.cppreference.com/w/cpp/container/unordered_map) as the order is not guaranteed to be the same on all compilers/platforms.
3. Always check you are satisfying the **strict weak ordering** when using some standard library containers/function. See [`Compare`](https://en.cppreference.com/w/cpp/named_req/Compare) page for a list.
4. Never use the random number generator from the STL
