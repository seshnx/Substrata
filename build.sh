#!/bin/bash
# Build script for SeshNx Substrata Plugin (macOS/Linux)

set -e  # Exit on error

echo "========================================"
echo "SeshNx Substrata - Build Script"
echo "========================================"
echo ""

# Check if JUCE directory exists
if [ ! -d "JUCE" ]; then
    echo "ERROR: JUCE directory not found!"
    echo ""
    echo "Please either:"
    echo "  1. Clone JUCE into a 'JUCE' subdirectory:"
    echo "     git clone https://github.com/juce-framework/JUCE.git JUCE"
    echo "  2. Or modify CMakeLists.txt to point to your JUCE installation"
    echo ""
    exit 1
fi

# Detect OS
OS="$(uname -s)"
case "${OS}" in
    Linux*)     BUILD_TYPE="Release";;
    Darwin*)    BUILD_TYPE="Release";;
    *)          BUILD_TYPE="Release";;
esac

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure with CMake
echo ""
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

# Build the project
echo ""
echo "Building plugin..."
cmake --build . --config ${BUILD_TYPE}

echo ""
echo "========================================"
echo "Build completed successfully!"
echo "========================================"
echo ""

echo "Plugin location:"
if [ -f "Substrata_artefacts/Release/VST3/Substrata.vst3" ] || [ -f "Substrata_artefacts/${BUILD_TYPE}/VST3/Substrata.vst3" ]; then
    echo "  VST3: Substrata_artefacts/${BUILD_TYPE}/VST3/Substrata.vst3"
fi
if [ -d "Substrata_artefacts/Release/AU/Substrata.component" ] || [ -d "Substrata_artefacts/${BUILD_TYPE}/AU/Substrata.component" ]; then
    echo "  AU: Substrata_artefacts/${BUILD_TYPE}/AU/Substrata.component"
fi
if [ -f "Substrata_artefacts/Release/Standalone/Substrata" ] || [ -f "Substrata_artefacts/${BUILD_TYPE}/Standalone/Substrata" ]; then
    echo "  Standalone: Substrata_artefacts/${BUILD_TYPE}/Standalone/Substrata"
fi
echo ""

