#pragma once

#include "../EffectBase.h"
#include "../EffectInfos.h"
#include "../SmoothParam.h"
#include "../Crossover.h"
#include "../FrequencyShifting/hiir/PolyphaseIir2Designer.h"

/*  A sine an octave below whatever's in the lows, at their level. The lows are made into an analytic signal by a pair of
    allpass chains ninety degrees apart, designed for the sample rate, and a phase locked loop turns an oscillator at half
    the analytic signal's phase. */
struct SubOctave
{
    static constexpr int hilbertCoefs = 12;
    static constexpr double lowestHz = 20.0; // the pair is ninety degrees apart from here up
    static constexpr float lowsHz = 250.0f;
    static constexpr float highestSubHz = 200.0f;

    // the two chains' coefficients in turn, first the undelayed chain's then the other's
    static std::array<float, hilbertCoefs> design (double rate)
    {
        std::array<double, hilbertCoefs> designed {};
        hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw (designed.data(), hilbertCoefs, lowestHz / rate);

        std::array<float, hilbertCoefs> result {};

        for (size_t i = 0; i < result.size(); ++i)
            result[i] = (float) designed[i];

        return result;
    }

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        coefs = design (sampleRate);

        allpasses = {};
        delayed = phase = frequency = 0.0f;

        // the loop's gains are per sample, so they're scaled to keep it as quick in time at any rate
        const auto perSample = (float) (48000.0 / sampleRate);
        phaseGain = 0.25f / juce::MathConstants<float>::pi * perSample;
        frequencyGain = 0.0625f / juce::MathConstants<float>::pi * perSample * perSample;
        highestFrequency = (float) (highestSubHz / sampleRate);
    }

    float process (float lows)
    {
        const auto [re, im] = analytic (lows);
        const auto magnitude = std::sqrt (re * re + im * im);
        const auto inverse = 1.0f / std::max (magnitude, 1e-8f);

        const auto turn = juce::MathConstants<float>::twoPi * phase;
        const auto c = std::cos (turn), s = std::sin (turn);

        // how far the lows' phase is ahead of twice the oscillator's, as the sine of the difference
        const auto error = 0.5f * (im * inverse * (c * c - s * s) - re * inverse * 2.0f * c * s);

        frequency = juce::jlimit (0.0f, highestFrequency, frequency + error * frequencyGain);
        phase += error * phaseGain + frequency;
        phase -= std::floor (phase);

        return c * magnitude;
    }

    // every other coefficient on each chain, the second fed a sample late, each section (a - z^-2) / (1 - a z^-2)
    std::pair<float, float> analytic (float x)
    {
        auto re = x, im = delayed;
        delayed = x;

        for (size_t i = 0; i < coefs.size(); i += 2)
        {
            re = allpasses[i].run (coefs[i], re);
            im = allpasses[i + 1].run (coefs[i + 1], im);
        }

        return { re, im };
    }

    struct Allpass
    {
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

        float run (float a, float x)
        {
            const auto y = a * (x + y2) - x2;
            x2 = x1;
            x1 = x;
            y2 = y1;
            y1 = y;
            return y;
        }
    };

    std::array<float, hilbertCoefs> coefs {};
    std::array<Allpass, hilbertCoefs> allpasses {};
    float delayed = 0.0f, phase = 0.0f, frequency = 0.0f;
    float phaseGain = 0.0f, frequencyGain = 0.0f, highestFrequency = 0.0f;
    double sampleRate = 44100.0;
};

// the sub is made from both channels together and added to each, so it stays mono
class SubGen : public MacroEffect
{
public:
    explicit SubGen (juce::AudioProcessorValueTreeState& state)
        : MacroEffect (state, SlotId { ModuleId::module2, 0 }),
          amount (getParam (ParamIDs::subGenAmount)) {}

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        amount.prepare (spec);
        lows.prepare (spec);
        lows.setCutoffFrequency (SubOctave::lowsHz);
        sub.prepare (spec.sampleRate);
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        amount.update();

        const auto numChannels = block.getNumChannels();

        for (size_t n = 0; n < block.getNumSamples(); ++n)
        {
            auto mid = 0.0f;

            for (size_t ch = 0; ch < numChannels; ++ch)
                mid += block.getSample ((int) ch, (int) n);

            float low = 0.0f, high = 0.0f;
            lows.processSample (0, mid / (float) numChannels, low, high);

            const auto added = sub.process (low) * amount.getNextValue (0) * 0.01f;

            for (size_t ch = 0; ch < numChannels; ++ch)
                block.addSample ((int) ch, (int) n, added);
        }
    }

private:
    SmoothParam amount;
    Crossover lows;
    SubOctave sub;
};
