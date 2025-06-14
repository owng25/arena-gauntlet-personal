#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

echo -e "\n==== Removing old directories ====\n"
LinuxCleanAll

# Required conan settings
FindAndExportCorrectClangCompilerVersion

if [ $BUILD_INTEL -eq 1 ]; then
    UnrealGenericPackage "$BUILD_LINUX_INTEL_UNREAL_DIR_NAME" "conan_profile_linux_intel_unreal"
fi

# if [ $BUILD_ARM -eq 1 ]; then
#     UnrealGenericPackage "$BUILD_LINUX_ARM_UNREAL_DIR_NAME" "conan_profile_linux_arm_unreal"
# fi
