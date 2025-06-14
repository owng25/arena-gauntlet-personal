#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

echo -e "\n==== Removing old directories ====\n"
LinuxCleanAll

# Find automatically the clang version
FindAndExportCorrectClangCompilerVersion

# Variables
ENABLE_PROFILING=False
ENABLE_TESTING=False
ENABLE_CLI=False
ENABLE_VISUALIZATION=False

for arg in "$@"
do
    case $arg in
        --enable-profiling)
        ENABLE_PROFILING=True
        shift
        ;;
        --enable-testing)
        ENABLE_TESTING=True
        shift
        ;;
        --enable-cli)
        ENABLE_CLI=True
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
conan install . \
    --profile=conan/conan_profile_linux_server \
    -s compiler.version=$CVER \
    -o enable_profiling=$ENABLE_PROFILING \
    -o enable_testing=$ENABLE_TESTING \
    -o enable_cli=$ENABLE_CLI \
    -o enable_visualization=$ENABLE_VISUALIZATION \
    --install-folder "$BUILD_LINUX_SERVER_DIR_NAME" --build outdated

# Building
GenericBuild "$BUILD_LINUX_SERVER_DIR_NAME"

# Create library directories and copy files
GenericPackage "$BUILD_LINUX_SERVER_DIR_NAME" "$PACKAGE_SERVER_DIR_NAME"
