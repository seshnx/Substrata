#include "HarmonicGenerator.h"

//==============================================================================
// BandShifter implementation
//==============================================================================
void HarmonicGenerator::BandShifter::prepare(int size)
{
    bufferSize = size;
    buffer.resize(static_cast<size_t>(bufferSize), 0.0f);

    // Pre-compute Hann window
    window.resize(GRAIN_SIZE);
    for (int i = 0; i < GRAIN_SIZE; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(GRAIN_SIZE - 1);
        window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * t));
    }

    reset();
}

void HarmonicGenerator::BandShifter::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
    samplesSinceGrain = 0;
    for (auto& g : grains)
    {
        g.readPos = 0.0f;
        g.grainPos = 0;
        g.active = false;
    }
}

float HarmonicGenerator::BandShifter::process(float input, float pitchRatio)
{
    // Write to buffer
    buffer[writePos] = input;

    // Start new grain if needed
    samplesSinceGrain++;
    if (samplesSinceGrain >= GRAIN_SIZE / 2)
    {
        samplesSinceGrain = 0;
        for (auto& g : grains)
        {
            if (!g.active)
            {
                g.active = true;
                g.grainPos = 0;
                g.readPos = static_cast<float>(writePos - GRAIN_SIZE * 2);
                if (g.readPos < 0.0f) g.readPos += static_cast<float>(bufferSize);
                break;
            }
        }
    }

    // Sum active grains
    float output = 0.0f;
    for (auto& g : grains)
    {
        if (!g.active) continue;

        float win = window[g.grainPos];
        int idx = static_cast<int>(g.readPos);
        float frac = g.readPos - static_cast<float>(idx);

        int i0 = (idx - 1 + bufferSize) % bufferSize;
        int i1 = idx % bufferSize;
        int i2 = (idx + 1) % bufferSize;
        int i3 = (idx + 2) % bufferSize;

        float sample = cubicInterp(buffer[i0], buffer[i1], buffer[i2], buffer[i3], frac);
        output += sample * win;

        g.grainPos++;
        g.readPos += pitchRatio;
        if (g.readPos >= static_cast<float>(bufferSize)) g.readPos -= static_cast<float>(bufferSize);
        if (g.readPos < 0.0f) g.readPos += static_cast<float>(bufferSize);

        if (g.grainPos >= GRAIN_SIZE) g.active = false;
    }

    writePos = (writePos + 1) % bufferSize;
    return output;
}

float HarmonicGenerator::BandShifter::cubicInterp(float y0, float y1, float y2, float y3, float t)
{
    float t2 = t * t, t3 = t2 * t;
    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    return a0 * t3 + a1 * t2 + a2 * t + y1;
}

//==============================================================================
// HarmonicGenerator implementation
//==============================================================================
HarmonicGenerator::HarmonicGenerator()
{
    for (auto& bandPhases : phases)
        for (auto& p : bandPhases)
            p = 0.0f;
}

void HarmonicGenerator::prepare(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlock;

    // Prepare bandpass filters
    for (int band = 0; band < NUM_BANDS; ++band)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            bandpassFilters[band][ch].reset();
            bandpassFilters[band][ch].coefficients =
                juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 440.0, 2.0);
        }
        lastFilterFreqs[band] = 0.0f;
    }

    // Prepare band shifters
    int shifterBufferSize = static_cast<int>(sampleRate * 0.2);
    for (int band = 0; band < NUM_BANDS; ++band)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            bandShifters[band][ch].prepare(shifterBufferSize);
        }
    }

    // Prepare Copy mode components
    for (int ch = 0; ch < 2; ++ch)
    {
        fundamentalBandpass[ch].reset();
        fundamentalBandpass[ch].coefficients =
            juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 440.0, 2.0);
    }
    lastFundamentalFreq = 0.0f;

    for (int band = 0; band < NUM_BANDS; ++band)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            copyShifters[band][ch].prepare(shifterBufferSize);
        }
    }

    // Prepare Microtonal mode pitch shifters
    for (int band = 0; band < NUM_BANDS; ++band)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            microtonalShifters[band][ch].prepare(shifterBufferSize);
        }
        lastMicrotonalFreqs[band] = 0.0f;
    }

    reset();
}

void HarmonicGenerator::reset()
{
    for (int band = 0; band < NUM_BANDS; ++band)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            bandpassFilters[band][ch].reset();
            bandShifters[band][ch].reset();
            copyShifters[band][ch].reset();
            microtonalShifters[band][ch].reset();
            phases[band][ch] = 0.0f;
        }
        lastFilterFreqs[band] = 0.0f;
        lastMicrotonalFreqs[band] = 0.0f;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        fundamentalBandpass[ch].reset();
        envFollower[ch] = 0.0f;
        dcBlockIn[ch] = 0.0f;
        dcBlockOut[ch] = 0.0f;
    }
    lastFundamentalFreq = 0.0f;

    smoothF0.snapTo(440.0f);
    smoothBrighten.snapTo(0.0f);
    smoothDarken.snapTo(0.0f);
    smoothMix.snapTo(0.5f);
    smoothOvertoneShift.snapTo(0.0f);
    smoothStereoWidth.snapTo(0.0f);
}

void HarmonicGenerator::processBlock(juce::AudioBuffer<float>& buffer, float f0,
                                       float brightenAmount, float darkenAmount,
                                       Character character, float mix)
{
    if (f0 < 50.0f || f0 > 2000.0f)
        return;

    switch (currentMode)
    {
        case Mode::Filter:
            processFilterMode(buffer, f0, brightenAmount, darkenAmount, character, mix);
            break;
        case Mode::Synth:
            processSynthMode(buffer, f0, brightenAmount, darkenAmount, character, mix);
            break;
        case Mode::Copy:
            processCopyMode(buffer, f0, brightenAmount, darkenAmount, character, mix);
            break;
        case Mode::Microtonal:
            processMicrotonalMode(buffer, f0, brightenAmount, darkenAmount, character, mix);
            break;
    }
}

void HarmonicGenerator::processFilterMode(juce::AudioBuffer<float>& buffer, float f0,
                                           float brighten, float darken, Character character, float mix)
{
    smoothF0.setTarget(f0);
    smoothBrighten.setTarget(brighten);
    smoothDarken.setTarget(darken);
    smoothMix.setTarget(mix);
    smoothOvertoneShift.setTarget(targetOvertoneShift);
    smoothStereoWidth.setTarget(targetStereoWidth);

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    const bool isStereo = (numChannels == 2);

    // Harmonic multipliers: 0.5, 2.0, 4.0
    const float harmonicMults[NUM_BANDS] = { 0.5f, 2.0f, 4.0f };
    // Pan positions for each band: -1 (left), 0 (center), +1 (right)
    const float bandPans[NUM_BANDS] = { -1.0f, 0.0f, 1.0f };

    // Process both channels together for stereo width
    for (int i = 0; i < numSamples; ++i)
    {
        float currentF0 = smoothF0.getNext(FREQ_SMOOTHING);
        float currentBrighten = smoothBrighten.getNext(PARAM_SMOOTHING);
        float currentDarken = smoothDarken.getNext(PARAM_SMOOTHING);
        float currentMix = smoothMix.getNext(PARAM_SMOOTHING);
        float currentShift = smoothOvertoneShift.getNext(PARAM_SMOOTHING);
        float currentWidth = smoothStereoWidth.getNext(PARAM_SMOOTHING);

        float pitchRatio = std::pow(2.0f, currentShift / 1200.0f);

        float inputL = buffer.getSample(0, i);
        float inputR = isStereo ? buffer.getSample(1, i) : inputL;
        float dryL = inputL;
        float dryR = inputR;

        float harmonicSumL = 0.0f;
        float harmonicSumR = 0.0f;

        // Process each harmonic band
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            float harmonicFreq = currentF0 * harmonicMults[band];
            harmonicFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45f), harmonicFreq);

            // Update filter coefficients if frequency changed significantly
            if (std::abs(harmonicFreq - lastFilterFreqs[band]) > 1.0f)
            {
                float Q = (band == 0) ? 1.5f : 2.5f;
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, harmonicFreq, Q);
                bandpassFilters[band][0].coefficients = coeffs;
                if (isStereo) bandpassFilters[band][1].coefficients = coeffs;
                lastFilterFreqs[band] = harmonicFreq;
            }

            // Extract and process left channel
            float filteredL = bandpassFilters[band][0].processSample(inputL);
            float shiftedL = bandShifters[band][0].process(filteredL, pitchRatio);

            // Extract and process right channel
            float filteredR = isStereo ? bandpassFilters[band][1].processSample(inputR) : filteredL;
            float shiftedR = isStereo ? bandShifters[band][1].process(filteredR, pitchRatio) : shiftedL;

            // Apply gain based on band type
            float gain = 0.0f;
            if (band == 0)
                gain = currentDarken * 2.0f;
            else
                gain = currentBrighten * (band == 1 ? 0.8f : 0.5f);

            // Apply saturation
            float processedL = applySaturation(shiftedL * gain, character);
            float processedR = applySaturation(shiftedR * gain, character);

            // Apply stereo width panning
            float pan = bandPans[band] * currentWidth;
            float panL = std::cos((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            float panR = std::sin((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

            harmonicSumL += processedL * panL + processedR * (1.0f - panR);
            harmonicSumR += processedR * panR + processedL * (1.0f - panL);
        }

        // DC block
        float wetL = dryL + harmonicSumL;
        float wetR = dryR + harmonicSumR;
        wetL = processDCBlock(wetL, 0);
        if (isStereo) wetR = processDCBlock(wetR, 1);

        // Mix
        buffer.setSample(0, i, (1.0f - currentMix) * dryL + currentMix * wetL);
        if (isStereo) buffer.setSample(1, i, (1.0f - currentMix) * dryR + currentMix * wetR);
    }
}

void HarmonicGenerator::processSynthMode(juce::AudioBuffer<float>& buffer, float f0,
                                          float brighten, float darken, Character character, float mix)
{
    smoothF0.setTarget(f0);
    smoothBrighten.setTarget(brighten);
    smoothDarken.setTarget(darken);
    smoothMix.setTarget(mix);
    smoothOvertoneShift.setTarget(targetOvertoneShift);
    smoothStereoWidth.setTarget(targetStereoWidth);

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    const float twoPi = juce::MathConstants<float>::twoPi;
    const bool isStereo = (numChannels == 2);

    // Harmonic multipliers: 0.5, 2.0, 4.0
    const float harmonicMults[NUM_BANDS] = { 0.5f, 2.0f, 4.0f };
    // Pan positions for each band: -1 (left), 0 (center), +1 (right)
    const float bandPans[NUM_BANDS] = { -1.0f, 0.0f, 1.0f };

    for (int i = 0; i < numSamples; ++i)
    {
        float currentF0 = smoothF0.getNext(FREQ_SMOOTHING);
        float currentBrighten = smoothBrighten.getNext(PARAM_SMOOTHING);
        float currentDarken = smoothDarken.getNext(PARAM_SMOOTHING);
        float currentMix = smoothMix.getNext(PARAM_SMOOTHING);
        float currentShift = smoothOvertoneShift.getNext(PARAM_SMOOTHING);
        float currentWidth = smoothStereoWidth.getNext(PARAM_SMOOTHING);

        float shiftRatio = std::pow(2.0f, currentShift / 1200.0f);

        float inputL = buffer.getSample(0, i);
        float inputR = isStereo ? buffer.getSample(1, i) : inputL;
        float dryL = inputL;
        float dryR = inputR;

        // Envelope follower (use mono mix for envelope)
        float monoIn = (inputL + inputR) * 0.5f;
        float absIn = std::abs(monoIn);
        if (absIn > envFollower[0])
            envFollower[0] = 0.01f * absIn + 0.99f * envFollower[0];
        else
            envFollower[0] *= 0.9995f;

        float envelope = envFollower[0];
        float harmonicSumL = 0.0f;
        float harmonicSumR = 0.0f;

        // Generate each harmonic
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            float harmonicFreq = currentF0 * harmonicMults[band] * shiftRatio;
            float phaseInc = twoPi * harmonicFreq / static_cast<float>(sampleRate);

            // Generate waveform (use single phase per band for coherent stereo)
            float wave = generateWaveform(phases[band][0], currentWaveform);

            // Determine gain
            float gain = 0.0f;
            if (band == 0)
                gain = currentDarken * 1.5f;
            else
                gain = currentBrighten * (band == 1 ? 0.7f : 0.4f);

            // Modulate by envelope
            float harmonic = wave * envelope * gain;
            harmonic = applySaturation(harmonic, character);

            // Apply stereo width panning
            float pan = bandPans[band] * currentWidth;
            float panL = std::cos((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            float panR = std::sin((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

            harmonicSumL += harmonic * panL;
            harmonicSumR += harmonic * panR;

            // Advance phase
            phases[band][0] += phaseInc;
            if (phases[band][0] >= twoPi)
                phases[band][0] -= twoPi;
        }

        // DC block
        float wetL = dryL + harmonicSumL;
        float wetR = dryR + harmonicSumR;
        wetL = processDCBlock(wetL, 0);
        if (isStereo) wetR = processDCBlock(wetR, 1);

        // Mix
        buffer.setSample(0, i, (1.0f - currentMix) * dryL + currentMix * wetL);
        if (isStereo) buffer.setSample(1, i, (1.0f - currentMix) * dryR + currentMix * wetR);
    }
}

void HarmonicGenerator::processCopyMode(juce::AudioBuffer<float>& buffer, float f0,
                                         float brighten, float darken, Character character, float mix)
{
    smoothF0.setTarget(f0);
    smoothBrighten.setTarget(brighten);
    smoothDarken.setTarget(darken);
    smoothMix.setTarget(mix);
    smoothOvertoneShift.setTarget(targetOvertoneShift);
    smoothStereoWidth.setTarget(targetStereoWidth);

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    const bool isStereo = (numChannels == 2);

    // Pitch ratios to shift fundamental to each harmonic: 0.5, 2.0, 4.0
    const float harmonicMults[NUM_BANDS] = { 0.5f, 2.0f, 4.0f };
    // Pan positions for each band: -1 (left), 0 (center), +1 (right)
    const float bandPans[NUM_BANDS] = { -1.0f, 0.0f, 1.0f };

    for (int i = 0; i < numSamples; ++i)
    {
        float currentF0 = smoothF0.getNext(FREQ_SMOOTHING);
        float currentBrighten = smoothBrighten.getNext(PARAM_SMOOTHING);
        float currentDarken = smoothDarken.getNext(PARAM_SMOOTHING);
        float currentMix = smoothMix.getNext(PARAM_SMOOTHING);
        float currentShift = smoothOvertoneShift.getNext(PARAM_SMOOTHING);
        float currentWidth = smoothStereoWidth.getNext(PARAM_SMOOTHING);

        float shiftRatio = std::pow(2.0f, currentShift / 1200.0f);

        float inputL = buffer.getSample(0, i);
        float inputR = isStereo ? buffer.getSample(1, i) : inputL;
        float dryL = inputL;
        float dryR = inputR;

        // Clamp fundamental frequency to valid range
        float fundamentalFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45f), currentF0);

        // Update fundamental bandpass filter if frequency changed significantly
        if (std::abs(fundamentalFreq - lastFundamentalFreq) > 1.0f)
        {
            float Q = 2.0f;
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, fundamentalFreq, Q);
            fundamentalBandpass[0].coefficients = coeffs;
            if (isStereo) fundamentalBandpass[1].coefficients = coeffs;
            lastFundamentalFreq = fundamentalFreq;
        }

        // Extract fundamental content via bandpass
        float fundamentalL = fundamentalBandpass[0].processSample(inputL);
        float fundamentalR = isStereo ? fundamentalBandpass[1].processSample(inputR) : fundamentalL;

        float harmonicSumL = 0.0f;
        float harmonicSumR = 0.0f;

        // Copy fundamental to each harmonic band via pitch shifting
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            float pitchRatio = harmonicMults[band] * shiftRatio;

            // Pitch shift the extracted fundamental to this harmonic
            float shiftedL = copyShifters[band][0].process(fundamentalL, pitchRatio);
            float shiftedR = isStereo ? copyShifters[band][1].process(fundamentalR, pitchRatio) : shiftedL;

            // Apply gain based on band type
            float gain = 0.0f;
            if (band == 0)
                gain = currentDarken * 2.0f;
            else
                gain = currentBrighten * (band == 1 ? 0.8f : 0.5f);

            // Apply saturation
            float processedL = applySaturation(shiftedL * gain, character);
            float processedR = applySaturation(shiftedR * gain, character);

            // Apply stereo width panning
            float pan = bandPans[band] * currentWidth;
            float panL = std::cos((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            float panR = std::sin((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

            harmonicSumL += processedL * panL + processedR * (1.0f - panR);
            harmonicSumR += processedR * panR + processedL * (1.0f - panL);
        }

        // DC block
        float wetL = dryL + harmonicSumL;
        float wetR = dryR + harmonicSumR;
        wetL = processDCBlock(wetL, 0);
        if (isStereo) wetR = processDCBlock(wetR, 1);

        // Mix
        buffer.setSample(0, i, (1.0f - currentMix) * dryL + currentMix * wetL);
        if (isStereo) buffer.setSample(1, i, (1.0f - currentMix) * dryR + currentMix * wetR);
    }
}

void HarmonicGenerator::processMicrotonalMode(juce::AudioBuffer<float>& buffer, float f0,
                                               float brighten, float darken, Character character, float mix)
{
    smoothF0.setTarget(f0);
    smoothBrighten.setTarget(brighten);
    smoothDarken.setTarget(darken);
    smoothMix.setTarget(mix);
    smoothOvertoneShift.setTarget(targetOvertoneShift);
    smoothStereoWidth.setTarget(targetStereoWidth);

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    const bool isStereo = (numChannels == 2);

    // Get current scale
    const auto& scale = ScaleDatabase::getScale(currentScaleIndex);

    // Pan positions for each band: -1 (left), 0 (center), +1 (right)
    const float bandPans[NUM_BANDS] = { -1.0f, 0.0f, 1.0f };

    for (int i = 0; i < numSamples; ++i)
    {
        float currentF0 = smoothF0.getNext(FREQ_SMOOTHING);
        float currentBrighten = smoothBrighten.getNext(PARAM_SMOOTHING);
        float currentDarken = smoothDarken.getNext(PARAM_SMOOTHING);
        float currentMix = smoothMix.getNext(PARAM_SMOOTHING);
        float currentShift = smoothOvertoneShift.getNext(PARAM_SMOOTHING);
        float currentWidth = smoothStereoWidth.getNext(PARAM_SMOOTHING);

        float shiftRatio = std::pow(2.0f, currentShift / 1200.0f);

        float inputL = buffer.getSample(0, i);
        float inputR = isStereo ? buffer.getSample(1, i) : inputL;
        float dryL = inputL;
        float dryR = inputR;

        // Find the root frequency snapped to the selected key
        float rootFreq = ScaleDatabase::snapToNearestRoot(currentF0, currentKey);

        // Clamp fundamental frequency
        float fundamentalFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45f), currentF0);

        // Update fundamental bandpass filter if frequency changed significantly
        if (std::abs(fundamentalFreq - lastFundamentalFreq) > 1.0f)
        {
            float Q = 2.0f;
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, fundamentalFreq, Q);
            fundamentalBandpass[0].coefficients = coeffs;
            if (isStereo) fundamentalBandpass[1].coefficients = coeffs;
            lastFundamentalFreq = fundamentalFreq;
        }

        // Extract fundamental content via bandpass
        float fundamentalL = fundamentalBandpass[0].processSample(inputL);
        float fundamentalR = isStereo ? fundamentalBandpass[1].processSample(inputR) : fundamentalL;

        float harmonicSumL = 0.0f;
        float harmonicSumR = 0.0f;

        // Generate harmonics at scale degrees
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            // Get the target frequency for this scale degree
            int degree = harmonicDegrees[band];
            int octaveOffset = (degree >= static_cast<int>(scale.intervals.size()) - 1) ? 1 : 0;
            int degreeInScale = degree % (static_cast<int>(scale.intervals.size()) - 1);

            float targetFreq = ScaleDatabase::getFrequencyForDegree(rootFreq, scale, degreeInScale, octaveOffset);
            targetFreq *= shiftRatio;  // Apply overtone shift
            targetFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45f), targetFreq);

            // Calculate pitch ratio from fundamental to target frequency
            float pitchRatio = targetFreq / fundamentalFreq;

            // Pitch shift the extracted fundamental to this scale degree
            float shiftedL = microtonalShifters[band][0].process(fundamentalL, pitchRatio);
            float shiftedR = isStereo ? microtonalShifters[band][1].process(fundamentalR, pitchRatio) : shiftedL;

            // Apply gain based on band position
            // Band 0 uses darken (lower harmonics), bands 1-2 use brighten (upper harmonics)
            float gain = 0.0f;
            if (band == 0)
                gain = currentDarken * 1.5f;
            else
                gain = currentBrighten * (band == 1 ? 0.8f : 0.5f);

            // Apply saturation
            float processedL = applySaturation(shiftedL * gain, character);
            float processedR = applySaturation(shiftedR * gain, character);

            // Apply stereo width panning
            float pan = bandPans[band] * currentWidth;
            float panL = std::cos((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            float panR = std::sin((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

            harmonicSumL += processedL * panL + processedR * (1.0f - panR);
            harmonicSumR += processedR * panR + processedL * (1.0f - panL);
        }

        // DC block
        float wetL = dryL + harmonicSumL;
        float wetR = dryR + harmonicSumR;
        wetL = processDCBlock(wetL, 0);
        if (isStereo) wetR = processDCBlock(wetR, 1);

        // Mix
        buffer.setSample(0, i, (1.0f - currentMix) * dryL + currentMix * wetL);
        if (isStereo) buffer.setSample(1, i, (1.0f - currentMix) * dryR + currentMix * wetR);
    }
}

float HarmonicGenerator::generateWaveform(float phase, Waveform waveform)
{
    const float twoPi = juce::MathConstants<float>::twoPi;
    float normalized = phase / twoPi;  // 0 to 1

    switch (waveform)
    {
        case Waveform::Sine:
            return std::sin(phase);

        case Waveform::Square:
            // Band-limited approximation using first few harmonics
            return std::tanh(4.0f * std::sin(phase));

        case Waveform::Saw:
            // Naive saw with soft edges
            return std::tanh(3.0f * (2.0f * normalized - 1.0f));

        case Waveform::Triangle:
            // Triangle from phase
            if (normalized < 0.25f)
                return 4.0f * normalized;
            else if (normalized < 0.75f)
                return 2.0f - 4.0f * normalized;
            else
                return 4.0f * normalized - 4.0f;
    }

    return 0.0f;
}

float HarmonicGenerator::applySaturation(float input, Character character)
{
    switch (character)
    {
        case Character::Clean:
            return std::tanh(input * 1.5f) * 0.67f;

        case Character::Gritty:
        {
            float x = input * 2.0f;
            if (x > 0.0f)
                x = 1.0f - std::exp(-x);
            else
                x = -1.0f + std::exp(x);
            x = x - 0.15f * x * x * x;
            return std::tanh(x) * 0.8f;
        }
    }
    return input;
}

float HarmonicGenerator::processDCBlock(float input, int channel)
{
    const float R = 0.995f;
    float output = input - dcBlockIn[channel] + R * dcBlockOut[channel];
    dcBlockIn[channel] = input;
    dcBlockOut[channel] = output;
    return output;
}
