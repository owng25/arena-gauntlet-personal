#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

echo -e "\n==== Removing old directories ====\n"
MacCleanServer

# Downloading dependencies
echo -e "\n==== Downloading dependencies ====\n"
conan install . --profile=conan/conan_profile_mac_server --install-folder "$BUILD_MAC_SERVER_DIR_NAME" --build outdated

# Build
GenericBuild "$BUILD_MAC_SERVER_DIR_NAME"

# Create library directories and copy files
GenericPackage "$BUILD_MAC_SERVER_DIR_NAME" "$PACKAGE_SERVER_DIR_NAME"
