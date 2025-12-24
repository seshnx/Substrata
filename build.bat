@echo off
REM Build script for SeshNx Substrata Plugin (Windows Batch)
REM For more features, use build.ps1 instead

setlocal enabledelayedexpansion

set CONFIG=Release
set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build

echo ========================================
echo SeshNx Substrata - Build Script
echo ========================================
echo Project Directory: %PROJECT_DIR%
echo Build Directory: %BUILD_DIR%
echo Configuration: %CONFIG%
echo.

REM Check if JUCE directory exists
if not exist "%PROJECT_DIR%JUCE" (
    echo ERROR: JUCE directory not found!
    echo.
    echo Please either:
    echo   1. Run: setup_juce.ps1
    echo   2. Clone JUCE manually:
    echo      git clone https://github.com/juce-framework/JUCE.git JUCE
    echo   3. Or modify CMakeLists.txt to point to your JUCE installation
    echo.
    pause
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
    echo Build directory created.
    echo.
    set FORCE_CONFIGURE=1
)

cd /d "%BUILD_DIR%"

REM Configure with CMake if needed
if defined FORCE_CONFIGURE (
    echo Configuring CMake...
    cmake .. -DCMAKE_BUILD_TYPE=%CONFIG%
    if errorlevel 1 (
        echo.
        echo ERROR: CMake configuration failed!
        pause
        exit /b 1
    )
    echo CMake configuration successful.
    echo.
) else if not exist "CMakeCache.txt" (
    echo Configuring CMake...
    cmake .. -DCMAKE_BUILD_TYPE=%CONFIG%
    if errorlevel 1 (
        echo.
        echo ERROR: CMake configuration failed!
        pause
        exit /b 1
    )
    echo CMake configuration successful.
    echo.
) else (
    echo Using existing CMake configuration.
    echo.
)

REM Build the project
echo Building project...
echo.
cmake --build . --config %CONFIG%
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.

REM Show output locations
echo Output files:
if exist "Substrata_artefacts\%CONFIG%\VST3\Substrata.vst3" (
    echo   VST3: Substrata_artefacts\%CONFIG%\VST3\Substrata.vst3
)
if exist "Substrata_artefacts\%CONFIG%\AU\Substrata.component" (
    echo   AU: Substrata_artefacts\%CONFIG%\AU\Substrata.component
)
if exist "Substrata_artefacts\%CONFIG%\Standalone\Substrata.exe" (
    echo   Standalone: Substrata_artefacts\%CONFIG%\Standalone\Substrata.exe
)
echo.

pause

