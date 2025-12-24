#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

//==============================================================================
// FrequencyVisualizer
//==============================================================================
void FrequencyVisualizer::updateAnimation()
{
    pulsePhase += 0.08f;
    if (pulsePhase > juce::MathConstants<float>::twoPi)
        pulsePhase -= juce::MathConstants<float>::twoPi;
}

void FrequencyVisualizer::setInputSpectrumData(const float* data, int size, double sampleRate)
{
    currentSampleRate = sampleRate;
    int copySize = std::min(size, static_cast<int>(SPECTRUM_SIZE));
    for (int i = 0; i < copySize; ++i)
    {
        // Smooth the spectrum data for visual appeal
        smoothedInputSpectrum[i] = smoothedInputSpectrum[i] * 0.7f + data[i] * 0.3f;
    }
}

void FrequencyVisualizer::setOutputSpectrumData(const float* data, int size)
{
    int copySize = std::min(size, static_cast<int>(SPECTRUM_SIZE));
    for (int i = 0; i < copySize; ++i)
    {
        // Smooth the spectrum data for visual appeal
        smoothedOutputSpectrum[i] = smoothedOutputSpectrum[i] * 0.7f + data[i] * 0.3f;
    }
}

void FrequencyVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    // Background
    g.setColour(juce::Colour(0xff141414));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(0xff252525));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Reserve space for piano keyboard at bottom
    const float keyboardHeight = 28.0f;
    auto keyboardBounds = bounds.removeFromBottom(keyboardHeight + 5.0f);
    keyboardBounds = keyboardBounds.reduced(15.0f, 0.0f).withTrimmedBottom(5.0f);

    auto innerBounds = bounds.reduced(15.0f, 25.0f);
    innerBounds.removeFromBottom(5.0f);  // Space between spectrum and keyboard

    // Title
    g.setColour(juce::Colour(0xffe0e0e0));
    g.setFont(11.0f);
    g.drawText("FREQUENCY ANALYSIS", bounds.removeFromTop(20.0f), juce::Justification::centred);

    // Draw grid
    drawGrid(g, innerBounds);

    // Draw spectrum analyzers (input in teal, output/ghost in purple)
    drawInputSpectrum(g, innerBounds);
    drawOutputSpectrum(g, innerBounds);

    // Draw piano keyboard aligned with spectrum
    drawPianoKeyboard(g, keyboardBounds, innerBounds);

    if (!isValidPitch || currentF0 < 50.0f || currentF0 > 2000.0f)
    {
        g.setColour(juce::Colour(0xff707070));
        g.setFont(12.0f);
        g.drawText("Awaiting Input Signal...", innerBounds, juce::Justification::centred);
        return;
    }

    // Calculate shifted frequencies
    float shiftRatio = std::pow(2.0f, overtoneShiftCents / 1200.0f);

    // Draw harmonics
    drawHarmonicPoint(g, currentF0 * 0.5f * shiftRatio, 8.0f, juce::Colour(0xff6699ff), innerBounds, "0.5f", darkenAmount);
    drawHarmonicPoint(g, currentF0 * 4.0f * shiftRatio, 7.0f, juce::Colour(0xffff9966), innerBounds, "4f", brightenAmount);
    drawHarmonicPoint(g, currentF0 * 2.0f * shiftRatio, 8.0f, juce::Colour(0xffffcc66), innerBounds, "2f", brightenAmount);
    drawHarmonicPoint(g, currentF0, 10.0f, juce::Colours::white, innerBounds, "f0", 1.0f);
}

void FrequencyVisualizer::drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float frequencies[] = { 50.0f, 100.0f, 200.0f, 400.0f, 800.0f, 1600.0f };
    const char* labels[] = { "50", "100", "200", "400", "800", "1.6k" };

    g.setFont(9.0f);

    for (int i = 0; i < 6; ++i)
    {
        float x = frequencyToX(frequencies[i], bounds);
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
        g.setColour(juce::Colour(0xff707070));
        g.drawText(labels[i], static_cast<int>(x) - 20, static_cast<int>(bounds.getBottom()) + 2, 40, 12,
                   juce::Justification::centred);
    }

    g.setColour(juce::Colour(0xff252525));
    g.drawHorizontalLine(static_cast<int>(bounds.getCentreY()), bounds.getX(), bounds.getRight());
}

void FrequencyVisualizer::drawInputSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float minFreq = 40.0f;
    const float maxFreq = 2500.0f;
    const int numBins = SPECTRUM_SIZE;

    // Create path for input spectrum
    juce::Path spectrumPath;
    bool pathStarted = false;

    float prevX = bounds.getX();

    for (int i = 1; i < numBins; ++i)
    {
        float binFreq = static_cast<float>(i) * static_cast<float>(currentSampleRate) / static_cast<float>(numBins * 2);

        if (binFreq < minFreq || binFreq > maxFreq)
            continue;

        float x = frequencyToX(binFreq, bounds);
        float magnitude = smoothedInputSpectrum[i];

        float height = magnitude * bounds.getHeight() * 0.9f;
        float y = bounds.getBottom() - height;

        if (!pathStarted)
        {
            spectrumPath.startNewSubPath(x, bounds.getBottom());
            pathStarted = true;
        }

        spectrumPath.lineTo(x, y);
        prevX = x;
    }

    if (pathStarted)
    {
        spectrumPath.lineTo(prevX, bounds.getBottom());
        spectrumPath.closeSubPath();

        // Input spectrum: teal/cyan color
        juce::ColourGradient gradient(
            juce::Colour(0x3066ffcc), bounds.getX(), bounds.getY(),
            juce::Colour(0x0866ffcc), bounds.getX(), bounds.getBottom(),
            false);
        g.setGradientFill(gradient);
        g.fillPath(spectrumPath);

        g.setColour(juce::Colour(0x6066ffcc));
        g.strokePath(spectrumPath, juce::PathStrokeType(1.0f));
    }
}

void FrequencyVisualizer::drawOutputSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float minFreq = 40.0f;
    const float maxFreq = 2500.0f;
    const int numBins = SPECTRUM_SIZE;

    // Create path for output spectrum (ghost)
    juce::Path spectrumPath;
    bool pathStarted = false;

    float prevX = bounds.getX();

    for (int i = 1; i < numBins; ++i)
    {
        float binFreq = static_cast<float>(i) * static_cast<float>(currentSampleRate) / static_cast<float>(numBins * 2);

        if (binFreq < minFreq || binFreq > maxFreq)
            continue;

        float x = frequencyToX(binFreq, bounds);
        float magnitude = smoothedOutputSpectrum[i];

        float height = magnitude * bounds.getHeight() * 0.9f;
        float y = bounds.getBottom() - height;

        if (!pathStarted)
        {
            spectrumPath.startNewSubPath(x, bounds.getBottom());
            pathStarted = true;
        }

        spectrumPath.lineTo(x, y);
        prevX = x;
    }

    if (pathStarted)
    {
        spectrumPath.lineTo(prevX, bounds.getBottom());
        spectrumPath.closeSubPath();

        // Output spectrum (ghost): purple/magenta color
        juce::ColourGradient gradient(
            juce::Colour(0x40cc66ff), bounds.getX(), bounds.getY(),
            juce::Colour(0x10cc66ff), bounds.getX(), bounds.getBottom(),
            false);
        g.setGradientFill(gradient);
        g.fillPath(spectrumPath);

        g.setColour(juce::Colour(0x80cc66ff));
        g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
    }
}

void FrequencyVisualizer::drawHarmonicPoint(juce::Graphics& g, float frequency, float baseRadius,
                                            juce::Colour color, juce::Rectangle<float> bounds,
                                            const juce::String& label, float intensity)
{
    if (frequency < 30.0f || frequency > 3000.0f)
        return;

    float x = frequencyToX(frequency, bounds);
    float centerY = bounds.getCentreY();

    if (x < bounds.getX() || x > bounds.getRight())
        return;

    float pulse = 1.0f + 0.15f * std::sin(pulsePhase) * intensity;
    float radius = baseRadius * pulse * (0.5f + 0.5f * intensity);

    // Glow
    for (int i = 3; i >= 1; --i)
    {
        float glowR = radius * static_cast<float>(i);
        g.setColour(color.withAlpha(0.1f * intensity));
        g.fillEllipse(x - glowR, centerY - glowR, glowR * 2.0f, glowR * 2.0f);
    }

    // Vertical line
    g.setColour(color.withAlpha(0.4f * intensity));
    g.drawLine(x, centerY, x, bounds.getBottom(), 1.0f);

    // Main point
    g.setColour(color.withAlpha(0.3f + 0.7f * intensity));
    g.fillEllipse(x - radius, centerY - radius, radius * 2.0f, radius * 2.0f);

    // Core
    float coreR = radius * 0.5f;
    g.setColour(color);
    g.fillEllipse(x - coreR, centerY - coreR, coreR * 2.0f, coreR * 2.0f);

    // Label
    g.setColour(color.withAlpha(0.6f + 0.4f * intensity));
    g.setFont(9.0f);
    g.drawText(label, static_cast<int>(x) - 15, static_cast<int>(centerY - radius - 18), 30, 14,
               juce::Justification::centred);
}

float FrequencyVisualizer::frequencyToX(float frequency, juce::Rectangle<float> bounds)
{
    const float minFreq = 40.0f;
    const float maxFreq = 2500.0f;

    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logFreq = std::log10(juce::jlimit(minFreq, maxFreq, frequency));

    float normalized = (logFreq - logMin) / (logMax - logMin);
    return bounds.getX() + normalized * bounds.getWidth();
}

juce::String FrequencyVisualizer::frequencyToNote(float frequency)
{
    if (frequency <= 0.0f) return "---";

    const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    float midiNote = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    int roundedNote = static_cast<int>(std::round(midiNote));
    int noteIndex = roundedNote % 12;
    if (noteIndex < 0) noteIndex += 12;
    int octave = (roundedNote / 12) - 1;

    int centsOff = static_cast<int>((midiNote - static_cast<float>(roundedNote)) * 100.0f);
    juce::String centsStr = (centsOff >= 0 ? "+" : "") + juce::String(centsOff);

    return juce::String(noteNames[noteIndex]) + juce::String(octave) + " (" + centsStr + "c)";
}

float FrequencyVisualizer::midiNoteToFrequency(int midiNote)
{
    // A4 (MIDI note 69) = 440 Hz
    return 440.0f * std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);
}

bool FrequencyVisualizer::isBlackKey(int midiNote)
{
    int noteInOctave = midiNote % 12;
    // Black keys are: C#(1), D#(3), F#(6), G#(8), A#(10)
    return noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
           noteInOctave == 8 || noteInOctave == 10;
}

void FrequencyVisualizer::drawPianoKeyboard(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             juce::Rectangle<float> spectrumBounds)
{
    const float minFreq = 40.0f;
    const float maxFreq = 2500.0f;

    // Find MIDI note range for our frequency range
    // MIDI note = 69 + 12 * log2(freq / 440)
    int minNote = static_cast<int>(69.0f + 12.0f * std::log2(minFreq / 440.0f));
    int maxNote = static_cast<int>(69.0f + 12.0f * std::log2(maxFreq / 440.0f)) + 1;

    // Clamp to valid MIDI range
    minNote = juce::jmax(0, minNote);
    maxNote = juce::jmin(127, maxNote);

    float whiteKeyHeight = bounds.getHeight();
    float blackKeyHeight = whiteKeyHeight * 0.6f;

    // First pass: draw white keys
    for (int note = minNote; note <= maxNote; ++note)
    {
        if (isBlackKey(note))
            continue;

        float noteFreq = midiNoteToFrequency(note);
        float nextWhiteFreq = noteFreq;

        // Find next white key
        for (int n = note + 1; n <= maxNote + 1; ++n)
        {
            if (!isBlackKey(n))
            {
                nextWhiteFreq = midiNoteToFrequency(n);
                break;
            }
        }

        float x1 = frequencyToX(noteFreq, spectrumBounds);
        float x2 = frequencyToX(nextWhiteFreq, spectrumBounds);

        // Clamp to bounds
        x1 = juce::jmax(x1, spectrumBounds.getX());
        x2 = juce::jmin(x2, spectrumBounds.getRight());

        if (x2 <= x1)
            continue;

        juce::Rectangle<float> keyRect(x1, bounds.getY(), x2 - x1, whiteKeyHeight);

        // Highlight if this is the detected fundamental
        int noteInOctave = note % 12;
        bool isCurrentNote = false;
        if (isValidPitch && currentF0 >= minFreq && currentF0 <= maxFreq)
        {
            float detectedMidi = 69.0f + 12.0f * std::log2(currentF0 / 440.0f);
            int detectedNote = static_cast<int>(std::round(detectedMidi)) % 12;
            isCurrentNote = (noteInOctave == detectedNote);
        }

        // Draw white key
        if (isCurrentNote)
            g.setColour(juce::Colour(0xff66ccff));  // Highlighted blue
        else
            g.setColour(juce::Colour(0xffe8e8e8));  // Off-white

        g.fillRect(keyRect);

        // Key border
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(keyRect, 0.5f);

        // Draw note name on C keys
        if (noteInOctave == 0 && (x2 - x1) > 12.0f)
        {
            int octave = (note / 12) - 1;
            g.setColour(juce::Colour(0xff505050));
            g.setFont(8.0f);
            g.drawText("C" + juce::String(octave), keyRect.withTrimmedTop(whiteKeyHeight * 0.5f),
                       juce::Justification::centred);
        }
    }

    // Second pass: draw black keys on top
    for (int note = minNote; note <= maxNote; ++note)
    {
        if (!isBlackKey(note))
            continue;

        float noteFreq = midiNoteToFrequency(note);

        // Black keys are drawn between the surrounding white keys
        float prevWhiteFreq = midiNoteToFrequency(note - 1);
        float nextWhiteFreq = midiNoteToFrequency(note + 1);

        // Position black key centered on its frequency
        float centerX = frequencyToX(noteFreq, spectrumBounds);
        float leftX = frequencyToX(prevWhiteFreq, spectrumBounds);
        float rightX = frequencyToX(nextWhiteFreq, spectrumBounds);

        // Black key width is proportional to the gap
        float keyWidth = (rightX - leftX) * 0.5f;
        keyWidth = juce::jmax(keyWidth, 3.0f);  // Minimum width

        float x1 = centerX - keyWidth * 0.5f;
        float x2 = centerX + keyWidth * 0.5f;

        // Clamp to bounds
        if (x2 < spectrumBounds.getX() || x1 > spectrumBounds.getRight())
            continue;

        x1 = juce::jmax(x1, spectrumBounds.getX());
        x2 = juce::jmin(x2, spectrumBounds.getRight());

        juce::Rectangle<float> keyRect(x1, bounds.getY(), x2 - x1, blackKeyHeight);

        // Highlight if this is the detected fundamental
        bool isCurrentNote = false;
        if (isValidPitch && currentF0 >= minFreq && currentF0 <= maxFreq)
        {
            float detectedMidi = 69.0f + 12.0f * std::log2(currentF0 / 440.0f);
            int detectedNote = static_cast<int>(std::round(detectedMidi)) % 12;
            isCurrentNote = ((note % 12) == detectedNote);
        }

        // Draw black key
        if (isCurrentNote)
            g.setColour(juce::Colour(0xff3399ff));  // Highlighted blue
        else
            g.setColour(juce::Colour(0xff1a1a1a));  // Dark

        g.fillRect(keyRect);

        // Subtle highlight on top edge
        g.setColour(juce::Colour(0xff404040));
        g.drawHorizontalLine(static_cast<int>(keyRect.getY()), keyRect.getX(), keyRect.getRight());
    }

    // Draw indicator line from detected pitch
    if (isValidPitch && currentF0 >= minFreq && currentF0 <= maxFreq)
    {
        float pitchX = frequencyToX(currentF0, spectrumBounds);
        g.setColour(juce::Colour(0xaaffffff));
        g.drawVerticalLine(static_cast<int>(pitchX), bounds.getY(), bounds.getBottom());
    }
}

//==============================================================================
// SubstrataAudioProcessorEditor
//==============================================================================
SubstrataAudioProcessorEditor::SubstrataAudioProcessorEditor(SubstrataAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(750, 595);

    // Load company logo
    companyLogo = juce::ImageCache::getFromMemory(BinaryData::company_logo_png, BinaryData::company_logo_pngSize);
    if (!companyLogo.isValid())
    {
        juce::MemoryInputStream stream(BinaryData::company_logo_png, BinaryData::company_logo_pngSize, false);
        auto format = juce::ImageFileFormat::findImageFormatForStream(stream);
        if (format != nullptr)
        {
            stream.setPosition(0);
            companyLogo = format->decodeImage(stream);
        }
    }

    // Visualizer
    addAndMakeVisible(visualizer);

    // Setup sliders
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& name,
                              double min, double max, double def)
    {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        slider.setRange(min, max, 0.01);
        slider.setValue(def);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff252525));
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe0e0e0));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff141414));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff252525));
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour(0xff707070));
        label.setFont(juce::Font(10.0f));
        addAndMakeVisible(label);
    };

    setupSlider(brightenSlider, brightenLabel, "BRIGHTEN", 0.0, 1.0, 0.0);
    setupSlider(darkenSlider, darkenLabel, "DARKEN", 0.0, 1.0, 0.0);
    setupSlider(mixSlider, mixLabel, "MIX", 0.0, 1.0, 0.5);
    setupSlider(globalDetuneSlider, globalDetuneLabel, "DETUNE", -100.0, 100.0, 0.0);
    setupSlider(stereoWidthSlider, stereoWidthLabel, "WIDTH", 0.0, 1.0, 0.0);

    // Setup combos
    auto setupCombo = [this](juce::ComboBox& combo, juce::Label& label, const juce::String& name)
    {
        combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff141414));
        combo.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe0e0e0));
        combo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff252525));
        combo.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe0e0e0));
        combo.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(combo);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour(0xff707070));
        label.setFont(juce::Font(10.0f));
        addAndMakeVisible(label);
    };

    characterCombo.addItem("CLEAN", 1);
    characterCombo.addItem("GRITTY", 2);
    characterCombo.setSelectedId(1);
    setupCombo(characterCombo, characterLabel, "CHARACTER");

    overtoneShiftCombo.addItem("-100c", 1);
    overtoneShiftCombo.addItem("-50c", 2);
    overtoneShiftCombo.addItem("0c", 3);
    overtoneShiftCombo.addItem("+50c", 4);
    overtoneShiftCombo.addItem("+100c", 5);
    overtoneShiftCombo.setSelectedId(3);
    setupCombo(overtoneShiftCombo, overtoneShiftLabel, "SHIFT");

    modeCombo.addItem("FILTER", 1);
    modeCombo.addItem("SYNTH", 2);
    modeCombo.addItem("COPY", 3);
    modeCombo.addItem("MICROTONAL", 4);
    modeCombo.setSelectedId(1);
    setupCombo(modeCombo, modeLabel, "MODE");

    waveformCombo.addItem("SINE", 1);
    waveformCombo.addItem("SQUARE", 2);
    waveformCombo.addItem("SAW", 3);
    waveformCombo.addItem("TRI", 4);
    waveformCombo.setSelectedId(1);
    setupCombo(waveformCombo, waveformLabel, "WAVE");

    // Key combo
    keyCombo.addItem("C", 1);
    keyCombo.addItem("C#", 2);
    keyCombo.addItem("D", 3);
    keyCombo.addItem("D#", 4);
    keyCombo.addItem("E", 5);
    keyCombo.addItem("F", 6);
    keyCombo.addItem("F#", 7);
    keyCombo.addItem("G", 8);
    keyCombo.addItem("G#", 9);
    keyCombo.addItem("A", 10);
    keyCombo.addItem("A#", 11);
    keyCombo.addItem("B", 12);
    keyCombo.setSelectedId(1);
    setupCombo(keyCombo, keyLabel, "KEY");

    // Scale combo
    const char* scaleNames[] = {
        "Ionian (Major)", "Dorian", "Phrygian", "Lydian", "Mixolydian",
        "Aeolian (Minor)", "Locrian", "Harmonic Minor", "Melodic Minor", "Whole Tone",
        "Chromatic", "Blues", "Pentatonic Major", "Pentatonic Minor",
        "Rast", "Bayati", "Hijaz", "Hijaz Kar", "Saba", "Nahawand", "Ajam", "Kurd",
        "Sikah", "Huzam", "Nawa Athar", "Nikriz", "Athar Kurd",
        "Hirajoshi", "Iwato", "In Sen", "Chinese", "Pelog", "Slendro",
        "Raga Bhairav", "Raga Todi",
        "Double Harmonic", "Hungarian Minor", "Hungarian Major",
        "Neapolitan Minor", "Neapolitan Major", "Persian", "Enigmatic",
        "Prometheus", "Tritone", "Augmented", "Diminished (HW)", "Diminished (WH)",
        "Quarter Chromatic", "Neutral"
    };
    for (int i = 0; i < 49; ++i)
        scaleCombo.addItem(scaleNames[i], i + 1);
    scaleCombo.setSelectedId(1);
    setupCombo(scaleCombo, scaleLabel, "SCALE");

    // Attachments
    brightenAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::BRIGHTEN_ID, brightenSlider);
    darkenAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::DARKEN_ID, darkenSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::MIX_ID, mixSlider);
    globalDetuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::GLOBAL_DETUNE_ID, globalDetuneSlider);
    stereoWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::STEREO_WIDTH_ID, stereoWidthSlider);
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::CHARACTER_ID, characterCombo);
    overtoneShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::OVERTONE_SHIFT_ID, overtoneShiftCombo);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::MODE_ID, modeCombo);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::WAVEFORM_ID, waveformCombo);
    keyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::KEY_ID, keyCombo);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), SubstrataAudioProcessor::SCALE_ID, scaleCombo);

    startTimerHz(30);
}

SubstrataAudioProcessorEditor::~SubstrataAudioProcessorEditor()
{
    stopTimer();
}

void SubstrataAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a));

    // Header
    g.setColour(juce::Colours::white);
    g.setFont(22.0f);
    g.drawText("SUBSTRATA", 15, 10, 200, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff707070));
    g.setFont(10.0f);
    g.drawText("HARMONIC SHIFTER", 200, 18, 150, 20, juce::Justification::centredLeft);

    // Draw company logo (replaces text "SeshNx")
    if (companyLogo.isValid())
    {
        float logoHeight = 30.0f;
        float logoAspect = static_cast<float>(companyLogo.getWidth()) / static_cast<float>(companyLogo.getHeight());
        float logoWidth = logoHeight * logoAspect;
        float logoX = static_cast<float>(getWidth()) - logoWidth - 15.0f;
        float logoY = (50.0f - logoHeight) * 0.5f;
        juce::Rectangle<float> logoBounds(logoX, logoY, logoWidth, logoHeight);
        g.drawImage(companyLogo, logoBounds, juce::RectanglePlacement::centred);
    }
    else
    {
        g.drawText("SeshNx", getWidth() - 80, 18, 70, 20, juce::Justification::centredRight);
    }

    g.setColour(juce::Colour(0xff252525));
    g.drawHorizontalLine(50, 15.0f, static_cast<float>(getWidth() - 15));
}

void SubstrataAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(55);

    visualizer.setBounds(bounds.removeFromTop(200).reduced(15, 10));
    bounds.removeFromTop(5);

    auto controlArea = bounds.reduced(15, 0);
    int knobW = 75, knobH = 95;
    int comboW = 70, comboH = 26;
    int spacing = 6;

    // Row 1: Mode/Wave | Darken | Brighten | Character | Mix | Width | Shift | Detune
    int row1Y = controlArea.getY();
    int comboY = row1Y + (knobH - comboH * 2 - 8) / 2;

    // Calculate total width and center (4 combos + 5 knobs)
    int totalW = comboW * 4 + knobW * 5 + spacing * 8;
    int x = controlArea.getX() + (controlArea.getWidth() - totalW) / 2;

    // Mode combo (top of pair)
    modeCombo.setBounds(x, comboY, comboW, comboH);
    modeLabel.setBounds(x, comboY - 15, comboW, 14);
    // Waveform combo (bottom of pair)
    waveformCombo.setBounds(x, comboY + comboH + 8, comboW, comboH);
    waveformLabel.setBounds(x, comboY + comboH * 2 + 12, comboW, 14);
    x += comboW + spacing;

    darkenSlider.setBounds(x, row1Y, knobW, knobH);
    darkenLabel.setBounds(x, row1Y + knobH, knobW, 16);
    x += knobW + spacing;

    brightenSlider.setBounds(x, row1Y, knobW, knobH);
    brightenLabel.setBounds(x, row1Y + knobH, knobW, 16);
    x += knobW + spacing;

    characterCombo.setBounds(x, comboY + (comboH + 8) / 2, comboW, comboH);
    characterLabel.setBounds(x, comboY + (comboH + 8) / 2 + comboH + 5, comboW, 14);
    x += comboW + spacing;

    mixSlider.setBounds(x, row1Y, knobW, knobH);
    mixLabel.setBounds(x, row1Y + knobH, knobW, 16);
    x += knobW + spacing;

    stereoWidthSlider.setBounds(x, row1Y, knobW, knobH);
    stereoWidthLabel.setBounds(x, row1Y + knobH, knobW, 16);
    x += knobW + spacing;

    overtoneShiftCombo.setBounds(x, comboY + (comboH + 8) / 2, comboW, comboH);
    overtoneShiftLabel.setBounds(x, comboY + (comboH + 8) / 2 + comboH + 5, comboW, 14);
    x += comboW + spacing;

    globalDetuneSlider.setBounds(x, row1Y, knobW, knobH);
    globalDetuneLabel.setBounds(x, row1Y + knobH, knobW, 16);

    // Row 2: Key and Scale for Microtonal mode
    int row2Y = row1Y + knobH + 30;
    int keyComboW = 60;
    int scaleComboW = 180;  // Wider for scale names
    int row2TotalW = keyComboW + scaleComboW + spacing;
    int row2X = controlArea.getX() + (controlArea.getWidth() - row2TotalW) / 2;

    keyLabel.setBounds(row2X, row2Y, keyComboW, 14);
    keyCombo.setBounds(row2X, row2Y + 16, keyComboW, comboH);
    row2X += keyComboW + spacing;

    scaleLabel.setBounds(row2X, row2Y, scaleComboW, 14);
    scaleCombo.setBounds(row2X, row2Y + 16, scaleComboW, comboH);
}

void SubstrataAudioProcessorEditor::timerCallback()
{
    visualizer.setF0(audioProcessor.getCurrentF0());
    visualizer.setValidPitch(audioProcessor.isValidPitch());
    visualizer.setBrightenAmount(static_cast<float>(brightenSlider.getValue()));
    visualizer.setDarkenAmount(static_cast<float>(darkenSlider.getValue()));

    int shiftIdx = overtoneShiftCombo.getSelectedItemIndex();
    visualizer.setOvertoneShiftCents(static_cast<float>((shiftIdx - 2) * 50));

    // Update spectrum analyzers (input and output/ghost)
    const auto& inputSpectrum = audioProcessor.getInputSpectrumData();
    const auto& outputSpectrum = audioProcessor.getOutputSpectrumData();
    visualizer.setInputSpectrumData(inputSpectrum.data(), static_cast<int>(inputSpectrum.size()), audioProcessor.getSampleRate());
    visualizer.setOutputSpectrumData(outputSpectrum.data(), static_cast<int>(outputSpectrum.size()));

    visualizer.updateAnimation();
    visualizer.repaint();
}
