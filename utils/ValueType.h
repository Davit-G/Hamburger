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
};

static_assert(std::size(valueFormats) == static_cast<size_t>(ValueType::numUnits),
              "every ValueType entry needs a format in paramUnitFormats");

inline juce::String getParamUnitSuffix(ValueType unit) noexcept {
    return juce::String(juce::CharPointer_UTF8(valueFormats[static_cast<int>(unit)].suffix));
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

    return juce::String(value, format.decimalPlaces, false) + getParamUnitSuffix(unit);
}

inline std::optional<float> valueStringToFloat(const juce::String &text, ValueType unit) noexcept {
    auto suffix = getParamUnitSuffix(unit);
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
