#pragma once

#include "../EffectBase.h"
#include "../SmoothParam.h"
#include "../FrequencyShifting/HilbertBiquad.h"

/*  The phase distortion's frequency shifter on its own, the whole input moved up or down by the same number of hz, so
    its harmonics stop lining up. The shift knob maps the same way, and keytracks the same way, see ModMatrix.

    Feedback puts what came out back in, the sample before or up to maxDelayMs before that, so what's come through is
    shifted again on every pass and spirals up or down, a step of the shift each time. The longer the delay, the more each
    pass is an echo of its own rather than part of one smear. In ahead of the foldback highpass, so passes shifted down past 0 Hz are taken
    out rather than piling up. The shifter is at unity gain only where its two allpass chains are a quarter turn apart, and
    under about 20 Hz they aren't, so what's fed back is soft clipped: the loop can ring there but never run away. */
class FreqShift : public MacroEffect
{
public:
    explicit FreqShift (juce::AudioProcessorValueTreeState& state)
        : MacroEffect (state, SlotId { ModuleId::module2, 0 }),
          shift (getParam (ParamIDs::freqShift)),
          feedback (getParam (ParamIDs::freqShiftFeedback)),
          delay (getParam (ParamIDs::freqShiftDelay)) {}

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        shift.prepare (spec);
        feedback.prepare (spec);
        delay.prepare (spec);
        sampleRate = spec.sampleRate;
        longestDelay = 1.0f + (float) (maxDelayMs * 0.001 * sampleRate);

        for (auto& channel : channels)
        {
            channel.shifter.prepare (spec);
            channel.foldback.reset();
            channel.output.assign ((size_t) longestDelay + 2, 0.0f);
            channel.written = 0;
        }

        foldbackCutoff = 0.0;
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        shift.update();
        feedback.update();
        delay.update();

        /*  The shift is in cycles per sample at 44.1kHz, see HilbertBiquadShifter, so its cube times 0.1 times 44100 is
            how far it shifts in hz. Under a downward shift, whatever's below it would fold back off 0 Hz, so it's taken
            out first. Shifting up, the highpass sits out of the way. */
        const auto shiftHz = std::pow ((double) shift.getRaw (0), 3.0) * 0.1 * 44100.0;
        const auto cutoff = juce::jlimit (minFoldbackHz, sampleRate * 0.25, -shiftHz);

        if (cutoff != foldbackCutoff)
        {
            foldbackCutoff = cutoff;

            for (auto& channel : channels)
                channel.foldback.setCutoff (cutoff, sampleRate);
        }

        const auto numChannels = juce::jmin (block.getNumChannels(), channels.size());

        for (size_t n = 0; n < block.getNumSamples(); ++n)
        {
            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto& channel = channels[ch];
                const auto increment = std::pow (shift.getNextValue ((int) ch), 3.0f) * 0.1f;
                const auto amount = feedback.getNextValue ((int) ch) * 0.01f * maxFeedback;
                // a sample, the shortest a loop can be, then however long the knob adds on
                const auto delaySamples = juce::jlimit (1.0f, longestDelay, 1.0f + delay.getNextValue ((int) ch) * 0.001f * (float) sampleRate);
                auto& sample = block.getChannelPointer (ch)[n];

                const auto fedBack = amount * feedbackCeiling * std::tanh (channel.before (delaySamples) / feedbackCeiling);
                sample = channel.shifter.shift ((float) channel.foldback.processSample (sample + fedBack), increment);

                channel.output[channel.written] = sample;
                channel.written = (channel.written + 1) % channel.output.size();
            }
        }
    }

private:
    static constexpr double minFoldbackHz = 5.0;
    static constexpr float maxFeedback = 0.95f;     // full feedback, kept far enough under 1 that a ringing pass still dies away
    static constexpr float feedbackCeiling = 2.0f;  // where the clip levels off, so it leaves anything under about -6 dB alone
    static constexpr double maxDelayMs = 50.0;      // the delay knob's top, on top of the sample it starts at, see ParamIDs::freqShiftDelay

    struct Channel
    {
        HilbertBiquadShifter shifter;
        Butterworth foldback;
        std::vector<float> output; // the last of what came out, written round and round
        size_t written = 0;

        // what came out delaySamples ago, 1 the sample before, a fraction between the two either side so a moving delay glides
        float before (float delaySamples) const
        {
            auto position = (float) written - delaySamples;

            if (position < 0.0f)
                position += (float) output.size();

            const auto at = (size_t) position;
            const auto fraction = position - (float) at;
            return output[at] + fraction * (output[(at + 1) % output.size()] - output[at]);
        }
    };

    SmoothParam shift, feedback, delay;
    std::array<Channel, 2> channels;
    double sampleRate = 44100.0, foldbackCutoff = 0.0;
    float longestDelay = 1.0f;
};
