#pragma once

#include "SubGen.h"

/*  The undelayed chain of the sub gen's Hilbert pair on its own, stacked. It changes nothing in level, only smears the phase,
    and more so the lower it goes.

    Each section only looks two samples back, so two samples in a row don't depend on each other and go through together:
    both channels of both in one SIMD register, and one register per section holding what it made for the pair before. */
class HilbertStack : public MacroEffect
{
public:
    static constexpr int maxStacks = 25;
    static constexpr size_t sectionsPerStack = SubOctave::hilbertCoefs / 2;

    explicit HilbertStack (juce::AudioProcessorValueTreeState& state)
        : MacroEffect (state, SlotId { ModuleId::module2, 0 }),
          stacks (getParam (ParamIDs::hilbertStacks)) {}

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        const auto designed = SubOctave::design (spec.sampleRate);

        for (size_t i = 0; i < sectionsPerStack; ++i)
            coefs[i] = designed[i * 2];

        held = {};
        running = 0;
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        const auto count = (size_t) juce::jlimit (0, maxStacks, juce::roundToInt (stacks.getRaw (0))) * sectionsPerStack;

        // the sections just brought in start from silence rather than wherever they were left
        for (auto i = running; i < count; ++i)
            held[i] = Register (0.0f);

        running = count;

        const auto numChannels = juce::jmin (block.getNumChannels(), (size_t) 2);
        const auto numSamples = block.getNumSamples();
        size_t n = 0;

        // lanes 0 and 1 are the first sample's channels, 2 and 3 the second's
        for (; n + 1 < numSamples; n += 2)
        {
            Register x (0.0f);

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                x.set (ch, block.getSample ((int) ch, (int) n));
                x.set (ch + 2, block.getSample ((int) ch, (int) n + 1));
            }

            // (a - z^-2) / (1 - a z^-2), through what was made two samples back
            for (size_t i = 0; i < count; ++i)
            {
                const auto a = coefs[i % sectionsPerStack];
                const auto w = x + held[i] * a;

                x = w * a - held[i];
                held[i] = w;
            }

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                block.setSample ((int) ch, (int) n, x.get (ch));
                block.setSample ((int) ch, (int) n + 1, x.get (ch + 2));
            }
        }

        // an odd block's last sample on its own, leaving each pair as the last two samples again
        if (n < numSamples)
        {
            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto x = block.getSample ((int) ch, (int) n);

                for (size_t i = 0; i < count; ++i)
                {
                    const auto a = coefs[i % sectionsPerStack];
                    const auto twoBack = held[i].get (ch);
                    const auto w = x + twoBack * a;

                    x = w * a - twoBack;
                    held[i].set (ch, held[i].get (ch + 2));
                    held[i].set (ch + 2, w);
                }

                block.setSample ((int) ch, (int) n, x);
            }
        }
    }

private:
    using Register = juce::dsp::SIMDRegister<float>;
    static_assert (Register::size() >= 4);

    SmoothParam stacks;
    std::array<float, sectionsPerStack> coefs {};
    std::array<Register, maxStacks * sectionsPerStack> held {};
    size_t running = 0;
};
