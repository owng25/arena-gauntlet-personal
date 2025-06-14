#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

# Required conan settings
FindAndExportCorrectClangCompilerVersion

# Variables
ENABLE_UE=False
UE_PATH=~/UnrealEngine
ENABLE_TESTING=True
ENABLE_PROFILING=False
ENABLE_CLI=True
ENABLE_VISUALIZATION=False

CONAN_PROFILE=conan_profile_linux

for arg in "$@"
do
    case $arg in
        --enable-ue)
        ENABLE_UE=True
        ENABLE_TESTING=False
        CONAN_PROFILE=conan_profile_linux_intel_unreal
        shift
        ;;
        --ue-path)
        UE_PATH="${arg#*=}"
        shift
        ;;
        --disable-cli)
        ENABLE_CLI=False
        shift
        ;;
        --disable-testing)
        ENABLE_TESTING=False
        shift
        ;;
        --enable-profiling)
        ENABLE_PROFILING=True
        shift
        ;;
        --enable-visualization)
        ENABLE_VISUALIZATION=True
        shift
        ;;
    esac
done

# Downloading dependencies
echo -e "\n==== Downloading dependencies ====\n"
conan install . --profile=conan/$CONAN_PROFILE \
    -s compiler.version=$CVER \
    -s build_type=Release \
    -o enable_profiling=$ENABLE_PROFILING \
    -o enable_testing=$ENABLE_TESTING \
    -o enable_visualization=$ENABLE_VISUALIZATION \
    -o enable_ue=$ENABLE_UE \
    -o enable_cli=$ENABLE_CLI \
    -o ue_path="$UE_PATH" \
    --install-folder "$BUILD_LINUX_DIR_NAME" --build outdated

# Build
GenericBuild "$BUILD_LINUX_DIR_NAME"

# Build Makefiles
# cmake -S . -G Ninja -B "$BUILD_LINUX_DIR_NAME" -DENABLE_UNITY_BUILD=ON -DENABLE_UE=$ENABLE_UE -DUE_PATH=$UE_PATH -DENABLE_TESTING=$ENABLE_TESTING -DENABLE_PROFILING=$ENABLE_PROFILING
# cmake --build "$BUILD_LINUX_DIR_NAME" --config Release
