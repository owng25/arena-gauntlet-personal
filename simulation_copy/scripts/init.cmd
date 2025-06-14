@REM TODO(vampy): Use generic? https://stackoverflow.com/questions/2763875/batch-file-include-external-file-for-variables

set OLD_PACKAGE_DIR_NAME=IlluviumThirdParty
set PACKAGE_DIR_NAME=IlluviumSimulationEngine
set BUILD_DIR_NAME=BuildWin64
set BUILD_UNREAL_DIR_NAME=BuildWin64Unreal


:CleanUnrealAndPackageDirs
    if exist %BUILD_UNREAL_DIR_NAME% rmdir /s /q %BUILD_UNREAL_DIR_NAME%
    if exist %PACKAGE_DIR_NAME% rmdir /s /q %PACKAGE_DIR_NAME%
    if exist %OLD_PACKAGE_DIR_NAME% rmdir /s /q %OLD_PACKAGE_DIR_NAME%
    exit /B 0
