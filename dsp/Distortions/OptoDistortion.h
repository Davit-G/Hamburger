#pragma once

#include "../SmoothParam.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"
#include "../OptoCell.h"

/*  Audio through two LEDs back to back to ground, the way pedals use them as clipping diodes, and an LDR they light. Each
    LED has its own forward voltage, so the two halves of the wave clip at different points, and it's class A biased, resting
    partway to the first LED's knee so that half goes first. The LDR sees the light from whatever the LEDs clip away, follows
    it with an LA-2A's timing and turns the audio down by it, so the harder it clips the quieter it gets, cycle by cycle in
    the bass and as a whole above it.

    Grill's bias is here too: the level, followed at the dc speed, pushes the resting point further toward the first knee the
    louder it gets. The curve at the resting point is taken off again, so moving it doesn't move the audio's centre. */
class OptoDistortion : public MacroEffect
{
public:
    OptoDistortion (juce::AudioProcessorValueTreeState& state, SlotId slot)
        : MacroEffect (state, slot),
          drive (getParam (ParamIDs::optoDrive)),
          bias (getParam (ParamIDs::optoBias)),
          dcSpeed (getParam (ParamIDs::optoDcSpeed)),
          damping (getParam (ParamIDs::optoDamping)) {}

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        drive.prepare (spec);
        bias.prepare (spec);
        envelope.prepare (spec.sampleRate);

        for (auto& cell : cells)
            cell.prepare (spec.sampleRate);
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        drive.update();
        bias.update();

        const auto ms = dcSpeed.getRaw (0), damp = damping.getRaw (0);
        envelope.setTimes (ms, ms, damp);

        for (auto& cell : cells)
            cell.setSpeed (cellSpeedMs, damp);

        const auto numChannels = juce::jmin (block.getNumChannels(), cells.size());

        for (size_t n = 0; n < block.getNumSamples(); ++n)
        {
            const auto gainIn = juce::Decibels::decibelsToGain (drive.getNextValue (0));

            auto level = 0.0f;
            for (size_t ch = 0; ch < numChannels; ++ch)
                level += std::abs (block.getSample ((int) ch, (int) n));

            const auto offset = resting + envelope.process (level * gainIn / (float) numChannels) * bias.getNextValue (0) * biasRange;
            const auto centre = leds.clip (offset);

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                const auto volts = block.getSample ((int) ch, (int) n) * gainIn + offset;
                const auto clipped = leds.clip (volts);
                // either LED lights the cell, by as much as it clips away
                const auto glow = cells[ch].process (std::abs (volts - clipped));
                block.setSample ((int) ch, (int) n, (clipped - centre) / (1.0f + glow * sensitivity));
            }
        }
    }

private:
    struct Leds
    {
        float forwardPositive, forwardNegative, knee;

        // straight through until an LED conducts, then bending over into its forward voltage, each side at its own
        float clip (float volts) const
        {
            if (volts > forwardPositive)
                return forwardPositive + knee * std::log1p ((volts - forwardPositive) / knee);

            if (volts < -forwardNegative)
                return -forwardNegative - knee * std::log1p ((-volts - forwardNegative) / knee);

            return volts;
        }
    };

    // in full scale, where each LED starts to conduct and how soft its knee is, and where the class A bias rests
    static constexpr Leds leds { 0.6f, 0.9f, 0.08f };
    static constexpr float resting = 0.3f;
    static constexpr float cellSpeedMs = 5.0f;  // 6 ms to light, 54 ms to let go
    static constexpr float sensitivity = 4.0f;  // how far the audio is turned down by a unit of light
    static constexpr float biasRange = 1.0f;    // at full, a loud signal moves the resting point about as far as the knee

    SmoothParam drive, bias, dcSpeed, damping;
    DampedFollower envelope;
    std::array<OptoCell, 2> cells;
};
