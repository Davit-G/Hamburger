#pragma once

#include "../SmoothParam.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"
#include "../OptoCell.h"
#include "../../gui/Modules/ScopeDataCollector.h"
#include "ThreeBands.h"

/*  An opto compressor lit straight by the audio. Two LEDs back to back light the cell once the audio passes their forward
    voltage: the first at the threshold, the other a little higher, so the two halves of the wave light it unevenly. The LDR
    follows the light at the speed, with an LA-2A's attack to release, and turns the audio down by the ratio, taking how far
    it glows as how far over the threshold the audio is. The audio only ever passes the LDR and never goes through the LEDs,
    so it's turned down rather than clipped.

    Everything's measured against the threshold, so it compresses as hard wherever the threshold is. Linked, both cells see
    the light from both channels. */
class OptoCells
{
public:
    static constexpr float damping = 1.25f;
    static constexpr float otherForward = 1.5f; // the second LED conducts 3.5 dB past the threshold
    static constexpr float knee = 0.08f;        // how soft each LED's knee is, in threshold units

    void prepare (double sampleRate)
    {
        for (auto& cell : cells)
            cell.prepare (sampleRate);
    }

    void setSpeed (float ms)
    {
        for (auto& cell : cells)
            cell.setSpeed (ms, damping);
    }

    // reduction is how much of each dB over the threshold is taken off, 1 - 1 / ratio
    void process (float& left, float& right, float perThreshold, float reduction, float linked)
    {
        const std::array<float, 2> light { lit (left * perThreshold), lit (right * perThreshold) };
        const auto shared = (light[0] + light[1]) * 0.5f;

        for (auto [ch, x] : { std::pair<size_t, float*> { 0, &left }, std::pair<size_t, float*> { 1, &right } })
        {
            const auto glow = cells[ch].process (light[ch] + (shared - light[ch]) * linked);
            *x *= std::pow (1.0f + glow, -reduction);
        }
    }

    // what a full scale sine would be turned down by once the cells settled, which the output's turned back up by
    static float autoGain (float perThreshold, float reduction)
    {
        // the average of the wave past an LED's forward voltage, over a whole cycle
        auto past = [amplitude = perThreshold] (float forward)
        {
            if (amplitude <= forward)
                return 0.0f;

            return (2.0f * std::sqrt (amplitude * amplitude - forward * forward)
                    - forward * (juce::MathConstants<float>::pi - 2.0f * std::asin (forward / amplitude)))
                   / juce::MathConstants<float>::twoPi;
        };

        return std::pow (1.0f + past (1.0f) + past (otherForward), reduction);
    }

private:
    // the light from the two LEDs, each conducting past its forward voltage through a soft knee, in units of the threshold
    static float lit (float volts)
    {
        return knee * (softplus ((volts - 1.0f) / knee) + softplus ((-volts - otherForward) / knee));
    }

    static float softplus (float x) { return x > 20.0f ? x : std::log1p (std::exp (x)); }

    std::array<OptoCell, 2> cells;
};

class Opto : public MacroEffect
{
public:
    Opto (juce::AudioProcessorValueTreeState& state, ScopeDataCollector<float>& dataCollector)
        : MacroEffect (state, SlotId { ModuleId::dynamics, 0 }),
          threshold (getParam (ParamIDs::optoThreshold)),
          ratio (getParam (ParamIDs::optoRatio)),
          speed (getParam (ParamIDs::optoCompSpeed)),
          link (getParam (ParamIDs::compStereoLink)),
          scopeDataCollector (dataCollector) {}

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        threshold.prepare (spec);
        cells.prepare (spec.sampleRate);
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        threshold.update();
        cells.setSpeed (speed.getRaw (0));

        const auto reduction = 1.0f - 1.0f / ratio.getRaw (0);
        const auto linked = link.getRaw (0) * 0.01f;
        const auto makeup = OptoCells::autoGain (1.0f / juce::Decibels::decibelsToGain (threshold.getRaw (0)), reduction);

        // the loudest each channel came in, for the scope to set against the threshold
        std::array<float, 2> loudest {};

        for (size_t n = 0; n < block.getNumSamples(); ++n)
        {
            auto left = block.getSample (0, (int) n), right = block.getSample (1, (int) n);
            loudest = { juce::jmax (loudest[0], std::abs (left)), juce::jmax (loudest[1], std::abs (right)) };
            cells.process (left, right, 1.0f / juce::Decibels::decibelsToGain (threshold.getNextValue (0)), reduction, linked);

            block.setSample (0, (int) n, left * makeup);
            block.setSample (1, (int) n, right * makeup);
        }

        scopeDataCollector.band1.accumulate (loudest[0]);
        scopeDataCollector.band2.accumulate (loudest[1]);
    }

private:
    SmoothParam threshold, ratio, speed, link;
    OptoCells cells;

    ScopeDataCollector<float>& scopeDataCollector;
};

// the same in lows, mids and highs, each band lighting its own cells, linked across both channels, with the threshold
// tilted down in the lows and up in the highs, or the other way
class MBOpto : public MacroEffect
{
public:
    MBOpto (juce::AudioProcessorValueTreeState& state, ScopeDataCollector<float>& dataCollector, ThreeBands& threeBands)
        : MacroEffect (state, SlotId { ModuleId::dynamics, 0 }),
          threshold (getParam (ParamIDs::optoThreshold)),
          ratio (getParam (ParamIDs::optoRatio)),
          speed (getParam (ParamIDs::optoCompSpeed)),
          tilt (getParam (ParamIDs::compBandTilt)),
          scopeDataCollector (dataCollector),
          bands (threeBands) {}

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        threshold.prepare (spec);

        for (auto& band : cells)
            band.prepare (spec.sampleRate);
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        threshold.update();

        for (auto& band : cells)
            band.setSpeed (speed.getRaw (0));

        const auto reduction = 1.0f - 1.0f / ratio.getRaw (0);
        const auto makeup = OptoCells::autoGain (1.0f / juce::Decibels::decibelsToGain (threshold.getRaw (0)), reduction);

        // each band's threshold against the mids', as a gain, the lows down by the tilt and the highs up
        const auto tilted = juce::Decibels::decibelsToGain (tilt.getRaw (0));
        const std::array<float, 3> bandScale { tilted, 1.0f, 1.0f / tilted };

        bands.split (block);

        // the loudest each band came in, for the scope to set against its tilted threshold
        std::array<float, 3> loudest {};

        for (size_t n = 0; n < block.getNumSamples(); ++n)
        {
            const auto perThreshold = 1.0f / juce::Decibels::decibelsToGain (threshold.getNextValue (0));
            auto outL = 0.0f, outR = 0.0f;

            for (size_t i = 0; i < cells.size(); ++i)
            {
                auto left = bands.bands[i].getSample (0, (int) n), right = bands.bands[i].getSample (1, (int) n);
                loudest[i] = juce::jmax (loudest[i], std::abs (left), std::abs (right));
                cells[i].process (left, right, perThreshold * bandScale[i], reduction, 1.0f);
                outL += left;
                outR += right;
            }

            block.setSample (0, (int) n, outL * makeup);
            block.setSample (1, (int) n, outR * makeup);
        }

        for (auto [meter, level] : { std::pair { &scopeDataCollector.band1, loudest[0] }, std::pair { &scopeDataCollector.band2, loudest[1] },
                                     std::pair { &scopeDataCollector.band3, loudest[2] } })
            meter->accumulate (level);
    }

private:
    SmoothParam threshold, ratio, speed, tilt;
    std::array<OptoCells, 3> cells;

    ScopeDataCollector<float>& scopeDataCollector;
    ThreeBands& bands;
};
