#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

echo -e "\n==== Removing old directories ====\n"
LinuxCleanAll

# Required conan settings
FindAndExportCorrectClangCompilerVersion

# # Install dependencies
# echo -e "\n==== Downloading dependencies ====\n"
# conan install . \
#     --profile:b=default \
#     --profile:h=conan/conan_profile_android \
#     --install-folder "$BUILD_LINUX_ANDROID_UNREAL_DIR_NAME" --build missing

# # Build library
# GenericBuild "$BUILD_LINUX_ANDROID_UNREAL_DIR_NAME"

# # Package
# GenericPackage "$BUILD_LINUX_ANDROID_UNREAL_DIR_NAME" "$PACKAGE_DIR_NAME"

UnrealGenericPackage "$BUILD_LINUX_ANDROID_UNREAL_DIR_NAME" "conan_profile_android"
