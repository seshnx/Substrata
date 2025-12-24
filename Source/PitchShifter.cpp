#include "PitchShifter.h"

PitchShifter::PitchShifter()
{
    // Pre-allocate buffers
    for (auto& buf : delayBuffer)
        buf.resize(16384, 0.0f);

    // Pre-compute Hann window
    windowTable.resize(GRAIN_SIZE);
    for (int i = 0; i < GRAIN_SIZE; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(GRAIN_SIZE - 1);
        windowTable[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * t));
    }
}

void PitchShifter::prepare(double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
    bufferSize = static_cast<int>(sampleRate * 0.4);  // 400ms buffer
    bufferSize = std::max(bufferSize, GRAIN_SIZE * 4);

    for (auto& buf : delayBuffer)
    {
        buf.resize(static_cast<size_t>(bufferSize), 0.0f);
        std::fill(buf.begin(), buf.end(), 0.0f);
    }

    reset();
}

void PitchShifter::reset()
{
    for (auto& buf : delayBuffer)
        std::fill(buf.begin(), buf.end(), 0.0f);

    for (auto& channelGrains : grains)
        for (auto& grain : channelGrains)
        {
            grain.readPos = 0.0f;
            grain.grainPos = 0;
            grain.active = false;
        }

    for (auto& wp : writePos)
        wp = 0;

    for (auto& s : samplesSinceLastGrain)
        s = 0;

    currentPitchRatio = 1.0f;
    targetPitchRatio = 1.0f;
}

void PitchShifter::processBlock(juce::AudioBuffer<float>& buffer, float pitchShiftCents)
{
    // Convert cents to ratio
    targetPitchRatio = std::pow(2.0f, pitchShiftCents / 1200.0f);

    int numChannels = std::min(buffer.getNumChannels(), 2);
    for (int channel = 0; channel < numChannels; ++channel)
    {
        processChannel(buffer.getWritePointer(channel), buffer.getNumSamples(), channel);
    }
}

void PitchShifter::processChannel(float* channelData, int numSamples, int channel)
{
    auto& buffer = delayBuffer[channel];
    auto& wp = writePos[channel];
    auto& channelGrains = grains[channel];
    auto& sampleCount = samplesSinceLastGrain[channel];

    for (int i = 0; i < numSamples; ++i)
    {
        // Smooth pitch ratio
        currentPitchRatio = SMOOTHING_COEFF * currentPitchRatio + (1.0f - SMOOTHING_COEFF) * targetPitchRatio;

        // Write input to delay buffer
        buffer[wp] = channelData[i];

        // Check if we need to start a new grain
        sampleCount++;
        if (sampleCount >= HOP_SIZE)
        {
            sampleCount = 0;

            // Find an inactive grain slot
            for (auto& grain : channelGrains)
            {
                if (!grain.active)
                {
                    grain.active = true;
                    grain.grainPos = 0;
                    // Start reading from a position behind the write head
                    grain.readPos = static_cast<float>(wp - GRAIN_SIZE * 2);
                    if (grain.readPos < 0.0f)
                        grain.readPos += static_cast<float>(bufferSize);
                    break;
                }
            }
        }

        // Sum output from all active grains
        float output = 0.0f;

        for (auto& grain : channelGrains)
        {
            if (!grain.active)
                continue;

            // Get window value
            float window = windowTable[grain.grainPos];

            // Read from buffer with cubic interpolation
            int readIdx = static_cast<int>(grain.readPos);
            float frac = grain.readPos - static_cast<float>(readIdx);

            // Get 4 samples for cubic interpolation
            int idx0 = (readIdx - 1 + bufferSize) % bufferSize;
            int idx1 = readIdx % bufferSize;
            int idx2 = (readIdx + 1) % bufferSize;
            int idx3 = (readIdx + 2) % bufferSize;

            float sample = cubicInterpolate(buffer[idx0], buffer[idx1], buffer[idx2], buffer[idx3], frac);

            output += sample * window;

            // Advance grain
            grain.grainPos++;
            grain.readPos += currentPitchRatio;

            // Wrap read position
            if (grain.readPos >= static_cast<float>(bufferSize))
                grain.readPos -= static_cast<float>(bufferSize);
            else if (grain.readPos < 0.0f)
                grain.readPos += static_cast<float>(bufferSize);

            // Deactivate grain when complete
            if (grain.grainPos >= GRAIN_SIZE)
            {
                grain.active = false;
            }
        }

        // Output the mixed grains
        channelData[i] = output;

        // Advance write position
        wp = (wp + 1) % bufferSize;
    }
}

float PitchShifter::cubicInterpolate(float y0, float y1, float y2, float y3, float t)
{
    // Catmull-Rom spline interpolation
    float t2 = t * t;
    float t3 = t2 * t;

    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;

    return a0 * t3 + a1 * t2 + a2 * t + a3;
}

float PitchShifter::getWindowValue(int pos, int size)
{
    if (pos < 0 || pos >= size)
        return 0.0f;

    // Hann window
    float t = static_cast<float>(pos) / static_cast<float>(size - 1);
    return 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * t));
}
