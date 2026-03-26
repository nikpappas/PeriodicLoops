@echo off
setlocal

set "BUILD_CONFIG=Debug"
set "SRC=build\src\PeriodicLoops_artefacts\%BUILD_CONFIG%\VST3\Periodic Loops.vst3"
set "DEST=%CommonProgramFiles%\VST3\Periodic Loops.vst3"

if not exist "%SRC%" (
    echo ERROR: VST3 not found at "%SRC%"
    echo Run: cmake --build build --config %BUILD_CONFIG%
    exit /b 1
)

echo Installing "%SRC%" to "%DEST%"
xcopy /E /I /Y "%SRC%" "%DEST%"

if %ERRORLEVEL% == 0 (
    echo Done.
) else (
    echo FAILED. Try running as Administrator.
)
