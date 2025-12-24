#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include "PitchTracker.h"
#include "PitchShifter.h"
#include "HarmonicGenerator.h"
#include "ScaleDatabase.h"

//==============================================================================
/**
    Audio processor for SeshNx Substrata - Harmonic Shifter
*/
class SubstrataAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    SubstrataAudioProcessor();
    ~SubstrataAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

    // Parameter IDs
    static constexpr const char* BRIGHTEN_ID = "brighten";
    static constexpr const char* DARKEN_ID = "darken";
    static constexpr const char* CHARACTER_ID = "character";
    static constexpr const char* MIX_ID = "mix";
    static constexpr const char* OVERTONE_SHIFT_ID = "overtoneShift";
    static constexpr const char* GLOBAL_DETUNE_ID = "globalDetune";
    static constexpr const char* MODE_ID = "mode";
    static constexpr const char* WAVEFORM_ID = "waveform";
    static constexpr const char* STEREO_WIDTH_ID = "stereoWidth";
    static constexpr const char* KEY_ID = "key";
    static constexpr const char* SCALE_ID = "scale";

    float getCurrentF0() const
    {
        return pitchTracker.getCurrentF0();
    }
    bool isValidPitch() const
    {
        return pitchTracker.isValidPitch();
    }

    // Spectrum analyzer
    static constexpr int FFT_ORDER = 11;  // 2048 samples
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    const std::array<float, FFT_SIZE / 2>& getInputSpectrumData() const { return inputSpectrumData; }
    const std::array<float, FFT_SIZE / 2>& getOutputSpectrumData() const { return outputSpectrumData; }
    double getSampleRate() const { return currentSampleRate; }

private:
    void pushInputSampleIntoFifo(float sample);
    void pushOutputSampleIntoFifo(float sample);
    void processInputFFT();
    void processOutputFFT();
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    
    // DSP components
    PitchTracker pitchTracker;
    PitchShifter pitchShifter;
    HarmonicGenerator harmonicGenerator;

    // FFT for spectrum analyzer (input and output)
    juce::dsp::FFT fft { FFT_ORDER };
    juce::dsp::WindowingFunction<float> window { FFT_SIZE, juce::dsp::WindowingFunction<float>::hann };

    // Input spectrum (before harmonic processing)
    // Note: FFT requires 2*FFT_SIZE for performFrequencyOnlyForwardTransform
    std::array<float, FFT_SIZE * 2> inputFftData {};
    std::array<float, FFT_SIZE / 2> inputSpectrumData {};
    int inputFifoIndex = 0;

    // Output spectrum (after harmonic processing)
    // Note: FFT requires 2*FFT_SIZE for performFrequencyOnlyForwardTransform
    std::array<float, FFT_SIZE * 2> outputFftData {};
    std::array<float, FFT_SIZE / 2> outputSpectrumData {};
    int outputFifoIndex = 0;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubstrataAudioProcessor)
};

