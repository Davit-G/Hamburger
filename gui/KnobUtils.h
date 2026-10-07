#pragma once

#include "juce_core/juce_core.h"

#include <cmath>
#include <optional>

// enum of units, with strings attached to them
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

    numUnits
};

struct ValueFormat {
    const char *suffix;
    int decimalPlaces;
};

// keep in the same order as the enum
inline constexpr ValueFormat valueFormats[] = {
    {"", 2},    // none
    {" Hz", 1}, // hz
    {" ms", 1}, // ms
    {" dB", 1}, // db
    {" %", 2},  // percent
    {"x", 0},   // x
    {"", 0},    // category
    {":1", 1},  // compressionRatio
    {"x", 0},   // oversample
};

static_assert(std::size(valueFormats) == static_cast<size_t>(ValueType::numUnits),
              "every ParamUnits entry needs a format in paramUnitFormats");

inline const ValueFormat &getParamUnitFormat(ValueType unit) noexcept {
    jassert(unit < ValueType::numUnits);
    return valueFormats[static_cast<size_t>(unit)];
}

inline juce::String getParamUnitSuffix(ValueType unit) noexcept {
    return getParamUnitFormat(unit).suffix;
}


inline float paramValueToDisplay(float value, ValueType unit) noexcept {
    // oversampling is stored as an exponent, but shown as the factor it works out to
    if (unit == ValueType::oversample)
        return std::pow(2.0f, std::round(value));

    return value;
}

inline float displayToParamValue(float display, ValueType unit) noexcept {
    if (unit == ValueType::oversample)
        return std::round(std::log2(juce::jmax(display, 1.0f)));

    return display;
}

inline juce::String floatToValueString(float value, ValueType unit) noexcept {
    const auto &format = getParamUnitFormat(unit);

    return juce::String(paramValueToDisplay(value, unit), format.decimalPlaces, false) + format.suffix;
}

// removes unit from end of string that contains unit in it
inline juce::String stripParamUnitSuffix(const juce::String &text, ValueType unit) noexcept {
    auto suffix = getParamUnitSuffix(unit);

    if (suffix.isNotEmpty() && text.endsWithIgnoreCase(suffix))
        return text.dropLastCharacters(suffix.length()).trim();

    return text.trim();
}

inline std::optional<float> valueStringToFloat(const juce::String &text, ValueType unit) noexcept {
    auto stripped = stripParamUnitSuffix(text, unit);

    auto s = text.trimStart();

    if (s.startsWithChar('-') || s.startsWithChar('+'))
        s = s.substring(1);

    if (s.startsWithChar('.'))
        s = s.substring(1);

    bool startsWithNumber = s.isNotEmpty() && juce::CharacterFunctions::isDigit(s[0]);

    if (!startsWithNumber)
        return std::nullopt;

    return displayToParamValue(stripped.getFloatValue(), unit);
}
