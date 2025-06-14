#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

if [ $BUILD_INTEL -eq 1 ]; then
    GenericPackage "$BUILD_MAC_INTEL_DIR_NAME" "$PACKAGE_DIR_NAME"
    MacCopyLibrary "$PACKAGE_DIR_NAME" "/tmp/$SIMULATION_MAC_INTEL_LIB_NAME"
fi

if [ $BUILD_ARM -eq 1 ]; then
    GenericPackage "$BUILD_MAC_ARM_DIR_NAME" "$PACKAGE_DIR_NAME"
    MacCopyLibrary "$PACKAGE_DIR_NAME" "/tmp/$SIMULATION_MAC_ARM_LIB_NAME"
fi

if [ $BUILD_PACKAGE_UNIVERSAL -eq 1 ]; then
    MacPackageCreateUniversalBuilds "$PACKAGE_DIR_NAME"
fi
