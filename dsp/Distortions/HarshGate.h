#pragma once

 
#include "../SmoothParam.h"

//==============================================================================
/*
 */
class HarshGate
{
public:
    HarshGate(juce::AudioProcessorValueTreeState& treeState): amount(treeState, "gateAmt") {};

    void processBlock(dsp::AudioBlock<float>& block) {
        amount.update();

        auto rightDryData = block.getChannelPointer(1);
        auto leftDryData = block.getChannelPointer(0);

        for (int sample = 0; sample < block.getNumSamples(); sample++) {
            float amt = amount.getNextValue() * 0.4;

            float l = leftDryData[sample];
            float r = rightDryData[sample];

            auto lr = l + r;
            
            // basic piecewise

            if (abs(lr) < amt) {
                block.setSample(0, sample, 0.0f);
                block.setSample(1, sample, 0.0f);
                continue;
            }

            if (lr > amt) {
                block.setSample(0, sample, l - amt);
                block.setSample(1, sample, r - amt);
            } else if (lr < -amt) {
                block.setSample(0, sample, l + amt);
                block.setSample(1, sample, r + amt);
            }
        }
    }
    
    void prepare(dsp::ProcessSpec& spec) {
        amount.prepare(spec);
    }
private:
    SmoothParam amount;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarshGate);
};