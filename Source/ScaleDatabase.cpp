#include "ScaleDatabase.h"
#include <algorithm>

// Static member initialization
std::vector<ScaleDatabase::Scale> ScaleDatabase::scales;
bool ScaleDatabase::initialized = false;

const char* const ScaleDatabase::KEY_NAMES[NUM_KEYS] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

void ScaleDatabase::initializeScales()
{
    if (initialized) return;

    scales.clear();

    // ========== WESTERN MODES (12-TET) ==========

    scales.push_back({
        "Ionian (Major)",
        Category::Western,
        {0, 200, 400, 500, 700, 900, 1100, 1200},
        "Standard major scale"
    });

    scales.push_back({
        "Dorian",
        Category::Western,
        {0, 200, 300, 500, 700, 900, 1000, 1200},
        "Minor scale with raised 6th"
    });

    scales.push_back({
        "Phrygian",
        Category::Western,
        {0, 100, 300, 500, 700, 800, 1000, 1200},
        "Minor scale with flat 2nd"
    });

    scales.push_back({
        "Lydian",
        Category::Western,
        {0, 200, 400, 600, 700, 900, 1100, 1200},
        "Major scale with raised 4th"
    });

    scales.push_back({
        "Mixolydian",
        Category::Western,
        {0, 200, 400, 500, 700, 900, 1000, 1200},
        "Major scale with flat 7th"
    });

    scales.push_back({
        "Aeolian (Minor)",
        Category::Western,
        {0, 200, 300, 500, 700, 800, 1000, 1200},
        "Natural minor scale"
    });

    scales.push_back({
        "Locrian",
        Category::Western,
        {0, 100, 300, 500, 600, 800, 1000, 1200},
        "Diminished scale with flat 2nd and 5th"
    });

    scales.push_back({
        "Harmonic Minor",
        Category::Western,
        {0, 200, 300, 500, 700, 800, 1100, 1200},
        "Minor with raised 7th"
    });

    scales.push_back({
        "Melodic Minor",
        Category::Western,
        {0, 200, 300, 500, 700, 900, 1100, 1200},
        "Minor with raised 6th and 7th"
    });

    scales.push_back({
        "Whole Tone",
        Category::Western,
        {0, 200, 400, 600, 800, 1000, 1200},
        "Symmetric scale of whole steps"
    });

    scales.push_back({
        "Chromatic",
        Category::Western,
        {0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200},
        "All 12 semitones"
    });

    scales.push_back({
        "Blues",
        Category::Western,
        {0, 300, 500, 600, 700, 1000, 1200},
        "Minor pentatonic with blue note"
    });

    scales.push_back({
        "Pentatonic Major",
        Category::Western,
        {0, 200, 400, 700, 900, 1200},
        "5-note major scale"
    });

    scales.push_back({
        "Pentatonic Minor",
        Category::Western,
        {0, 300, 500, 700, 1000, 1200},
        "5-note minor scale"
    });

    // ========== MIDDLE EASTERN MAQAMAT (with quarter tones) ==========
    // Quarter tone = 50 cents, three-quarter tone = 150 cents

    scales.push_back({
        "Rast",
        Category::MiddleEastern,
        {0, 200, 350, 500, 700, 900, 1050, 1200},
        "Fundamental Arabic maqam with neutral 3rd and 7th"
    });

    scales.push_back({
        "Bayati",
        Category::MiddleEastern,
        {0, 150, 300, 500, 700, 850, 1000, 1200},
        "Common maqam with quarter-flat 2nd"
    });

    scales.push_back({
        "Hijaz",
        Category::MiddleEastern,
        {0, 100, 400, 500, 700, 800, 1100, 1200},
        "Distinctive augmented 2nd interval"
    });

    scales.push_back({
        "Hijaz Kar",
        Category::MiddleEastern,
        {0, 100, 400, 500, 700, 800, 1100, 1200},
        "Hijaz with emphasis on upper tetrachord"
    });

    scales.push_back({
        "Saba",
        Category::MiddleEastern,
        {0, 150, 300, 400, 700, 800, 1000, 1200},
        "Expressive maqam with flat 4th"
    });

    scales.push_back({
        "Nahawand",
        Category::MiddleEastern,
        {0, 200, 300, 500, 700, 800, 1100, 1200},
        "Arabic equivalent of harmonic minor"
    });

    scales.push_back({
        "Ajam",
        Category::MiddleEastern,
        {0, 200, 400, 500, 700, 900, 1100, 1200},
        "Arabic major scale"
    });

    scales.push_back({
        "Kurd",
        Category::MiddleEastern,
        {0, 100, 300, 500, 700, 800, 1000, 1200},
        "Similar to Phrygian mode"
    });

    scales.push_back({
        "Sikah",
        Category::MiddleEastern,
        {0, 150, 350, 650, 850, 1000, 1150, 1200},
        "Starts on E half-flat, ethereal quality"
    });

    scales.push_back({
        "Huzam",
        Category::MiddleEastern,
        {0, 150, 350, 500, 700, 850, 1100, 1200},
        "Variant of Sikah family"
    });

    scales.push_back({
        "Nawa Athar",
        Category::MiddleEastern,
        {0, 200, 300, 600, 700, 800, 1100, 1200},
        "Double harmonic with augmented 4th"
    });

    scales.push_back({
        "Nikriz",
        Category::MiddleEastern,
        {0, 200, 300, 600, 700, 900, 1000, 1200},
        "Augmented 4th with minor quality"
    });

    scales.push_back({
        "Athar Kurd",
        Category::MiddleEastern,
        {0, 100, 300, 600, 700, 800, 1000, 1200},
        "Kurd with augmented 4th"
    });

    // ========== ASIAN SCALES ==========

    scales.push_back({
        "Hirajoshi",
        Category::Asian,
        {0, 200, 300, 700, 800, 1200},
        "Japanese pentatonic scale"
    });

    scales.push_back({
        "Iwato",
        Category::Asian,
        {0, 100, 500, 600, 1000, 1200},
        "Japanese scale with semitone starts"
    });

    scales.push_back({
        "In Sen",
        Category::Asian,
        {0, 100, 500, 700, 1000, 1200},
        "Japanese pentatonic"
    });

    scales.push_back({
        "Chinese",
        Category::Asian,
        {0, 400, 600, 700, 1100, 1200},
        "Traditional Chinese pentatonic"
    });

    scales.push_back({
        "Pelog",
        Category::Asian,
        {0, 100, 300, 700, 800, 1200},
        "Indonesian gamelan scale"
    });

    scales.push_back({
        "Slendro",
        Category::Asian,
        {0, 240, 480, 720, 960, 1200},
        "5-tone Indonesian scale (approximate)"
    });

    scales.push_back({
        "Raga Bhairav",
        Category::Asian,
        {0, 100, 400, 500, 700, 800, 1100, 1200},
        "North Indian morning raga"
    });

    scales.push_back({
        "Raga Todi",
        Category::Asian,
        {0, 100, 300, 600, 700, 800, 1100, 1200},
        "North Indian raga with aug 4th"
    });

    // ========== EXOTIC / SYNTHETIC SCALES ==========

    scales.push_back({
        "Double Harmonic",
        Category::Exotic,
        {0, 100, 400, 500, 700, 800, 1100, 1200},
        "Byzantine/Arabic scale"
    });

    scales.push_back({
        "Hungarian Minor",
        Category::Exotic,
        {0, 200, 300, 600, 700, 800, 1100, 1200},
        "Double augmented seconds"
    });

    scales.push_back({
        "Hungarian Major",
        Category::Exotic,
        {0, 300, 400, 600, 700, 900, 1000, 1200},
        "Augmented 2nd and raised 4th"
    });

    scales.push_back({
        "Neapolitan Minor",
        Category::Exotic,
        {0, 100, 300, 500, 700, 800, 1100, 1200},
        "Minor with flat 2nd"
    });

    scales.push_back({
        "Neapolitan Major",
        Category::Exotic,
        {0, 100, 300, 500, 700, 900, 1100, 1200},
        "Major with flat 2nd"
    });

    scales.push_back({
        "Persian",
        Category::Exotic,
        {0, 100, 400, 500, 600, 800, 1100, 1200},
        "Distinctive flat 5th"
    });

    scales.push_back({
        "Enigmatic",
        Category::Exotic,
        {0, 100, 400, 600, 800, 1000, 1100, 1200},
        "Invented by Verdi"
    });

    scales.push_back({
        "Prometheus",
        Category::Exotic,
        {0, 200, 400, 600, 900, 1000, 1200},
        "Scriabin's mystic scale"
    });

    scales.push_back({
        "Tritone",
        Category::Exotic,
        {0, 100, 400, 600, 700, 1000, 1200},
        "Two tritones scale"
    });

    scales.push_back({
        "Augmented",
        Category::Exotic,
        {0, 300, 400, 700, 800, 1100, 1200},
        "Symmetric augmented scale"
    });

    scales.push_back({
        "Diminished (HW)",
        Category::Exotic,
        {0, 100, 300, 400, 600, 700, 900, 1000, 1200},
        "Half-whole diminished"
    });

    scales.push_back({
        "Diminished (WH)",
        Category::Exotic,
        {0, 200, 300, 500, 600, 800, 900, 1100, 1200},
        "Whole-half diminished"
    });

    // Quarter-tone experimental scales
    scales.push_back({
        "Quarter Chromatic",
        Category::Exotic,
        {0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550,
         600, 650, 700, 750, 800, 850, 900, 950, 1000, 1050, 1100, 1150, 1200},
        "24-TET quarter tone scale"
    });

    scales.push_back({
        "Neutral",
        Category::Exotic,
        {0, 150, 350, 500, 700, 850, 1050, 1200},
        "Scale with neutral intervals"
    });

    initialized = true;
}

const std::vector<ScaleDatabase::Scale>& ScaleDatabase::getScales()
{
    initializeScales();
    return scales;
}

int ScaleDatabase::getNumScales()
{
    initializeScales();
    return static_cast<int>(scales.size());
}

const ScaleDatabase::Scale& ScaleDatabase::getScale(int index)
{
    initializeScales();
    if (index < 0 || index >= static_cast<int>(scales.size()))
        return scales[0];  // Return first scale as fallback
    return scales[index];
}

const char* ScaleDatabase::getScaleName(int index)
{
    initializeScales();
    if (index < 0 || index >= static_cast<int>(scales.size()))
        return "Unknown";
    return scales[index].name.c_str();
}

float ScaleDatabase::centsToRatio(float cents)
{
    return std::pow(2.0f, cents / 1200.0f);
}

float ScaleDatabase::ratioToCents(float ratio)
{
    return 1200.0f * std::log2(ratio);
}

float ScaleDatabase::getFrequencyForDegree(float rootFreq, const Scale& scale,
                                            int degreeIndex, int octaveOffset)
{
    if (scale.intervals.empty())
        return rootFreq;

    // Handle octave wrapping
    int numDegrees = static_cast<int>(scale.intervals.size()) - 1;  // -1 because last is octave
    if (numDegrees <= 0) numDegrees = 1;

    // Normalize degree index
    int octaves = 0;
    while (degreeIndex < 0)
    {
        degreeIndex += numDegrees;
        octaves--;
    }
    while (degreeIndex >= numDegrees)
    {
        degreeIndex -= numDegrees;
        octaves++;
    }

    octaves += octaveOffset;

    float cents = scale.intervals[degreeIndex];
    float octaveCents = static_cast<float>(octaves) * 1200.0f;

    return rootFreq * centsToRatio(cents + octaveCents);
}

int ScaleDatabase::getNearestDegree(float freq, float rootFreq, const Scale& scale)
{
    if (scale.intervals.empty() || rootFreq <= 0.0f || freq <= 0.0f)
        return 0;

    // Convert frequency ratio to cents
    float ratio = freq / rootFreq;

    // Normalize to within one octave
    while (ratio < 1.0f) ratio *= 2.0f;
    while (ratio >= 2.0f) ratio /= 2.0f;

    float cents = ratioToCents(ratio);

    // Find nearest scale degree
    int nearestDegree = 0;
    float minDistance = 1200.0f;

    int numDegrees = static_cast<int>(scale.intervals.size());
    for (int i = 0; i < numDegrees; ++i)
    {
        float distance = std::abs(cents - scale.intervals[i]);
        // Also check wrapping around octave
        float wrapDistance = std::abs(cents - (scale.intervals[i] - 1200.0f));
        float wrapDistance2 = std::abs(cents - (scale.intervals[i] + 1200.0f));

        distance = std::min({distance, wrapDistance, wrapDistance2});

        if (distance < minDistance)
        {
            minDistance = distance;
            nearestDegree = i;
        }
    }

    return nearestDegree;
}

float ScaleDatabase::getRootFrequency(int key, int octave)
{
    // A4 = 440 Hz, key 9 (A) in octave 4
    // Calculate semitones from A4
    int semitonesFromA4 = (octave - 4) * 12 + (key - 9);
    return 440.0f * std::pow(2.0f, static_cast<float>(semitonesFromA4) / 12.0f);
}

float ScaleDatabase::snapToNearestRoot(float detectedFreq, int key)
{
    if (detectedFreq <= 0.0f)
        return detectedFreq;

    // Find the nearest octave of the selected key
    float keyFreqOctave4 = getRootFrequency(key, 4);

    // Find the octave that puts us closest to the detected frequency
    float bestRoot = keyFreqOctave4;
    float bestDistance = std::abs(detectedFreq - bestRoot);

    for (int oct = 0; oct <= 8; ++oct)
    {
        float rootFreq = getRootFrequency(key, oct);
        float distance = std::abs(detectedFreq - rootFreq);

        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestRoot = rootFreq;
        }
    }

    return bestRoot;
}
