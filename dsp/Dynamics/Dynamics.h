#pragma once

#include <JuceHeader.h>
#include "Compressor.h"

#include "../SmoothParam.h"
#include "MBComp.h"

class Dynamics
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state) :
        mbComp(state)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("compressionType")); jassert(distoType);
        enabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("compressionOn")); jassert(enabled);
    }
    ~Dynamics() {}

    void processBlock(dsp::AudioBlock<float>& block) {
        if (!enabled->get()) return;
        int distoTypeIndex = distoType->getIndex();

        switch (distoTypeIndex)
        {
        case 0: // "COMPANDER"
            mbComp.processBlock(block);
            break;
        case 1: // "OTT",
            
            break;
        case 2: // "MID-SIDE"
            
            break;
        case 3:
            
            break;
        default:
            break;
        }

    }

    void prepare(dsp::ProcessSpec& spec) {
        mbComp.prepare(spec);
    }

private:
    // juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterBool* enabled = nullptr;

    MBComp mbComp;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dynamics)
};