#pragma once

 
#include "../SmoothParam.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"

class HarshGate : public MacroEffect
{
public:
    HarshGate(juce::AudioProcessorValueTreeState& treeState)
        : MacroEffect(treeState, SlotId{ModuleId::module1, 0}),
          amount(getParam(ParamIDs::gateAmt)),
          smooth(getParam(ParamIDs::gateSmooth)) {}

    void processBlock(juce::dsp::AudioBlock<float>& block) override {
        amount.update();
        smooth.update();

        for (size_t sample = 0; sample < block.getNumSamples(); sample++) {
            float amt = amount.getNextValue(0) * 0.8f;
            amt = amt * amt * amt * 3.0f;

            const float l = block.getSample(0, (int) sample);
            const float r = block.getSample(1, (int) sample);
            const float lr = l + r;
            
            const float halfKnee = amt * smooth.getNextValue(0) * 0.01f;
            const float lower = amt - halfKnee, upper = amt + halfKnee;
            const float level = std::abs(lr);

            float open = level >= upper ? 1.0f : 0.0f;

            if (level > lower && level < upper) {
                const float through = (level - lower) / (upper - lower);
                open = through * through * (3.0f - 2.0f * through);
            }

            const float push = lr >= 0.0f ? amt : -amt;
            block.setSample(0, (int) sample, (l + push) * open);
            block.setSample(1, (int) sample, (r + push) * open);
        }
    }

    void prepare(juce::dsp::ProcessSpec& spec) override {
        amount.prepare(spec);
        smooth.prepare(spec);
    }
private:
    SmoothParam amount;
    SmoothParam smooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarshGate)
};