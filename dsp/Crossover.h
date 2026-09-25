#pragma once

#include "juce_dsp/juce_dsp.h"

/*  A Linkwitz-Riley split at 12, 24 or 48 dB/oct: a Butterworth low pass and high pass, each run twice over, built from
    second order sections. Run twice, the two sides sum back to an allpass, so a signal split and summed comes out flat.
    At 12 dB/oct the Butterworth is first order, twice over one Q 0.5 section, and its two sides are half a turn apart at
    the split, so the high side comes out inverted to keep the sum flat there too.

    juce::dsp::LinkwitzRileyFilter only does 24. */
class Crossover
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        for (auto& section : lows)
        {
            section.prepare (spec);
            section.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        }

        for (auto& section : highs)
        {
            section.prepare (spec);
            section.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        }

        applyQs();
    }

    // 12, 24 or 48 dB/oct. a change starts the filters from silence, their old state belongs to a different filter
    void setSlope (int dbPerOctave)
    {
        const auto wanted = dbPerOctave == 12 ? 1 : dbPerOctave == 48 ? 4 : 2;

        if (wanted == sections)
            return;

        sections = wanted;
        applyQs();

        for (auto* chain : { &lows, &highs })
            for (auto& section : *chain)
                section.reset();
    }

    // every section, not just the ones in use, so a slope change finds them all at the right frequency
    void setCutoffFrequency (float hz)
    {
        for (auto* chain : { &lows, &highs })
            for (auto& section : *chain)
                section.setCutoffFrequency (hz);
    }

    void processSample (int channel, float x, float& low, float& high)
    {
        low = high = x;

        for (int i = 0; i < sections; ++i)
        {
            low = lows[(size_t) i].processSample (channel, low);
            high = highs[(size_t) i].processSample (channel, high);
        }

        if (sections == 1)
            high = -high;
    }

    // what the split does to a signal it isn't splitting, to keep the bands that skip it in phase with the ones that don't
    float allpass (int channel, float x)
    {
        float low = 0.0f, high = 0.0f;
        processSample (channel, x, low, high);
        return low + high;
    }

private:
    // each Butterworth's section Qs twice over: first order as one Q 0.5 section, second order 0.707, fourth 0.541 and 1.307
    void applyQs()
    {
        static constexpr float qs[4][4] { { 0.5f }, { 0.7071f, 0.7071f }, {}, { 0.5412f, 1.3066f, 0.5412f, 1.3066f } };

        for (int i = 0; i < sections; ++i)
        {
            lows[(size_t) i].setResonance (qs[sections - 1][i]);
            highs[(size_t) i].setResonance (qs[sections - 1][i]);
        }
    }

    std::array<juce::dsp::StateVariableTPTFilter<float>, 4> lows, highs;
    int sections = 2;
};
