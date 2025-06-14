set(CLI_BINARY simulation-cli)

set(sim_cli_sources_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE sim_cli_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${sim_cli_sources_dir}/*")
add_executable(${CLI_BINARY} ${sim_cli_sources})
target_include_directories(${CLI_BINARY} PUBLIC ${sim_cli_sources_dir})

find_package(lyra CONFIG REQUIRED)

target_link_libraries(
    ${CLI_BINARY} PRIVATE

    # Link against our library
    simulation_lib

    # Link against lyra
    bfg::lyra
)

if (ENABLE_VISUALIZATION)
    target_link_libraries(
        ${CLI_BINARY} PRIVATE

        # Link against visualization library
        visualization-lib
    )
endif()

# Include common settings
set_common_settings(${CLI_BINARY})
enable_sanitizers(${CLI_BINARY})

get_target_property(MAIN_CFLAGS ${CLI_BINARY} COMPILE_OPTIONS)
# also see: COMPILE_DEFINITIONS INCLUDE_DIRECTORIES
message("-- ${CLI_BINARY} compiler flags are: ${MAIN_CFLAGS}")
