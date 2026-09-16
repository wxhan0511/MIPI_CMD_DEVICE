@echo off
setlocal EnableExtensions

pushd "%~dp0" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Cannot enter the project directory.
    pause
    exit /b 1
)

set "ROOT_DIR=%CD%"
set "APP_BUILD=%ROOT_DIR%\build"
set "BOOT_BUILD=%ROOT_DIR%\bootloader\build"

echo ============================================================
echo  MIPI CMD Device - Clean rebuild Bootloader and Application
echo ============================================================
echo.

where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake.exe was not found in PATH.
    goto :failed
)

where ninja.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] ninja.exe was not found in PATH.
    goto :failed
)

echo [1/8] Reading Application version from Bsp\bsp.c...
set "VERSION_VALUES="
set "FW_VERSION="
for /f "tokens=2 delims={" %%A in ('findstr.exe /l /c:"sw_version[4]" "%ROOT_DIR%\Bsp\bsp.c"') do set "VERSION_VALUES=%%A"
if not defined VERSION_VALUES (
    echo [ERROR] Cannot find sw_version[4] in Bsp\bsp.c.
    goto :failed
)
for /f "tokens=1-4 delims=,;} " %%A in ("%VERSION_VALUES%") do set "FW_VERSION=%%A.%%B.%%C.%%D"
if not defined FW_VERSION (
    echo [ERROR] Cannot parse the Application version.
    goto :failed
)
set "APP_BASENAME=GC5_MIPI_CMD_DEVICE_Firmware_V%FW_VERSION%"
set "APP_HEX=%APP_BUILD%\%APP_BASENAME%.hex"
set "APP_BIN=%APP_BUILD%\%APP_BASENAME%.bin"
echo Application version: %FW_VERSION%

echo [2/8] Deleting all old HEX and BIN files...
for %%P in (
    "%BOOT_BUILD%\*.hex"
    "%BOOT_BUILD%\*.bin"
    "%APP_BUILD%\*.hex"
    "%APP_BUILD%\*.bin"
) do (
    if exist "%%~P" del /f /q "%%~P" >nul 2>&1
)
for %%P in (
    "%BOOT_BUILD%\*.hex"
    "%BOOT_BUILD%\*.bin"
    "%APP_BUILD%\*.hex"
    "%APP_BUILD%\*.bin"
) do (
    if exist "%%~P" (
        echo [ERROR] Failed to delete old file: "%%~P"
        goto :failed
    )
)

echo [3/8] Configuring Bootloader...
cmake -S "%ROOT_DIR%\bootloader" -B "%BOOT_BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
if errorlevel 1 goto :failed

echo [4/8] Clean rebuilding Bootloader...
cmake --build "%BOOT_BUILD%" --clean-first --parallel
if errorlevel 1 goto :failed

echo [5/8] Configuring Application...
cmake -S "%ROOT_DIR%" -B "%APP_BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 goto :failed

echo [6/8] Clean rebuilding Application...
cmake --build "%APP_BUILD%" --clean-first --parallel
if errorlevel 1 goto :failed

echo [7/8] Adding the version number to Application filenames...
if not exist "%APP_BUILD%\build.hex" (
    echo [ERROR] Application build.hex was not generated.
    goto :failed
)
if not exist "%APP_BUILD%\build.bin" (
    echo [ERROR] Application build.bin was not generated.
    goto :failed
)
move /y "%APP_BUILD%\build.hex" "%APP_HEX%" >nul
if errorlevel 1 goto :failed
move /y "%APP_BUILD%\build.bin" "%APP_BIN%" >nul
if errorlevel 1 goto :failed

echo [8/8] Verifying generated files...
for %%F in (
    "%BOOT_BUILD%\boot.hex"
    "%BOOT_BUILD%\boot.bin"
    "%APP_HEX%"
    "%APP_BIN%"
) do (
    if not exist "%%~F" (
        echo [ERROR] Missing output file: "%%~F"
        goto :failed
    )
    if %%~zF LEQ 0 (
        echo [ERROR] Empty output file: "%%~F"
        goto :failed
    )
)

echo.
echo ============================================================
echo [SUCCESS] Bootloader and Application rebuilt successfully.
echo ============================================================
echo Bootloader HEX: "%BOOT_BUILD%\boot.hex"
echo Bootloader BIN: "%BOOT_BUILD%\boot.bin"
echo Application HEX: "%APP_HEX%"
echo Application BIN: "%APP_BIN%"
echo.
popd
pause
exit /b 0

:failed
set "FAIL_CODE=%ERRORLEVEL%"
if "%FAIL_CODE%"=="0" set "FAIL_CODE=1"
echo.
echo [FAILED] Build did not complete. Removing all HEX and BIN files...
for %%P in (
    "%BOOT_BUILD%\*.hex"
    "%BOOT_BUILD%\*.bin"
    "%APP_BUILD%\*.hex"
    "%APP_BUILD%\*.bin"
) do (
    if exist "%%~P" del /f /q "%%~P" >nul 2>&1
)
echo No HEX or BIN output has been kept.
echo.
popd
pause
exit /b %FAIL_CODE%
