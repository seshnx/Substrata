#include "PitchTracker.h"

PitchTracker::PitchTracker()
{
    buffer.resize(4096, 0.0f);
}

void PitchTracker::prepare(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlock;
    buffer.resize(static_cast<size_t>(sampleRate * 0.1), 0.0f); // 100ms buffer
    bufferWritePos = 0;
    currentF0.store(440.0f, std::memory_order_relaxed);
    smoothedF0 = 440.0f;
    isValid.store(false, std::memory_order_relaxed);
}

float PitchTracker::processBlock(const float* input, int numSamples)
{
    // Safety checks
    if (input == nullptr || numSamples <= 0 || buffer.empty())
        return currentF0;
    
    // Fill circular buffer
    int bufferSize = static_cast<int>(buffer.size());
    for (int i = 0; i < numSamples; ++i)
    {
        buffer[bufferWritePos] = input[i];
        bufferWritePos = (bufferWritePos + 1) % bufferSize;
    }
    
    // Process pitch detection every block
    float detectedF0 = detectPitch(buffer.data(), static_cast<int>(buffer.size()));
    
    // Smooth the detected frequency
    if (detectedF0 > minFrequency && detectedF0 < maxFrequency)
    {
        smoothedF0 = smoothingFactor * smoothedF0 + (1.0f - smoothingFactor) * detectedF0;
        currentF0.store(smoothedF0, std::memory_order_relaxed);
        isValid.store(true, std::memory_order_relaxed);
    }
    else
    {
        isValid.store(false, std::memory_order_relaxed);
    }

    return currentF0.load(std::memory_order_relaxed);
}

float PitchTracker::detectPitch(const float* input, int length)
{
    float previousF0 = currentF0.load(std::memory_order_relaxed);

    // Safety check
    if (input == nullptr || length <= 0 || sampleRate <= 0.0)
        return previousF0;

    // Calculate signal energy (r[0] - autocorrelation at lag 0)
    float energy = 0.0f;
    for (int i = 0; i < length; ++i)
    {
        energy += input[i] * input[i];
    }

    // If signal is too quiet, skip detection
    if (energy < 1e-6f)
        return previousF0;

    // Calculate autocorrelation to find period
    int minLag = static_cast<int>(sampleRate / maxFrequency);
    int maxLag = static_cast<int>(sampleRate / minFrequency);

    // Safety check
    if (minLag <= 0 || maxLag <= minLag || maxLag >= length / 2)
        return previousF0;

    float maxCorrelation = 0.0f;
    int bestLag = minLag;

    for (int lag = minLag; lag < maxLag && lag < length / 2; ++lag)
    {
        float corr = autocorrelation(input, length, lag);

        // Normalize by energy to get value between -1 and 1
        float normalizedCorr = corr / energy;

        if (normalizedCorr > maxCorrelation)
        {
            maxCorrelation = normalizedCorr;
            bestLag = lag;
        }
    }

    // Accept if normalized correlation is strong enough (threshold now 0.0 to 1.0 range)
    if (maxCorrelation > 0.5f && bestLag > 0 && bestLag < length)
    {
        float detectedF0 = static_cast<float>(sampleRate) / static_cast<float>(bestLag);
        // Validate result
        if (detectedF0 >= minFrequency && detectedF0 <= maxFrequency)
            return detectedF0;
    }

    return previousF0;
}

float PitchTracker::autocorrelation(const float* input, int length, int lag)
{
    // Safety checks
    if (input == nullptr || length <= 0 || lag <= 0 || lag >= length)
        return 0.0f;

    float sum = 0.0f;
    int maxIndex = length - lag;

    for (int i = 0; i < maxIndex; ++i)
    {
        sum += input[i] * input[i + lag];
    }

    return sum;
}

