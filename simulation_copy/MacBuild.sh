#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

if [ $BUILD_INTEL -eq 1 ]; then
    MacInstallAndBuild $BUILD_MAC_INTEL_DIR_NAME "conan_profile_mac_intel"
fi

if [ $BUILD_ARM -eq 1 ]; then
    MacInstallAndBuild $BUILD_MAC_ARM_DIR_NAME "conan_profile_mac_arm"
fi

function CreateUniversalBuilds()
{
    local sufffix="$1"
    lipo -create "$BUILD_MAC_INTEL_DIR_NAME/$sufffix" "$BUILD_MAC_ARM_DIR_NAME/$sufffix" \
         -output "$BUILD_MAC_DIR_NAME/$sufffix"
}

if [ $BUILD_PACKAGE_UNIVERSAL -eq 1 ]; then
    echo -e "\n==== Creating universal files ====\n"

    mkdir -p "$BUILD_MAC_DIR_NAME/lib"
    mkdir -p "$BUILD_MAC_DIR_NAME/bin"

    lib_suffix="lib/$SIMULATION_MAC_LIB_NAME"
    bin_suffix="bin/$SIMULATION_MAC_BIN_TESTS_NAME"
    cli_suffix="bin/$SIMULATION_CLI_NAME"

    # Create universal builds for both the library and tests binary inside $BUILD_MAC_DIR_NAME
    CreateUniversalBuilds "$lib_suffix"
    CreateUniversalBuilds "$bin_suffix"
    CreateUniversalBuilds "$cli_suffix"
fi
