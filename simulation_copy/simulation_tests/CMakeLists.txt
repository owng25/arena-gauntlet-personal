# References:
# - https://raymii.org/s/tutorials/Cpp_project_setup_with_cmake_and_unit_tests.html
# - https://github.com/google/googletest/blob/master/googletest/README.md
# - https://google.github.io/googletest/quickstart-cmake.html

set(TEST_BINARY ${PROJECT_NAME}-Tests)

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

if (MSVC)
    if(MSVC_VERSION AND MSVC_VERSION GREATER_EQUAL 1910)
        add_definitions(-DGTEST_LANG_CXX11=1 -DGTEST_HAS_TR1_TUPLE=0)
    endif ()
endif ()

add_definitions(-DGOOGLE_UNIT_TEST)

enable_testing()
include(GoogleTest)

set(sim_tests_sources_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE sim_tests_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${sim_tests_sources_dir}/*")

# Define the tests
add_executable(${TEST_BINARY} ${sim_tests_sources})

target_include_directories(${TEST_BINARY} PUBLIC ${sim_tests_sources_dir})

find_package(GTest CONFIG REQUIRED)

# Link test binary
target_link_libraries(
    ${TEST_BINARY} PRIVATE

    # Link against our library
    simulation_lib

    # Link against google test
    GTest::gtest
)

if (ENABLE_VISUALIZATION)
    target_link_libraries(
        ${TEST_BINARY} PRIVATE

        # Link against visualization library
        visualization-lib
    )
endif()

# Include common settings
set_common_settings(${TEST_BINARY})

# Add tests by auto detecting them
gtest_discover_tests(${TEST_BINARY})
