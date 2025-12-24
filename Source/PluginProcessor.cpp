#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SubstrataAudioProcessor::SubstrataAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters(*this, nullptr, juce::Identifier("Substrata"),
        {
            std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { BRIGHTEN_ID, 1 }, "Brighten", 
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f),
            std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { DARKEN_ID, 1 }, "Darken", 
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f),
            std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { CHARACTER_ID, 1 }, "Character", 
                juce::StringArray("Clean", "Gritty"), 0),
            std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { MIX_ID, 1 }, "Mix", 
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f),
            std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { OVERTONE_SHIFT_ID, 1 }, "Overtone Shift", 
                juce::StringArray("-100", "-50", "0", "+50", "+100"), 2),
            std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { GLOBAL_DETUNE_ID, 1 }, "Global Detune",
                juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f),
            std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { MODE_ID, 1 }, "Mode",
                juce::StringArray("Filter", "Synth", "Copy", "Microtonal"), 0),
            std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { WAVEFORM_ID, 1 }, "Waveform",
                juce::StringArray("Sine", "Square", "Saw", "Triangle"), 0),
            std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { STEREO_WIDTH_ID, 1 }, "Stereo Width",
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f),
            std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { KEY_ID, 1 }, "Key",
                juce::StringArray("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"), 0),
            std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { SCALE_ID, 1 }, "Scale",
                juce::StringArray("Ionian (Major)", "Dorian", "Phrygian", "Lydian", "Mixolydian",
                    "Aeolian (Minor)", "Locrian", "Harmonic Minor", "Melodic Minor", "Whole Tone",
                    "Chromatic", "Blues", "Pentatonic Major", "Pentatonic Minor",
                    "Rast", "Bayati", "Hijaz", "Hijaz Kar", "Saba", "Nahawand", "Ajam", "Kurd",
                    "Sikah", "Huzam", "Nawa Athar", "Nikriz", "Athar Kurd",
                    "Hirajoshi", "Iwato", "In Sen", "Chinese", "Pelog", "Slendro",
                    "Raga Bhairav", "Raga Todi",
                    "Double Harmonic", "Hungarian Minor", "Hungarian Major",
                    "Neapolitan Minor", "Neapolitan Major", "Persian", "Enigmatic",
                    "Prometheus", "Tritone", "Augmented", "Diminished (HW)", "Diminished (WH)",
                    "Quarter Chromatic", "Neutral"), 0)
        })
{
}

SubstrataAudioProcessor::~SubstrataAudioProcessor()
{
}

//==============================================================================
const juce::String SubstrataAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SubstrataAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SubstrataAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SubstrataAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SubstrataAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SubstrataAudioProcessor::getNumPrograms()
{
    return 1;
}

int SubstrataAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SubstrataAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SubstrataAudioProcessor::getProgramName(int index)
{
    return {};
}

void SubstrataAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void SubstrataAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Safety checks
    if (sampleRate <= 0.0 || samplesPerBlock <= 0)
        return;

    currentSampleRate = sampleRate;
    pitchTracker.prepare(sampleRate, samplesPerBlock);
    pitchShifter.prepare(sampleRate, samplesPerBlock);
    harmonicGenerator.prepare(sampleRate, samplesPerBlock);

    // Reset FFT state
    std::fill(inputFftData.begin(), inputFftData.end(), 0.0f);
    std::fill(inputSpectrumData.begin(), inputSpectrumData.end(), 0.0f);
    inputFifoIndex = 0;

    std::fill(outputFftData.begin(), outputFftData.end(), 0.0f);
    std::fill(outputSpectrumData.begin(), outputSpectrumData.end(), 0.0f);
    outputFifoIndex = 0;
}

void SubstrataAudioProcessor::releaseResources()
{
    // PitchTracker doesn't have a reset method - it's stateless
    pitchShifter.reset();
    harmonicGenerator.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SubstrataAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
  #endif
}
#endif

void SubstrataAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // Check if parameters are initialized
    auto* brightenParam = parameters.getRawParameterValue(BRIGHTEN_ID);
    auto* darkenParam = parameters.getRawParameterValue(DARKEN_ID);
    auto* characterParam = parameters.getRawParameterValue(CHARACTER_ID);
    auto* mixParam = parameters.getRawParameterValue(MIX_ID);
    auto* overtoneShiftParam = parameters.getRawParameterValue(OVERTONE_SHIFT_ID);
    auto* globalDetuneParam = parameters.getRawParameterValue(GLOBAL_DETUNE_ID);
    auto* modeParam = parameters.getRawParameterValue(MODE_ID);
    auto* waveformParam = parameters.getRawParameterValue(WAVEFORM_ID);
    auto* stereoWidthParam = parameters.getRawParameterValue(STEREO_WIDTH_ID);
    auto* keyParam = parameters.getRawParameterValue(KEY_ID);
    auto* scaleParam = parameters.getRawParameterValue(SCALE_ID);

    if (!brightenParam || !darkenParam || !characterParam || !mixParam ||
        !overtoneShiftParam || !globalDetuneParam || !modeParam || !waveformParam ||
        !stereoWidthParam || !keyParam || !scaleParam)
    {
        return; // Parameters not initialized yet
    }

    // Get parameter values
    float brightenValue = brightenParam->load();
    float darkenValue = darkenParam->load();
    float characterValue = characterParam->load();
    float mixValue = mixParam->load();
    int modeValue = static_cast<int>(modeParam->load());
    int waveformValue = static_cast<int>(waveformParam->load());
    float stereoWidthValue = stereoWidthParam->load();
    int keyValue = static_cast<int>(keyParam->load());
    int scaleValue = static_cast<int>(scaleParam->load());

    // Convert overtone shift choice (0-4) to cents (-100, -50, 0, 50, 100)
    float overtoneShiftChoice = overtoneShiftParam->load();
    float overtoneShiftValue = (overtoneShiftChoice - 2.0f) * 50.0f;

    float globalDetuneValue = globalDetuneParam->load();

    // Process audio
    if (totalNumInputChannels > 0 && buffer.getNumSamples() > 0)
    {
        // Capture INPUT spectrum before processing
        const float* inputLeftData = buffer.getReadPointer(0);
        const float* inputRightData = totalNumInputChannels > 1 ? buffer.getReadPointer(1) : inputLeftData;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushInputSampleIntoFifo((inputLeftData[i] + inputRightData[i]) * 0.5f);
        }

        // Create a mono sum for pitch tracking
        juce::AudioBuffer<float> monoBuffer(1, buffer.getNumSamples());
        monoBuffer.clear();
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            monoBuffer.addFrom(0, 0, buffer, channel, 0, buffer.getNumSamples(), 1.0f / totalNumInputChannels);
        }

        // Track pitch
        float f0 = pitchTracker.processBlock(monoBuffer.getReadPointer(0), buffer.getNumSamples());

        // Set parameters
        harmonicGenerator.setOvertoneShift(overtoneShiftValue);
        harmonicGenerator.setMode(static_cast<HarmonicGenerator::Mode>(modeValue));
        harmonicGenerator.setWaveform(static_cast<HarmonicGenerator::Waveform>(waveformValue));
        harmonicGenerator.setStereoWidth(stereoWidthValue);
        harmonicGenerator.setKey(keyValue);
        harmonicGenerator.setScale(scaleValue);

        // Process harmonics
        HarmonicGenerator::Character character = (characterValue > 0.5f)
            ? HarmonicGenerator::Character::Gritty
            : HarmonicGenerator::Character::Clean;

        harmonicGenerator.processBlock(buffer, f0, brightenValue, darkenValue, character, mixValue);

        // Capture OUTPUT spectrum after harmonic processing
        const float* outputLeftData = buffer.getReadPointer(0);
        const float* outputRightData = totalNumInputChannels > 1 ? buffer.getReadPointer(1) : outputLeftData;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushOutputSampleIntoFifo((outputLeftData[i] + outputRightData[i]) * 0.5f);
        }
    }
    else
    {
        // No input, just set default values
        harmonicGenerator.setOvertoneShift(overtoneShiftValue);
        harmonicGenerator.setMode(static_cast<HarmonicGenerator::Mode>(modeValue));
        harmonicGenerator.setWaveform(static_cast<HarmonicGenerator::Waveform>(waveformValue));
        harmonicGenerator.setStereoWidth(stereoWidthValue);
        harmonicGenerator.setKey(keyValue);
        harmonicGenerator.setScale(scaleValue);
    }

    // Apply global pitch shift to entire signal (original + harmonics)
    if (std::abs(globalDetuneValue) > 0.1f)
    {
        pitchShifter.processBlock(buffer, globalDetuneValue);
    }
}

//==============================================================================
bool SubstrataAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SubstrataAudioProcessor::createEditor()
{
    return new SubstrataAudioProcessorEditor(*this);
}

//==============================================================================
void SubstrataAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SubstrataAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void SubstrataAudioProcessor::pushInputSampleIntoFifo(float sample)
{
    inputFftData[inputFifoIndex++] = sample;

    if (inputFifoIndex >= FFT_SIZE)
    {
        processInputFFT();
        inputFifoIndex = 0;
    }
}

void SubstrataAudioProcessor::pushOutputSampleIntoFifo(float sample)
{
    outputFftData[outputFifoIndex++] = sample;

    if (outputFifoIndex >= FFT_SIZE)
    {
        processOutputFFT();
        outputFifoIndex = 0;
    }
}

void SubstrataAudioProcessor::processInputFFT()
{
    // Apply window
    window.multiplyWithWindowingTable(inputFftData.data(), FFT_SIZE);

    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(inputFftData.data());

    // Convert to decibels and store in spectrum data
    const float minDb = -100.0f;
    const float maxDb = 0.0f;

    for (int i = 0; i < FFT_SIZE / 2; ++i)
    {
        float magnitude = inputFftData[i];
        float db = juce::jlimit(minDb, maxDb,
            juce::Decibels::gainToDecibels(magnitude) - juce::Decibels::gainToDecibels(static_cast<float>(FFT_SIZE)));

        // Normalize to 0-1 range
        inputSpectrumData[i] = (db - minDb) / (maxDb - minDb);
    }
}

void SubstrataAudioProcessor::processOutputFFT()
{
    // Apply window
    window.multiplyWithWindowingTable(outputFftData.data(), FFT_SIZE);

    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(outputFftData.data());

    // Convert to decibels and store in spectrum data
    const float minDb = -100.0f;
    const float maxDb = 0.0f;

    for (int i = 0; i < FFT_SIZE / 2; ++i)
    {
        float magnitude = outputFftData[i];
        float db = juce::jlimit(minDb, maxDb,
            juce::Decibels::gainToDecibels(magnitude) - juce::Decibels::gainToDecibels(static_cast<float>(FFT_SIZE)));

        // Normalize to 0-1 range
        outputSpectrumData[i] = (db - minDb) / (maxDb - minDb);
    }
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SubstrataAudioProcessor();
}

