#pragma once

 
#include "../SmoothParam.h"

//==============================================================================
/*
 */
class HarshGate
{
public:
    HarshGate(juce::AudioProcessorValueTreeState& treeState): amount(treeState, ParamIDs::gateAmt), mix(treeState, ParamIDs::gateMix) {};

    void processBlock(dsp::AudioBlock<float>& block) {
        amount.update();
        mix.update();

        auto rightDryData = block.getChannelPointer(1);
        auto leftDryData = block.getChannelPointer(0);

        for (int sample = 0; sample < block.getNumSamples(); sample++) {
            float amt = amount.getNextValue() * 0.8f;
            amt = amt * amt * amt * 3.0f;

            float l = leftDryData[sample];
            float r = rightDryData[sample];

            auto lr = l + r;
            
            
            float blend = mix.getNextValue();

            if (abs(lr) < amt) {
                block.setSample(0, sample, (0.0f) * blend + l * (1 - blend));
                block.setSample(1, sample, (0.0f) * blend + r * (1 - blend));
                continue;
            }

            if (lr > -amt) {
                block.setSample(0, sample, (l + amt) * blend + l * (1 - blend));
                block.setSample(1, sample, (r + amt) * blend + r * (1 - blend));
            } else if (lr < amt) {
                block.setSample(0, sample, (l - amt) * blend + l * (1 - blend));
                block.setSample(1, sample, (r - amt) * blend + r * (1 - blend));
            }


        }
    }
    
    void prepare(dsp::ProcessSpec& spec) {
        amount.prepare(spec);
    }
private:
    SmoothParam amount;
    SmoothParam mix;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarshGate);
};