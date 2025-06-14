@echo off
call scripts/init.cmd

REM Create library directories and copy files
@REM conan package . --build-folder BuildWin32 --package-folder IlluviumSimulationEngine
conan package . --build-folder %BUILD_DIR_NAME% --package-folder %PACKAGE_DIR_NAME%

@pause
