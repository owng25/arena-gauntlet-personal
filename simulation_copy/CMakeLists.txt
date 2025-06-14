cmake_minimum_required(VERSION 3.16)

# Verbose display commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set postfix for the platform/arhitecture
# By default it is Win64
set(PROJECT_NAME_POSTFIX "-Win64")

# This populates toolchain variables so prefix can be determined before the project is declared the language
enable_language(CXX)

# NOTE: MSVC is not yet initiliazed here for some reason
if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
    set(PROJECT_NAME_POSTFIX "-Win32")
elseif(APPLE)
    set(PROJECT_NAME_POSTFIX "-macOS")
elseif(ANDROID_PLATFORM)
    set(PROJECT_NAME_POSTFIX "-Android")
elseif(UNIX)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "armv8")
        # Building for arm64
        message(STATUS "Building for arm64 architecture")
        set(PROJECT_NAME_POSTFIX "-Linux-arm64")
    else()
        # Building for x64
        message(STATUS "Building for x64 architecture")
        set(PROJECT_NAME_POSTFIX "-Linux-x64")
    endif ()
endif()


project(libIlluviumSimulation${PROJECT_NAME_POSTFIX} CXX)

# Don't use any prefix, just use the project name
# Otherwise this is inconsistent on different platforms
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Add -Debug at the end of debug libs
set(CMAKE_DEBUG_POSTFIX "-Debug")

# Include standard project stuff
include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)

# Use system includes (-isystem) for dependencies so that we don't get warnings from them
# - https://github.com/bincrafters/community/issues/1100
set(CONAN_SYSTEM_INCLUDES ON)

# Set C++ standard to be C++20 without extension (for example microsoft or GNU extensions)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Set output directories for archives
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

# Set output directories for libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

option(ENABLE_UE "Enable building against UE" OFF)
option(UE_PATH "UE Path" "~/UnrealEngine/")

# Linux-specific settings
if(UNIX AND NOT APPLE)
    if(ENABLE_UE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++ -I ${UE_PATH}/Engine/Source/ThirdParty/Linux/LibCxx/include/ -I ${UE_PATH}/Engine/Source/ThirdParty/Linux/LibCxx/include/c++/v1")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L ${UE_PATH}/Engine/Source/ThirdParty/Linux/LibCxx/lib/Linux/x86_64-unknown-linux-gnu/ -lc++ -lc++abi")
    endif()
endif()

# Include the Source code
include_directories(.)

# Easier modules lookup
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

option(ENABLE_PROFILING "Enable code profiling" OFF)
if (ENABLE_PROFILING)
    message(STATUS "Building With Profiler Enabled")
    add_definitions(-DENABLE_PROFILING=1 -DBUILD_WITH_EASY_PROFILER=1 -DEASY_OPTION_LOG=1)
    find_package(easy_profiler CONFIG REQUIRED)
endif()

add_subdirectory(simulation_lib)

# Add visualization
option(ENABLE_VISUALIZATION "Build with visualization enabled" OFF)
if (ENABLE_VISUALIZATION)
    message(STATUS "Building with Visualization Enabled")
    add_definitions(-DENABLE_VISUALIZATION=1)
    add_subdirectory(simulation_visualization)
endif()

# Add tests
option(ENABLE_TESTING "Build with the tests" ON)
if (ENABLE_TESTING)
    # We need this here otherwise ctest won't detect our tests
    enable_testing()
    add_subdirectory(simulation_tests)
endif()

# Add cli
option(ENABLE_CLI "Build with cli enabled" ON)
if (ENABLE_CLI)
    add_subdirectory(simulation_cli)
endif()

# Output the defaults coming from cmake
message(STATUS "CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CMAKE_C_FLAGS_RELWITHDEBINFO: ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
message(STATUS "CMAKE_C_FLAGS_MINSIZEREL: ${CMAKE_C_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CONAN_TARGET_ARCH : ${CONAN_TARGET_ARCH}")

# Output the compile options, used
get_target_property(MAIN_CFLAGS simulation_lib COMPILE_OPTIONS)
