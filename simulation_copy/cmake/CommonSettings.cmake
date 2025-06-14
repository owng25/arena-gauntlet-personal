function(set_common_settings in_project_name)

    # Set C++ standard to be C++20
    target_compile_features(${in_project_name} PRIVATE cxx_std_20)

    # Enable warnings
    include(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CompilerWarnings.cmake)
    set_project_warnings(${in_project_name})

    # Enable this for faster build times
    option(ENABLE_UNITY_BUILD "Enable Unity builds of projects" OFF)
    if (ENABLE_UNITY_BUILD)
      set_target_properties(${in_project_name} PROPERTIES UNITY_BUILD ON)
    endif()

    # Disable exceptions
    if (MSVC)
        target_compile_options(${in_project_name} PRIVATE /EHsc)
    else()
        target_compile_options(${in_project_name} PRIVATE -fno-exceptions)
    endif()

    # Disable RTTI
    if(MSVC)
        target_compile_options(${in_project_name} PRIVATE /GR-)
    endif()

    # Enable big obj for MSVC
    if(MSVC)
        target_compile_options(${in_project_name} PRIVATE /bigobj)
    endif()

    # Enable position independant code
    if(NOT MSVC)
        target_compile_options(${in_project_name} PRIVATE -fPIC)
    endif()

    # Enable warnings
    if(MSVC)
        target_compile_options(${in_project_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${in_project_name} PRIVATE -Wall -Wextra -pedantic)
    endif()

    # Microsoft compiler specific options
    if(MSVC)
        # Enable multiprocessor builds
        target_compile_options(${in_project_name} PUBLIC "/MP")

        # Function level linking https://docs.microsoft.com/en-us/cpp/build/reference/gy-enable-function-level-linking?view=msvc-170
        # target_compile_options(${in_project_name} PUBLIC "/Gy")
    endif()

    # Link against pthread on Unix
    if (UNIX)
        target_link_libraries(${in_project_name} PRIVATE pthread)
    endif ()
endfunction()
