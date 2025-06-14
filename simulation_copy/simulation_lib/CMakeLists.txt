cmake_minimum_required(VERSION 3.16)

# Set postfix for the platform/architecture
set(library_name_postfix "-Win64")
if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
    set(library_name_postfix "-Win32")
elseif (APPLE)
    set(library_name_postfix "-macOS")
elseif (UNIX)
    set(library_name_postfix "-Linux")
endif()

set(sim_lib_target libIlluviumSimulation${PROJECT_NAME_POSTFIX})

set(sim_lib_sources_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE sim_lib_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${sim_lib_sources_dir}/*")

# Include git version
set(sim_version_header_path "${CMAKE_CURRENT_BINARY_DIR}/utility/version_git_internal.h")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
configure_file("${sim_lib_sources_dir}/utility/version_git_internal.h.in" ${sim_version_header_path} @ONLY)
list(APPEND sim_lib_sources ${sim_version_header_path})

add_library(${sim_lib_target} STATIC ${sim_lib_sources})
add_library(simulation_lib ALIAS ${sim_lib_target})
target_include_directories(${sim_lib_target}
    PUBLIC "${sim_lib_sources_dir}"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/utility/"
)

# Include common settings
include(CommonSettings)
set_common_settings(${sim_lib_target})

# Apple specific settings
if(APPLE)
    # In unreal 5 this will give out an error
    # https://stackoverflow.com/questions/9894961/strange-warnings-from-the-linker-ld
    target_compile_options(${sim_lib_target} PUBLIC -fvisibility=hidden)
endif()

# Link targets https://docs.conan.io/en/latest/integrations/build_system/cmake/cmake_generator.html#targets-approach
target_link_libraries(
    ${sim_lib_target} PUBLIC
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
)

# Include static analyzers
include(StaticAnalyzers)

# Include sanitizers
# https://github.com/johnthagen/clang-blueprint
# https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
# https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
include(Sanitizers)
enable_sanitizers(${sim_lib_target})

# Include test coverage
include(TestCoverage)

if (ENABLE_PROFILING)
    target_link_libraries(${sim_lib_target} PUBLIC easy_profiler)
endif()

# also see: COMPILE_DEFINITIONS INCLUDE_DIRECTORIES
message("-- Target compiler flags are: ${MAIN_CFLAGS}")
add_custom_command(TARGET ${sim_lib_target} POST_BUILD COMMAND echo built with the flags: ${MAIN_CFLAGS})
