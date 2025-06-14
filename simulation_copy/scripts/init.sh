#!/usr/bin/env bash
set -euf -o pipefail

export PACKAGE_DIR_NAME="IlluviumSimulationEngine"
export PACKAGE_SERVER_DIR_NAME="IlluviumServerSimulationEngine"

export BUILD_LINUX_DIR_NAME="BuildLinux"
export BUILD_LINUX_SERVER_DIR_NAME="BuildLinuxServer"
export BUILD_LINUX_ANDROID_UNREAL_DIR_NAME="BuildLinuxAndroidUnreal"
export BUILD_LINUX_INTEL_UNREAL_DIR_NAME="BuildLinuxIntelUnreal"
export BUILD_LINUX_ARM_UNREAL_DIR_NAME="BuildLinuxArmUnreal"

export BUILD_MAC_DIR_NAME="BuildMac"
export BUILD_MAC_SERVER_DIR_NAME="BuildMacServer"
export BUILD_MAC_UNREAL_DIR_NAME="BuildMacUnreal"
export BUILD_MAC_INTEL_DIR_NAME="BuildMacIntel"
export BUILD_MAC_INTEL_UNREAL_DIR_NAME="BuildMacIntelUnreal"
export BUILD_MAC_ARM_DIR_NAME="BuildMacArm"
export BUILD_MAC_ARM_UNREAL_DIR_NAME="BuildMacArmUnreal"

SIMULATION_BASE_NAME="libIlluviumSimulation"
SIMULATION_CLI_NAME="simulation-cli"
SIMULATION_MAC_LIB_BASE_NAME="$SIMULATION_BASE_NAME-macOS"
SIMULATION_MAC_LIB_NAME="$SIMULATION_MAC_LIB_BASE_NAME.a"
SIMULATION_MAC_INTEL_LIB_NAME="$SIMULATION_MAC_LIB_BASE_NAME-Intel.a"
SIMULATION_MAC_ARM_LIB_NAME="$SIMULATION_MAC_LIB_BASE_NAME-Arm.a"

SIMULATION_MAC_BIN_TESTS_NAME="$SIMULATION_BASE_NAME-macOS-Tests"

# Variables used by Linux/Mac building and packaging scripts
BUILD_PACKAGE_UNIVERSAL=1
BUILD_INTEL=1
BUILD_ARM=1

for arg in "$@"
do
    case $arg in
        --intel)
        BUILD_PACKAGE_UNIVERSAL=0
        BUILD_INTEL=1
        BUILD_ARM=0
        shift
        ;;
        --arm)
        BUILD_PACKAGE_UNIVERSAL=0
        BUILD_INTEL=0
        BUILD_ARM=1
        shift
        ;;
    esac
done

# Usage: MacCleanUnreal
function MacCleanUnreal()
{
    rm -Rfv "IlluviumThirdParty"
    rm -Rfv "$PACKAGE_DIR_NAME"
    rm -Rfv "$BUILD_MAC_UNREAL_DIR_NAME"
    rm -Rfv "$BUILD_MAC_INTEL_UNREAL_DIR_NAME"
    rm -Rfv "$BUILD_MAC_ARM_UNREAL_DIR_NAME"
}

# Usage: MacCleanServer
function MacCleanServer()
{
    rm -Rfv "IlluviumServerThirdParty"
    rm -Rfv "$PACKAGE_SERVER_DIR_NAME"
    rm -Rfv "$BUILD_MAC_SERVER_DIR_NAME"
}

# Usage: LinuxCleanAll
function LinuxCleanAll()
{
    rm -Rfv "IlluviumServerThirdParty"
    rm -Rfv "$PACKAGE_SERVER_DIR_NAME"
    rm -Rfv "$BUILD_LINUX_SERVER_DIR_NAME"
    rm -Rfv "$BUILD_LINUX_ANDROID_UNREAL_DIR_NAME"
    rm -Rfv "$BUILD_LINUX_INTEL_UNREAL_DIR_NAME"
    rm -Rfv "$BUILD_LINUX_ARM_UNREAL_DIR_NAME"
}

# Usage: GenericClean
function GenericClean()
{
    rm -Rfv "$BUILD_LINUX_DIR_NAME"
    rm -Rfv "$BUILD_MAC_DIR_NAME"

    rm -Rfv "$BUILD_MAC_INTEL_DIR_NAME"
    rm -Rfv "$BUILD_MAC_ARM_DIR_NAME"

    MacCleanUnreal
    MacCleanServer
    LinuxCleanAll
}

# Usage: GenericPackage BUILD_DIR PACKAGE_DIR
function GenericPackage()
{
    # Create library directories and copy files
    local build_dir="$1"
    local package_dir="$2"

    echo -e "\n==== Packaging ====\n"
    conan package . --build-folder "$build_dir" --package-folder "$package_dir"
}

# Usage: GenericPackage BUILD_DIR
function GenericBuild()
{
    local build_dir="$1"

    echo -e "\n==== Building ====\n"
    conan build . --source-folder . --build-folder "$build_dir"
}

# Usage: MacInstallAndBuild BUILD_DIRECTORY BUILD_PROFILE
function MacInstallAndBuild()
{
    local build_dir="$1"
    local conan_profile="conan/$2"

    # Downloading dependencies
    echo -e "\n==== Downloading dependencies ====\n"
    conan install . --profile=$conan_profile \
        --install-folder "$build_dir" --build outdated

    # Create Xcode Project
    # cmake -S . -G "Xcode" -B "$1" -DENABLE_UNITY_BUILD=ON -DENABLE_PROFILING=OFF

    # Build Xcode Project
    GenericBuild "$build_dir"
}

# Usage: MacPackageCreateUniversalBuilds PACKAGE_DIR
function MacPackageCreateUniversalBuilds()
{
    local package_dir="$1"
    local intel_lib_path="/tmp/$SIMULATION_MAC_INTEL_LIB_NAME"
    local arm_lib_path="/tmp/$SIMULATION_MAC_ARM_LIB_NAME"
    local dest="$package_dir/SimulationLibrary/Libraries/$SIMULATION_MAC_LIB_NAME"

    echo -e "\n==== Creating universal files ====\n"
    echo "Created: $dest"
    lipo -create "$intel_lib_path" "$arm_lib_path" -output "$dest"

    # Remove lib files
    rm -f "$intel_lib_path" "$arm_lib_path"
}

# Usage: MacCopyLibrary PACKAGE_DIR DEST_PATH
function MacCopyLibrary()
{
    local package_dir="$1"
    local dest_path="$2"

    cp -f -v "$package_dir/SimulationLibrary/Libraries/$SIMULATION_MAC_LIB_NAME" "$dest_path"
}

# Usage: UnrealGenericPackage BUILD_DIRECTORY BUILD_PROFILE
function UnrealGenericPackage()
{
    local BUILD_DIR="$1"
    local CONAN_PROFILE="conan/$2"

    # Cleaning old files
    echo -e "\n==== Cleaning old files ====\n"
    rm -Rf "$1"
    rm -Rf "$PACKAGE_DIR_NAME"

    # Downloading dependencies
    echo -e "\n==== Downloading dependencies ====\n"
    conan install . --profile:host=$CONAN_PROFILE --profile:build=default --install-folder "$BUILD_DIR" --build outdated

    GenericBuild "$BUILD_DIR"
    GenericPackage "$BUILD_DIR" "$PACKAGE_DIR_NAME"
}

function WaitForUserInput()
{
    read -s -n 1 -r -p "Press any key to continue . . . "
    echo ""
}

function FindAndExportCorrectClangCompilerVersion()
{
    # This is the maximum clang verison supported by conan
    local MIN_SUPPORTED_CLANG_VERSION=7
    local MAX_SUPPORTED_CLANG_VERSION=19

    get_current_clang_version()
    {
        if [[ $CC ]]; then
            echo $($CC --version | head -n 1 | sed -e 's/.* version \([0-9]*\).*/\1/')
        else
            echo 0
        fi
    }

    set +u
    if [[ ! $CC ]]; then
        echo -e "\n==== Detecting clang automatically ====\n"
        export CC="$(command -v clang)"
        export CXX="$(command -v clang++)"
    fi

    local detected_clang_version=$(get_current_clang_version)
    if (( $detected_clang_version < $MIN_SUPPORTED_CLANG_VERSION || $detected_clang_version > $MAX_SUPPORTED_CLANG_VERSION )); then
    # if true; then
        echo "Default clang version detected is not supported (detected=$detected_clang_version). Clang version must be in range [$MIN_SUPPORTED_CLANG_VERSION, $MAX_SUPPORTED_CLANG_VERSION]. Looking for something supported..."

        for ((i=$MIN_SUPPORTED_CLANG_VERSION; i <= $MAX_SUPPORTED_CLANG_VERSION; i++)); do
            local candidate_cc="$(command -v clang-$i)"
            local candidate_cxx="$(command -v clang++-$i)"

            echo "Looking for Clang version $i"
            if [ -x "$candidate_cc" ] && [ -x "$candidate_cxx" ]; then
                echo "Found clang: $candidate_cc and $candidate_cxx"
                export CC="$candidate_cc"
                export CXX="$candidate_cxx"
                break
            fi
        done
    fi

    if ! [ -x "$CC" ]; then
        echo "Error: Cannot find $CC" >&2
        exit 1
    fi

    if ! [ -x "$CXX" ]; then
        echo "Error: Cannot find $CXX" >&2
        exit 1
    fi

    if [[ -z "${CVER}" ]]; then
        echo -e "\n==== Detecting clang version ====\n"
        export CVER=$(get_current_clang_version)
    fi
    set -u

    echo "CC = $CC"
    echo "CXX = $CXX"
    echo "CVER = $CVER"

    conan profile update settings.compiler=clang default
    conan profile update settings.compiler.version=$CVER default
    conan profile update settings.compiler.libcxx=libstdc++11 default
}
