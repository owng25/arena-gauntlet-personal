set(VISUALIZATION_LIBRARY visualization-lib)

find_package(raylib CONFIG REQUIRED)

set(sim_vis_sources_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE sim_vis_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${sim_vis_sources_dir}/*")
add_library(${VISUALIZATION_LIBRARY} ${sim_vis_sources})
target_include_directories(${VISUALIZATION_LIBRARY} PUBLIC ${sim_vis_sources_dir})

target_link_libraries(
    ${VISUALIZATION_LIBRARY}

    # Link against our library
    PRIVATE simulation_lib

    # Link against raylib
    raylib
)

# Include common settings
set_common_settings(${VISUALIZATION_LIBRARY})
