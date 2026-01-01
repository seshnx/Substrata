# SeshNx Substrata

**Harmonic Shifter Plugin**

A VST3/AU audio plugin built with JUCE that dynamically tracks the fundamental frequency (f0) and allows for microtonal shifting of generated overtones.

**Part of the SeshNx Plugin Suite by Amalia Media LLC**

## Features

- **Pitch Tracking**: Fast, reliable autocorrelation-based pitch detection (50 Hz - 2000 Hz range)
- **Harmonic Manipulation**: Generates and manipulates harmonics at 0.5f0 (subharmonic), 2f0, and 4f0
- **Microtonal Shifting**: Pitch-shift generated overtones by ±100 cents in 50-cent steps
- **Saturation Models**: Switchable harmonic character (Clean vs. Gritty)
- **Real-time Visualizer**: Frequency analyzer displaying f0 and generated overtones

## Controls

- **Brighten**: Boosts 2f0 and 4f0 harmonics (0.0 - 1.0)
- **Darken**: Boosts 0.5f0 subharmonic (0.0 - 1.0)
- **Character**: Harmonic saturation model (Clean/Gritty)
- **Mix**: Wet/Dry mix (0.0 - 1.0)
- **Overtone Shift**: Pitch shift for generated overtones (-100, -50, 0, +50, +100 cents)
- **Global Detune**: Global pitch detune for entire signal (±100 cents)

## Building

### Prerequisites

- JUCE framework (version 6.0 or later)
- CMake (version 3.22 or later)
- C++17 compatible compiler

### Setup

1. **Set up JUCE** (choose one method):
   - **Option A - Automatic (recommended)**: Run the setup script:
     - Windows: `.\setup_juce.ps1`
     - macOS/Linux: `git clone https://github.com/juce-framework/JUCE.git JUCE`
   - **Option B - Manual**: Clone or download JUCE into a `JUCE` subdirectory
   - **Option C**: Modify `CMakeLists.txt` to point to your JUCE installation

2. **Build the plugin**:
   - **Windows (PowerShell) - Recommended**: 
     ```powershell
     .\build.ps1                # Release build (default)
     .\build.ps1 -Config Debug  # Debug build
     .\build.ps1 -Clean         # Clean build
     .\build.ps1 -Configure     # Force reconfiguration
     ```
   - **Windows (Batch)**: Double-click `build.bat` or run `build.bat` from command prompt
   - **macOS/Linux**: Run `chmod +x build.sh && ./build.sh`

   Or manually:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

### Platform-Specific Notes

- **Windows**: Generates VST3 plugin in `build/Substrata_artefacts/Release/VST3/`
- **macOS**: Generates VST3 and AU plugins in `build/Substrata_artefacts/Release/`
- **Linux**: Generates VST3 plugin

## Architecture

### Core Components

- **PitchTracker**: Autocorrelation-based pitch detection algorithm
- **HarmonicGenerator**: Generates harmonics at 0.5f0, 2f0, and 4f0 with optional pitch shifting
- **PitchShifter**: High-quality granular pitch shifter for microtonal adjustments
- **PluginProcessor**: Main audio processing engine
- **PluginEditor**: UI with frequency visualizer and controls

### DSP Pipeline

1. Input signal → Pitch tracking (mono sum)
2. Detect f0 using autocorrelation
3. Generate harmonics at 0.5f0, 2f0, 4f0
4. Apply pitch shift to generated harmonics (if overtone shift is set)
5. Apply saturation (Clean/Gritty)
6. Mix with original signal
7. Apply global detune (if set)

## License

Copyright (c) 2024 Amalia Media LLC. All rights reserved.

Proprietary software - Distribution prohibited without explicit permission.

---

## Version

**v1.0.0**

---

## Support

For technical support, bug reports, or feature requests, please contact the development team through official SeshNx channels.

---

*Part of the [SeshNx Plugin Suite](https://seshnx.com) by Amalia Media LLC*

