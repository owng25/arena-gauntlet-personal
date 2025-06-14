@echo on
call scripts/init.cmd

if exist BuildWin32 rmdir /s /q BuildWin32
if exist %BUILD_DIR_NAME% rmdir /s /q %BUILD_DIR_NAME%
call scripts/init.cmd CleanUnrealAndPackageDirs

@pause
