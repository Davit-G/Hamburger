#pragma once

#include "juce_core/juce_core.h"

#include <cmath>
#include <optional>

// enum of units, with strings attached to them
enum class ParamUnits {
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

struct ParamUnitFormat {
    const char *suffix;
    int decimalPlaces;
};

// keep in the same order as the enum
// suffixes are utf-8
inline constexpr ParamUnitFormat paramUnitFormats[] = {
    {"", 2},    // none
    {" Hz", 1}, // hz
    {" ms", 1}, // ms
    {" dB", 1}, // db
    {" %", 2},  // percent
    {"x", 0},   // x
    {"", 0},    // category
    {":1", 1},  // compressionRatio
    {"x", 0},   // oversample
    {"\u00b0", 1}, // degrees
};

static_assert(std::size(paramUnitFormats) == static_cast<size_t>(ParamUnits::numUnits),
              "every ParamUnits entry needs a format in paramUnitFormats");

inline const ParamUnitFormat &getParamUnitFormat(ParamUnits unit) noexcept {
    jassert(unit < ParamUnits::numUnits);
    return paramUnitFormats[static_cast<size_t>(unit)];
}

inline juce::String getParamUnitSuffix(ParamUnits unit) noexcept {
    return juce::String(juce::CharPointer_UTF8(getParamUnitFormat(unit).suffix));
}


inline float paramValueToDisplay(float value, ParamUnits unit) noexcept {
    // oversampling is stored as an exponent, but shown as the factor it works out to
    if (unit == ParamUnits::oversample)
        return std::pow(2.0f, std::round(value));

    return value;
}

inline float displayToParamValue(float display, ParamUnits unit) noexcept {
    if (unit == ParamUnits::oversample)
        return std::round(std::log2(juce::jmax(display, 1.0f)));

    return display;
}

inline juce::String createParamString(float value, ParamUnits unit) noexcept {
    const auto &format = getParamUnitFormat(unit);

    return juce::String(paramValueToDisplay(value, unit), format.decimalPlaces, false) + getParamUnitSuffix(unit);
}

inline juce::String formatDecibels(float db) {
    return juce::String(db > 0.0f ? "+" : "") + juce::String(db, 1) + " dB";
}

inline juce::String formatPercent(float normalised) {
    return juce::String(juce::roundToInt(normalised * 100.0f)) + "%";
}

// removes unit from end of string that contains unit in it
inline juce::String stripParamUnitSuffix(const juce::String &text, ParamUnits unit) noexcept {
    auto suffix = getParamUnitSuffix(unit);

    if (suffix.isNotEmpty() && text.endsWithIgnoreCase(suffix))
        return text.dropLastCharacters(suffix.length()).trim();

    return text.trim();
}

inline std::optional<float> parseParamString(const juce::String &text, ParamUnits unit) noexcept {
    auto stripped = stripParamUnitSuffix(text, unit);

    // getFloatValue() returns 0 for anything unparseable, so a digit is what separates
    // "0" the value from "abc" the typo
    if (!stripped.containsAnyOf("0123456789"))
        return std::nullopt;

    return displayToParamValue(stripped.getFloatValue(), unit);
}
