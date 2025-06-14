@echo off
call scripts/init.cmd

set CONAN_PROFILE=conan/conan_profile_windows_debug

IF "%~1"=="--enable-profiling" GOTO Profiling
GOTO Start

:Profiling
set CONAN_PROFILE=conan/conan_profile_windows_profiling

:Start
REM Downloading dependencies
conan install . --profile=%CONAN_PROFILE% --install-folder %BUILD_DIR_NAME% --build outdated

REM Create Project and build
conan build . --source-folder . --build-folder %BUILD_DIR_NAME%

@pause
