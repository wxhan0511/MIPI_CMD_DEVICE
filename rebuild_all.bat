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
set "APP_FACTORY_HEX=%APP_BUILD%\factory.hex"
set "APP_FACTORY_BIN=%APP_BUILD%\factory.bin"

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

echo [2/8] Cleaning stale build outputs...
for %%P in (
    "%APP_BUILD%\*.hex"
    "%APP_BUILD%\*.bin"
) do (
    if exist "%%~P" del /f /q "%%~P" >nul 2>&1
)
echo [3/8] Configuring Bootloader...
cmake -S "%ROOT_DIR%\bootloader" -B "%BOOT_BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
if errorlevel 1 goto :failed

echo [4/8] Clean rebuilding Bootloader and regenerating boot.bin...
cmake --build "%BOOT_BUILD%" --target clean
if errorlevel 1 goto :failed
cmake --build "%BOOT_BUILD%" --target boot.elf --parallel
if errorlevel 1 goto :failed

echo [5/8] Configuring Application...
cmake -S "%ROOT_DIR%" -B "%APP_BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 goto :failed

echo [6/8] Clean rebuilding Application and finalizing CRC...
cmake --build "%APP_BUILD%" --clean-first --parallel
if errorlevel 1 goto :failed

echo [7/8] Verifying CRC-finalized application and factory images...
for %%F in (
    "%BOOT_BUILD%\boot.hex"
    "%BOOT_BUILD%\boot.bin"
    "%APP_BUILD%\build.hex"
    "%APP_BUILD%\build.bin"
    "%APP_FACTORY_HEX%"
    "%APP_FACTORY_BIN%"
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

echo [8/8] Checking generated image sizes...
for %%F in ("%APP_BUILD%\build.bin") do set "APP_BIN_SIZE=%%~zF"
if not "%APP_BIN_SIZE%"=="524288" (
    echo [ERROR] Invalid Application BIN size: %APP_BIN_SIZE% bytes, expected 524288.
    goto :failed
)

for %%F in ("%APP_FACTORY_BIN%") do set "FACTORY_BIN_SIZE=%%~zF"
if not "%FACTORY_BIN_SIZE%"=="589824" (
    echo [ERROR] Invalid Factory BIN size: %FACTORY_BIN_SIZE% bytes, expected 589824.
    goto :failed
)

if not exist "%APP_BUILD%\GC5_MIPI_CMD_DEVICE_Firmware_V%FW_VERSION%.bin" (
    echo [ERROR] Versioned App BIN was not generated.
    goto :failed
)
if not exist "%APP_BUILD%\GC5_MIPI_CMD_DEVICE_Firmware_V%FW_VERSION%.hex" (
    echo [ERROR] Versioned App HEX was not generated.
    goto :failed
)
if not exist "%APP_BUILD%\GC5_MIPI_CMD_DEVICE_Factory_V%FW_VERSION%.bin" (
    echo [ERROR] Versioned factory BIN was not generated.
    goto :failed
)
if not exist "%APP_BUILD%\GC5_MIPI_CMD_DEVICE_Factory_V%FW_VERSION%.hex" (
    echo [ERROR] Versioned factory HEX was not generated.
    goto :failed
)

echo.
echo ============================================================
echo [SUCCESS] Bootloader and Application rebuilt successfully.
echo ============================================================
echo Bootloader HEX: "%BOOT_BUILD%\boot.hex"
echo Bootloader BIN: "%BOOT_BUILD%\boot.bin"
echo Application HEX: "%APP_BUILD%\build.hex"
echo Application BIN: "%APP_BUILD%\build.bin"
echo Factory HEX: "%APP_FACTORY_HEX%"
echo Factory BIN: "%APP_FACTORY_BIN%"
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
