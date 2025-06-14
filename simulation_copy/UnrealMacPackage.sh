#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

echo -e "\n==== Removing old directories ====\n"
MacCleanUnreal

if [ $BUILD_INTEL -eq 1 ]; then
    UnrealGenericPackage $BUILD_MAC_INTEL_UNREAL_DIR_NAME "conan_profile_mac_intel_unreal"
    MacCopyLibrary "$PACKAGE_DIR_NAME" "/tmp/$SIMULATION_MAC_INTEL_LIB_NAME"
fi

if [ $BUILD_ARM -eq 1 ]; then
    UnrealGenericPackage $BUILD_MAC_ARM_UNREAL_DIR_NAME "conan_profile_mac_arm_unreal"
    MacCopyLibrary "$PACKAGE_DIR_NAME" "/tmp/$SIMULATION_MAC_ARM_LIB_NAME"
fi

if [ $BUILD_PACKAGE_UNIVERSAL -eq 1 ]; then
    MacPackageCreateUniversalBuilds "$PACKAGE_DIR_NAME"
fi
