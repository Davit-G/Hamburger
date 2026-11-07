#pragma once

#include "juce_core/juce_core.h"

#include <cmath>
#include <optional>

enum class ValueType {
    none,
    hz,
    ms,
    db,
    percent,
    x,
    category,
    compressionRatio,
    oversample,
    degrees,
    beats, // an index into beatDivisions
    ratio, // an index into harmonicRatios
    delay, // ms on top of a single sample, the bottom read as that sample

    numUnits
};

struct ValueFormat {
    const char *suffix;
    int decimalPlaces;
};

// keep in the same order as the ValueType enum
// suffixes are utf-8
inline constexpr ValueFormat valueFormats[] = {
    {"", 2},
    {" Hz", 1},
    {" ms", 1}, 
    {" dB", 1},
    {" %", 2},
    {"x", 0},
    {"", 0},    // category
    {":1", 1},  // compressionRatio
    {"x", 0},   // oversample
    {"\u00b0", 1}, // degrees
    {"", 0},    // beats
    {"", 0},    // ratio
    {" ms", 1}, // delay
};

// tempo synced lengths, longest first so a knob over them sweeps steadily faster. in quarter notes
struct BeatDivision {
    const char *name;
    double beats;
};

inline constexpr BeatDivision beatDivisions[] = {
    {"4/1", 16.0}, {"2/1", 8.0}, {"1/1D", 6.0}, {"1/1", 4.0}, {"1/2D", 3.0}, {"1/1T", 8.0 / 3.0}, {"1/2", 2.0},
    {"1/4D", 1.5}, {"1/2T", 4.0 / 3.0}, {"1/4", 1.0}, {"1/8D", 0.75}, {"1/4T", 2.0 / 3.0}, {"1/8", 0.5},
    {"1/16D", 0.375}, {"1/8T", 1.0 / 3.0}, {"1/16", 0.25}, {"1/32D", 0.1875}, {"1/16T", 1.0 / 6.0}, {"1/32", 0.125},
    {"1/32T", 1.0 / 12.0},
};

inline constexpr int numBeatDivisions = (int) std::size(beatDivisions);

inline const BeatDivision &beatDivision(float index) {
    return beatDivisions[juce::jlimit(0, numBeatDivisions - 1, (int) std::round(index))];
}

// the harmonic mode's ratio knob steps through these, smallest first, also stored as an index into them
inline constexpr float harmonicRatios[] = { 0.25f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

inline constexpr int numHarmonicRatios = (int) std::size(harmonicRatios);

inline int harmonicRatioIndex(float index) {
    return juce::jlimit(0, numHarmonicRatios - 1, (int) std::round(index));
}

inline float harmonicRatio(float index) {
    return harmonicRatios[harmonicRatioIndex(index)];
}

inline juce::String harmonicRatioName(float index) {
    const auto ratio = harmonicRatios[harmonicRatioIndex(index)];

    // 1 shows as "1" not "1.00", 0.5 as "0.5" not "0.50"
    auto text = juce::String(ratio, 2, false);

    while (text.endsWith("0"))
        text = text.substring(0, text.length() - 1);

    return text.endsWith(".") ? text.substring(0, text.length() - 1) : text;
}

static_assert(std::size(valueFormats) == static_cast<size_t>(ValueType::numUnits),
              "every ValueType entry needs a format in paramUnitFormats");

inline juce::String getValueTypeuffix(ValueType unit) noexcept {
    return juce::String(juce::CharPointer_UTF8(valueFormats[static_cast<int>(unit)].suffix));
}




inline juce::String formatFrequency(float freq) {
    if (freq >= 1000.0f)
        return juce::String(freq / 1000.0f, 1) + " kHz";

    return juce::String(juce::roundToInt(freq)) + " Hz";
}

inline juce::String formatDecibels(float db) {
    return juce::String(db > 0.0f ? "+" : "") + juce::String(db, 1) + " dB";
}

inline juce::String formatPercent(float normalised) {
    return juce::String(juce::roundToInt(normalised * 100.0f)) + "%";
}



inline juce::String floatToValueString(float value, ValueType unit) noexcept {
    const auto &format = valueFormats[static_cast<int>(unit)];

    // oversampling is stored as an exponent, but shown as the factor it works out to
    if (unit == ValueType::oversample)
        value = std::pow(2.0f, std::round(value));

    if (unit == ValueType::beats)
        return beatDivision(value).name;

    if (unit == ValueType::ratio)
        return harmonicRatioName(value);

    if (unit == ValueType::delay && value <= 0.0f)
        return "1 sample";

    return juce::String(value, format.decimalPlaces, false) + getValueTypeuffix(unit);
}

inline std::optional<float> valueStringToFloat(const juce::String &text, ValueType unit) noexcept {
    if (unit == ValueType::beats) {
        for (int i = 0; i < numBeatDivisions; ++i)
            if (text.trim().equalsIgnoreCase(beatDivisions[i].name))
                return (float) i;

        return std::nullopt;
    }

    if (unit == ValueType::ratio) {
        const auto wanted = text.trim().getFloatValue();

        for (int i = 0; i < numHarmonicRatios; ++i)
            if (std::abs(harmonicRatios[i] - wanted) < 0.001f)
                return (float) i;

        return std::nullopt;
    }

    if (unit == ValueType::delay && text.containsIgnoreCase("sample"))
        return 0.0f;

    auto suffix = getValueTypeuffix(unit);
    auto stripped = juce::String();

    if (suffix.isNotEmpty() && text.endsWithIgnoreCase(suffix))
        stripped = text.dropLastCharacters(suffix.length()).trim();
    else {
        stripped = text.trim();
    }

    // getFloatValue() returns 0 for anything unparseable, so a digit is what separates
    // "0" the value from "abc" the typo
    if (!stripped.containsAnyOf("0123456789"))
        return std::nullopt;

    // reverses the oversampling 2^x if in oversample mode
    if (unit == ValueType::oversample) return std::round(std::log2(juce::jmax(stripped.getFloatValue(), 1.0f)));

    return stripped.getFloatValue();
}
