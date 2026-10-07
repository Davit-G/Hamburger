#pragma once

#include "juce_core/juce_core.h"

inline auto makeRange(float start, float end)
{
    return juce::NormalisableRange<float>(start, end, 0.001f);
}

// (min, max, interval, skew) — as used by the frequency-style knobs
inline auto makeSkewedRange(float start, float end, float skew)
{
    return juce::NormalisableRange<float>(start, end, 0.0f, skew);
}

// stepped range for choice / bool / int parameters
inline auto makeSteppedRange(float start, float end)
{
    return juce::NormalisableRange<float>(start, end, 1.0f);
}
