#pragma once

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
    oversample
};

inline juce::String createParamString(float value, ParamUnits unit) noexcept {
    switch (unit) {
        case ParamUnits::hz:
            return juce::String(value, 1, false) + " Hz";
        case ParamUnits::ms:
            return juce::String(value, 1, false) + " ms";
        case ParamUnits::db:
            return juce::String(value, 1, false) + " dB";
        case ParamUnits::percent:
            return juce::String(value, 2, false) + " %";
        case ParamUnits::x:
            return juce::String(value, 0, false) + "x";
        case ParamUnits::category:
            return juce::String(value, 0, false);
        case ParamUnits::oversample:
            return juce::String(powf(2.0f, round(value)), 0, false) + "x";
        case ParamUnits::compressionRatio:
            return juce::String(value, 1, false) + ":1";
        default:
            return juce::String(value, 2, false);
    }
}