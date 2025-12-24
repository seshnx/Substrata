#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <atomic>

/**
 * Fast pitch detection using autocorrelation-based approach
 * Robust across multiple octaves for bass, guitar, and vocals
 */
class PitchTracker
{
public:
    PitchTracker();
    ~PitchTracker() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    float processBlock(const float* input, int numSamples);
    
    float getCurrentF0() const { return currentF0.load(std::memory_order_relaxed); }
    bool isValidPitch() const { return isValid.load(std::memory_order_relaxed); }
    
    void setMinFrequency(float minFreq) { minFrequency = minFreq; }
    void setMaxFrequency(float maxFreq) { maxFrequency = maxFreq; }

private:
    float detectPitch(const float* input, int numSamples);
    float autocorrelation(const float* input, int length, int lag);
    
    double sampleRate = 44100.0;
    int blockSize = 512;

    std::atomic<float> currentF0 { 440.0f };
    std::atomic<bool> isValid { false };
    
    float minFrequency = 50.0f;   // ~B1
    float maxFrequency = 2000.0f; // ~B7
    
    std::vector<float> buffer;
    int bufferWritePos = 0;
    
    float smoothingFactor = 0.95f;
    float smoothedF0 = 440.0f;
};

