#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "ScaleDatabase.h"
#include <array>
#include <vector>
#include <cmath>

/**
 * Harmonic generator with multiple modes:
 * - Filter mode: Extract harmonics via bandpass, pitch-shift, and mix back
 * - Synth mode: Synthesize waveforms at harmonic frequencies
 * - Copy mode: Copy fundamental to harmonic octaves
 * - Microtonal mode: Generate harmonics based on musical scales
 */
class HarmonicGenerator
{
public:
    enum class Mode { Filter, Synth, Copy, Microtonal };
    enum class Waveform { Sine, Square, Saw, Triangle };
    enum class Character { Clean, Gritty };

    HarmonicGenerator();
    ~HarmonicGenerator() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, float f0,
                      float brightenAmount, float darkenAmount,
                      Character character, float mix);
    void reset();

    void setOvertoneShift(float cents) { targetOvertoneShift = cents; }
    void setMode(Mode m) { currentMode = m; }
    void setWaveform(Waveform w) { currentWaveform = w; }
    void setStereoWidth(float width) { targetStereoWidth = width; }

    // Microtonal settings
    void setKey(int k) { currentKey = juce::jlimit(0, 11, k); }
    void setScale(int s) { currentScaleIndex = juce::jlimit(0, ScaleDatabase::getNumScales() - 1, s); }
    void setMicrotonalDegrees(int d1, int d2, int d3) {
        harmonicDegrees[0] = d1;
        harmonicDegrees[1] = d2;
        harmonicDegrees[2] = d3;
    }

private:
    // Smoothed parameter helper
    struct SmoothedParam
    {
        float current = 0.0f;
        float target = 0.0f;
        void setTarget(float t) { target = t; }
        float getNext(float coeff) {
            current = current * coeff + target * (1.0f - coeff);
            return current;
        }
        void snapTo(float v) { current = target = v; }
    };

    // Mini pitch shifter for each harmonic band
    struct BandShifter
    {
        static constexpr int GRAIN_SIZE = 1024;
        static constexpr int NUM_GRAINS = 2;

        struct Grain {
            float readPos = 0.0f;
            int grainPos = 0;
            bool active = false;
        };

        std::vector<float> buffer;
        int bufferSize = 8192;
        int writePos = 0;
        std::array<Grain, NUM_GRAINS> grains;
        int samplesSinceGrain = 0;
        std::vector<float> window;

        void prepare(int size);
        void reset();
        float process(float input, float pitchRatio);
        float cubicInterp(float y0, float y1, float y2, float y3, float t);
    };

    // Processing methods
    void processFilterMode(juce::AudioBuffer<float>& buffer, float f0,
                           float brighten, float darken, Character character, float mix);
    void processSynthMode(juce::AudioBuffer<float>& buffer, float f0,
                          float brighten, float darken, Character character, float mix);
    void processCopyMode(juce::AudioBuffer<float>& buffer, float f0,
                         float brighten, float darken, Character character, float mix);
    void processMicrotonalMode(juce::AudioBuffer<float>& buffer, float f0,
                               float brighten, float darken, Character character, float mix);

    float generateWaveform(float phase, Waveform waveform);
    float applySaturation(float input, Character character);
    float processDCBlock(float input, int channel);

    double sampleRate = 44100.0;
    int blockSize = 512;

    Mode currentMode = Mode::Filter;
    Waveform currentWaveform = Waveform::Sine;

    // Smoothed parameters
    SmoothedParam smoothF0;
    SmoothedParam smoothBrighten;
    SmoothedParam smoothDarken;
    SmoothedParam smoothMix;
    SmoothedParam smoothOvertoneShift;
    float targetOvertoneShift = 0.0f;
    SmoothedParam smoothStereoWidth;
    float targetStereoWidth = 0.0f;

    static constexpr float PARAM_SMOOTHING = 0.997f;
    static constexpr float FREQ_SMOOTHING = 0.99f;

    // Bandpass filters for harmonic extraction (stereo)
    static constexpr int NUM_BANDS = 3;  // 0.5f0, 2f0, 4f0
    std::array<std::array<juce::dsp::IIR::Filter<float>, 2>, NUM_BANDS> bandpassFilters;
    std::array<float, NUM_BANDS> lastFilterFreqs = {0.0f, 0.0f, 0.0f};

    // Per-band pitch shifters (stereo)
    std::array<std::array<BandShifter, 2>, NUM_BANDS> bandShifters;

    // Copy mode: bandpass filter for extracting fundamental (stereo)
    std::array<juce::dsp::IIR::Filter<float>, 2> fundamentalBandpass;
    float lastFundamentalFreq = 0.0f;
    // Copy mode: pitch shifters to copy fundamental to each harmonic (stereo)
    std::array<std::array<BandShifter, 2>, NUM_BANDS> copyShifters;

    // Microtonal mode settings
    int currentKey = 0;  // 0 = C, 1 = C#, ..., 11 = B
    int currentScaleIndex = 0;
    std::array<int, NUM_BANDS> harmonicDegrees = {2, 4, 6};  // Default: 3rd, 5th, 7th (0-indexed: 2, 4, 6)
    // Microtonal mode: pitch shifters for scale-based harmonics (stereo)
    std::array<std::array<BandShifter, 2>, NUM_BANDS> microtonalShifters;
    std::array<float, NUM_BANDS> lastMicrotonalFreqs = {0.0f, 0.0f, 0.0f};

    // Synth mode phase accumulators (stereo)
    std::array<std::array<float, 2>, NUM_BANDS> phases;

    // Envelope followers (stereo)
    std::array<float, 2> envFollower = {0.0f, 0.0f};

    // DC blocking (stereo)
    std::array<float, 2> dcBlockIn = {0.0f, 0.0f};
    std::array<float, 2> dcBlockOut = {0.0f, 0.0f};
};
