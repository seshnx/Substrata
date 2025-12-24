#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <array>
#include "PluginProcessor.h"

//==============================================================================
class FrequencyVisualizer : public juce::Component
{
public:
    FrequencyVisualizer() = default;
    ~FrequencyVisualizer() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override {}

    void setF0(float f0) { currentF0 = f0; }
    void setValidPitch(bool valid) { isValidPitch = valid; }
    void setBrightenAmount(float amount) { brightenAmount = amount; }
    void setDarkenAmount(float amount) { darkenAmount = amount; }
    void setOvertoneShiftCents(float cents) { overtoneShiftCents = cents; }
    void updateAnimation();

    // Spectrum analyzer
    static constexpr int SPECTRUM_SIZE = 1024;
    void setInputSpectrumData(const float* data, int size, double sampleRate);
    void setOutputSpectrumData(const float* data, int size);

private:
    float currentF0 = 440.0f;
    bool isValidPitch = false;
    float brightenAmount = 0.0f;
    float darkenAmount = 0.0f;
    float overtoneShiftCents = 0.0f;
    float pulsePhase = 0.0f;

    // Spectrum data (input = original signal, output = after harmonic processing)
    std::array<float, SPECTRUM_SIZE> smoothedInputSpectrum {};
    std::array<float, SPECTRUM_SIZE> smoothedOutputSpectrum {};
    double currentSampleRate = 44100.0;

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawInputSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawOutputSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawHarmonicPoint(juce::Graphics& g, float frequency, float baseRadius,
                          juce::Colour color, juce::Rectangle<float> bounds,
                          const juce::String& label, float intensity);
    void drawPianoKeyboard(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<float> spectrumBounds);
    float frequencyToX(float frequency, juce::Rectangle<float> bounds);
    float midiNoteToFrequency(int midiNote);
    bool isBlackKey(int midiNote);
    juce::String frequencyToNote(float frequency);
};

//==============================================================================
class SubstrataAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    SubstrataAudioProcessorEditor(SubstrataAudioProcessor&);
    ~SubstrataAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    SubstrataAudioProcessor& audioProcessor;

    FrequencyVisualizer visualizer;

    juce::Slider brightenSlider;
    juce::Slider darkenSlider;
    juce::Slider mixSlider;
    juce::Slider globalDetuneSlider;
    juce::Slider stereoWidthSlider;
    juce::ComboBox characterCombo;
    juce::ComboBox overtoneShiftCombo;
    juce::ComboBox modeCombo;
    juce::ComboBox waveformCombo;
    juce::ComboBox keyCombo;
    juce::ComboBox scaleCombo;

    juce::Label brightenLabel;
    juce::Label darkenLabel;
    juce::Label mixLabel;
    juce::Label globalDetuneLabel;
    juce::Label stereoWidthLabel;
    juce::Label characterLabel;
    juce::Label overtoneShiftLabel;
    juce::Label modeLabel;
    juce::Label waveformLabel;
    juce::Label keyLabel;
    juce::Label scaleLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> brightenAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> darkenAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalDetuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> overtoneShiftAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment;

    // Company logo
    juce::Image companyLogo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubstrataAudioProcessorEditor)
};
