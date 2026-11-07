#pragma once

#include "../EffectBase.h"
#include "../SmoothParam.h"
#include "../LinearPrediction.h"

/*  The input's own formants taken off and put back moved, see LinearPrediction, with nothing between. The pitch stays
    where it is, only the resonances over it move, so a voice or an instrument's body comes out bigger or smaller. At no
    shift and full depth it's the input back exactly. The input's own slope is taken off before the formants are measured
    and put back as it was after, so shifting and deepening them never makes it any bassier or brighter overall. */
class FormantShift : public MacroEffect
{
public:
    explicit FormantShift (juce::AudioProcessorValueTreeState& state)
        : MacroEffect (state, SlotId { ModuleId::module2, 0 }),
          shift (getParam (ParamIDs::formantShift)),
          depth (getParam (ParamIDs::formantDepth)),
          resonance (getParam (ParamIDs::formantResonance)) {}

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        for (auto* param : { &shift, &depth, &resonance })
            param->prepare (spec);

        sampleRate = spec.sampleRate;
        prediction.prepare (spec.sampleRate, (int) spec.maximumBlockSize, (int) spec.numChannels);
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        for (auto* param : { &shift, &depth, &resonance })
            param->update();

        // as many stages for the formants up to 20 kHz at any oversampling
        const auto perRate = juce::jlimit (1, 4, juce::roundToInt (sampleRate / 48000.0));

        prediction.whiten (block, { order * perRate,
                                    0.0,
                                    true,
                                    std::exp2 (shift.getRaw (0) / 12.0),
                                    depth.getRaw (0) * 0.01,
                                    resonance.getRaw (0) * maxNarrowerHz * 0.01 });
        prediction.colour (block);
    }

private:
    static constexpr int order = 32; // fewer leave the formants a few dB proud of the excitation, and those then stay put
    static constexpr double maxNarrowerHz = 120.0; // how much the resonance at either end narrows or widens every formant

    SmoothParam shift, depth, resonance;
    double sampleRate = 44100.0;
    LinearPrediction prediction;
};
