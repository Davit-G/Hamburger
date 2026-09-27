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
    static constexpr int maxChannels = 2;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        jassert (spec.numChannels <= maxChannels);
        sampleRate = spec.sampleRate;
        reset();
        update();
    }

    // 12, 24 or 48 dB/oct. a change starts the filters from silence, their old state belongs to a different filter
    void setSlope (int dbPerOctave)
    {
        const auto wanted = dbPerOctave == 12 ? 1 : dbPerOctave == 48 ? 4 : 2;

        if (wanted == sections)
            return;

        sections = wanted;
        reset();
        update();
    }

    void setCutoffFrequency (float hz)
    {
        if (juce::exactlyEqual (hz, cutoff))
            return;

        cutoff = hz;
        update();
    }

    /*  A sample at a time rather than a section over a block at a time. Each section's next sample waits on its last, so a
        block of one section can only go as fast as that chain, where a sample through every section of every split, both
        channels in turn, gives the CPU many independent filters to work on at once. */
    void processSample (int channel, float x, float& low, float& high)
    {
        low = high = x;

        for (size_t i = 0; i < (size_t) sections; ++i)
        {
            low = lows[i].tick (channel, low).lp;
            high = highs[i].tick (channel, high).hp;
        }

        if (sections == 1)
            high = -high;
    }

    /*  What the split does to a signal it isn't splitting, to keep the bands that skip it in phase with the ones that don't.
        The two sides sum to the Butterworth's own allpass, so that's run instead of the split: first order at 12 dB/oct, and
        at 24 and 48 one second order allpass for each of the Butterworth's sections, the input less twice its band pass over Q. */
    float allpass (int channel, float x)
    {
        if (sections == 1)
        {
            auto& state = firstOrderState[(size_t) channel];
            const auto v = firstOrderGain * (x - state);
            const auto lp = v + state;
            state = lp + v;
            return 2.0f * lp - x;
        }

        for (size_t i = 0; i < (size_t) sections / 2; ++i)
            x -= 2.0f * allpasses[i].r2 * allpasses[i].tick (channel, x).bp;

        return x;
    }

private:
    // a second order TPT state variable filter, worked as juce::dsp::StateVariableTPTFilter does without choosing an output every sample
    struct Section
    {
        struct Outputs { float lp, bp, hp; };

        float g = 0.0f, r2 = 1.0f, h = 1.0f;
        std::array<float, maxChannels> s1 {}, s2 {};

        void set (float newG, float q)
        {
            g = newG;
            r2 = 1.0f / q;
            h = 1.0f / (1.0f + r2 * g + g * g);
        }

        Outputs tick (int channel, float x)
        {
            auto& a = s1[(size_t) channel];
            auto& b = s2[(size_t) channel];

            const auto hp = h * (x - a * (g + r2) - b);
            const auto bp = hp * g + a;
            a = hp * g + bp;
            const auto lp = bp * g + b;
            b = bp * g + lp;

            return { lp, bp, hp };
        }
    };

    // each Butterworth's section Qs twice over: first order as one Q 0.5 section, second order 0.707, fourth 0.541 and 1.307.
    // the Butterworth once over, for the allpass, is the first half of them
    void update()
    {
        static constexpr float qs[4][4] { { 0.5f }, { 0.7071f, 0.7071f }, {}, { 0.5412f, 1.3066f, 0.5412f, 1.3066f } };

        const auto g = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) cutoff, sampleRate * 0.49) / sampleRate);

        for (size_t i = 0; i < (size_t) sections; ++i)
        {
            lows[i].set (g, qs[sections - 1][i]);
            highs[i].set (g, qs[sections - 1][i]);

            if (i < (size_t) sections / 2)
                allpasses[i].set (g, qs[sections - 1][i]);
        }

        firstOrderGain = g / (1.0f + g);
    }

    void reset()
    {
        for (auto* chain : { &lows, &highs })
            for (auto& section : *chain)
                section.s1 = section.s2 = {};

        for (auto& section : allpasses)
            section.s1 = section.s2 = {};

        firstOrderState = {};
    }

    std::array<Section, 4> lows, highs;
    std::array<Section, 2> allpasses;
    std::array<float, maxChannels> firstOrderState {};
    float firstOrderGain = 0.0f;

    double sampleRate = 44100.0;
    float cutoff = 1000.0f;
    int sections = 2;
};

// the crossovers an effect splits its audio at and sums back through, for a dry mixed with it to match
struct Crossovers
{
    int slope = 24;
    int count = 0;
    std::array<float, 3> hz {};
};

// puts a dry through the allpasses that crossovers leave on whatever they split and sum back, so the two mix in phase
class DryPhase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        for (auto& stage : stages)
            stage.prepare (spec);

        highestHz = (float) spec.sampleRate * 0.45f;
        scratch.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize, false, false, true);
    }

    // afresh every block, then every effect's crossovers
    void clear() { used = 0; }

    void add (const Crossovers& crossovers)
    {
        for (int i = 0; i < crossovers.count && used < stages.size(); ++i, ++used)
        {
            stages[used].setSlope (crossovers.slope);
            stages[used].setCutoffFrequency (juce::jmin (crossovers.hz[(size_t) i], highestHz));
        }
    }

    // the block as the mixer's dry, leaving the block itself alone
    void pushDry (juce::dsp::DryWetMixer<float>& mixer, const juce::dsp::AudioBlock<float>& block)
    {
        if (used == 0)
            return mixer.pushDrySamples (block);

        auto dry = juce::dsp::AudioBlock<float> (scratch).getSubBlock (0, block.getNumSamples()).getSubsetChannelBlock (0, block.getNumChannels());
        dry.copyFrom (block);

        for (size_t n = 0; n < dry.getNumSamples(); ++n)
            for (size_t ch = 0; ch < dry.getNumChannels(); ++ch)
            {
                auto& sample = dry.getChannelPointer (ch)[n];

                for (size_t stage = 0; stage < used; ++stage)
                    sample = stages[stage].allpass ((int) ch, sample);
            }

        mixer.pushDrySamples (dry);
    }

private:
    std::array<Crossover, 5> stages;
    size_t used = 0;
    float highestHz = 20000.0f;
    juce::AudioBuffer<float> scratch;
};
