#pragma once

#include "../EffectBase.h"
#include "../SmoothParam.h"
#include "../FrequencyShifting/hiir/PolyphaseIir2Designer.h"

/*  A pair of allpass chains ninety degrees apart, designed for the sample rate. The same sample through both gives the
    analytic signal: its magnitude is the envelope, the angle between the two is the pitch. */
struct HilbertPair
{
    static constexpr int hilbertCoefs = 12;
    static constexpr double lowestHz = 20.0; // the pair is ninety degrees apart from here up

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
        delayed = 0.0f;
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
    float delayed = 0.0f;
    double sampleRate = 44100.0;
};

/*  A phase locked loop on the analytic signal's phase, and one oscillator at whatever step of that phase the ratio knob
    asks for. The loop is what makes the tone hold onto the pitch: the phase it finds is the whole turn of the fundamental,
    so 2 is an octave above it, 0.5 an octave below, and the tone stays in step with the partials the input already has
    rather than beating against them.

    The tone has its own phase accumulator rather than being the loop's phase multiplied by the ratio after the wrap, which
    is what keeps the ratios under 1 click free. Multiplying after the wrap jumps the tone by half a turn at 0.5 and a
    quarter at 0.25 once every cycle of the input, and that jump is the click; accumulating the tone's own turn keeps the
    wrap a whole turn and nothing moves. Only the ratios where the step is a whole number of the tone's turns, 0.5 and 0.25,
    could be measured against the tone's phase the way the sub gen did, so the tracking stays on the loop's.

    The loop follows whichever partial carries the most energy, so on a single tone it is the pitch, and on a fuller sound
    it sits somewhere under it. Everything is put back at the envelope the analytic signal measures, so the tone goes quiet
    with the input and follows it as it swells.

    A DC offset has no phase to lock to, so the loop settles wherever it is and puts the offset back as a constant tone at
    zero phase. Only the tracking is blocked, the input itself goes through as it is. */
struct HarmonicBank
{
    static constexpr float highestTrackedHz = 1200.0f; // the loop can't chase past this, so the top ratio tops out near 19.2 kHz, under any nyquist
    static constexpr double blockedHz = 10.0;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        pair.prepare (sampleRate);

        phase = tonePhase = frequency = held = previous = 0.0f;
        pole = (float) std::exp (-2.0 * juce::MathConstants<double>::pi * blockedHz / sampleRate);

        // the loop's gains are per sample, so they're scaled to keep it as quick in time at any rate
        const auto perSample = (float) (48000.0 / sampleRate);
        phaseGain = 0.25f / juce::MathConstants<float>::pi * perSample;
        frequencyGain = 0.0625f / juce::MathConstants<float>::pi * perSample * perSample;
        highestFrequency = (float) (highestTrackedHz / sampleRate);
    }

    // the oscillator at the ratio of the pitch it's locked to, over the envelope the pair measures
    float process (float x, float ratio)
    {
        const auto [re, im] = pair.analytic (blockDC (x));
        const auto magnitude = std::sqrt (re * re + im * im);
        const auto inverse = 1.0f / std::max (magnitude, 1e-8f);

        const auto turn = juce::MathConstants<float>::twoPi * phase;
        const auto c = std::cos (turn), s = std::sin (turn);

        // how far the analytic signal's phase is ahead of the loop's, as the sine of the difference
        const auto error = im * inverse * c - re * inverse * s;

        frequency = juce::jlimit (0.0f, highestFrequency, frequency + error * frequencyGain);
        const auto advance = error * phaseGain + frequency;

        phase += advance;
        phase -= std::floor (phase);

        // the tone's turn, moved by the ratio of the loop's, wrapped in its own turns so a fractional ratio can't jump
        tonePhase += ratio * advance;
        tonePhase -= std::floor (tonePhase);

        return std::cos (juce::MathConstants<float>::twoPi * tonePhase) * magnitude;
    }

private:
    // one pole at 10Hz, y = p y⁻¹ + x - x⁻¹, so a level that holds still puts out nothing
    float blockDC (float x)
    {
        const auto y = pole * held + x - previous;
        previous = x;
        held = y;
        return y;
    }

    HilbertPair pair;
    float phase = 0.0f, tonePhase = 0.0f, frequency = 0.0f;
    float phaseGain = 0.0f, frequencyGain = 0.0f, highestFrequency = 0.0f;
    float pole = 0.0f, held = 0.0f, previous = 0.0f;
    double sampleRate = 44100.0;
};

// each channel tracks its own pitch, so the tone stays on that side of the image
class Harmonic : public MacroEffect
{
public:
    static constexpr int maxChannels = 2;

    explicit Harmonic (juce::AudioProcessorValueTreeState& state)
        : MacroEffect (state, SlotId { ModuleId::module2, 0 }),
          amount (getParam (ParamIDs::harmonicAmount)),
          ratio (getParam (ParamIDs::harmonicRatio)) {}

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        for (auto* param : { &amount, &ratio })
            param->prepare (spec);

        for (auto& bank : banks)
            bank.prepare (spec.sampleRate);
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        for (auto* param : { &amount, &ratio })
            param->update();

        const auto numChannels = juce::jmin (block.getNumChannels(), (size_t) maxChannels);

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto& bank = banks[ch];

            for (size_t n = 0; n < block.getNumSamples(); ++n)
            {
                // the knob's index, snapped to the list, so a value the mod matrix puts between two steps lands on one
                const auto step = harmonicRatio (ratio.getNextValue ((int) ch));

                // the loop reads the sample as it comes in, before its own tone is added on top of it
                const auto input = block.getSample ((int) ch, (int) n);
                const auto added = bank.process (input, step) * amount.getNextValue ((int) ch) * 0.01f;

                block.addSample ((int) ch, (int) n, added);
            }
        }
    }

private:
    SmoothParam amount, ratio;
    std::array<HarmonicBank, maxChannels> banks {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Harmonic)
};
