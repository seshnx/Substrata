#pragma once

#include <array>
#include <vector>
#include <string>
#include <cmath>

/**
 * Comprehensive scale database for microtonal harmonic generation.
 * Intervals are stored in cents (100 cents = 1 semitone, 1200 cents = 1 octave).
 * Supports Western modes, Middle Eastern maqamat, and exotic scales.
 */
class ScaleDatabase
{
public:
    // Scale categories for organization
    enum class Category
    {
        Western,
        MiddleEastern,
        Asian,
        Exotic
    };

    struct Scale
    {
        std::string name;
        Category category;
        std::vector<float> intervals;  // Intervals in cents from root
        std::string description;
    };

    // Key names
    static constexpr int NUM_KEYS = 12;
    static const char* const KEY_NAMES[NUM_KEYS];

    // Get all available scales
    static const std::vector<Scale>& getScales();
    static int getNumScales();
    static const Scale& getScale(int index);
    static const char* getScaleName(int index);

    // Get frequency for a scale degree
    // rootFreq: the root frequency in Hz
    // degreeIndex: which scale degree (0 = root, 1 = 2nd degree, etc.)
    // octaveOffset: which octave (-1 = below, 0 = same, 1 = above, etc.)
    static float getFrequencyForDegree(float rootFreq, const Scale& scale,
                                        int degreeIndex, int octaveOffset = 0);

    // Get the nearest scale degree for a given frequency
    // Returns the degree index and the exact frequency it should snap to
    static int getNearestDegree(float freq, float rootFreq, const Scale& scale);

    // Convert cents to frequency ratio
    static float centsToRatio(float cents);

    // Convert frequency ratio to cents
    static float ratioToCents(float ratio);

    // Get root frequency for a key (based on A4 = 440 Hz)
    static float getRootFrequency(int key, int octave = 4);

    // Find the nearest root frequency for a detected pitch given a key
    static float snapToNearestRoot(float detectedFreq, int key);

private:
    static std::vector<Scale> scales;
    static bool initialized;
    static void initializeScales();
};
