#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <cmath>

/**
 * High-quality pitch shifter using overlap-add granular synthesis
 * Two overlapping grains with Hann windowing for click-free processing
 */
class PitchShifter
{
public:
    PitchShifter();
    ~PitchShifter() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, float pitchShiftCents);
    void reset();

private:
    static constexpr int NUM_GRAINS = 2;
    static constexpr int GRAIN_SIZE = 2048;  // ~46ms at 44.1kHz
    static constexpr int HOP_SIZE = GRAIN_SIZE / 2;  // 50% overlap

    struct Grain
    {
        float readPos = 0.0f;
        int grainPos = 0;
        bool active = false;
    };

    float cubicInterpolate(float y0, float y1, float y2, float y3, float t);
    float getWindowValue(int pos, int size);
    void processChannel(float* channelData, int numSamples, int channel);

    double sampleRate = 44100.0;

    // Circular delay buffer per channel
    std::array<std::vector<float>, 2> delayBuffer;
    std::array<int, 2> writePos = {0, 0};
    int bufferSize = 16384;

    // Grains per channel
    std::array<std::array<Grain, NUM_GRAINS>, 2> grains;
    std::array<int, 2> samplesSinceLastGrain = {0, 0};

    // Hann window lookup table
    std::vector<float> windowTable;

    // Smoothed pitch parameter
    float targetPitchRatio = 1.0f;
    float currentPitchRatio = 1.0f;
    static constexpr float SMOOTHING_COEFF = 0.995f;
};
