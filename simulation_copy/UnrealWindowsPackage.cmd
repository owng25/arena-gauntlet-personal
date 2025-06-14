@echo off
call scripts/init.cmd

REM Cleanup old directory
call scripts/init.cmd CleanUnrealAndPackageDirs

set CONAN_PROFILE=conan/conan_profile_windows_unreal

REM Install conan dependencies
conan install . --profile=%CONAN_PROFILE% --install-folder=%BUILD_UNREAL_DIR_NAME% --build outdated

REM Building
conan build . --source-folder . --build-folder %BUILD_UNREAL_DIR_NAME%

REM Create library directories and copy files
conan package . --build-folder %BUILD_UNREAL_DIR_NAME% --package-folder %PACKAGE_DIR_NAME%

@pause
