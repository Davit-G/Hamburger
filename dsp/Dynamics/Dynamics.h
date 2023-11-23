#pragma once

#include <JuceHeader.h>
#include "Compressor.h"

#include "../SmoothParam.h"
#include "MBComp.h"
#include "MSComp.h"
#include "StereoComp.h"

class Dynamics
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state) :
        mbComp(state),
        msComp(state),
        stereoComp(state)
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
        case 0: // "STEREO"
            stereoComp.processBlock(block);
            break;
        case 1: // "OTT"
            mbComp.processBlock(block);
            break;
        case 2: // "MID-SIDE" 
            msComp.processBlock(block);
            break;
        case 3:
            
            break;
        default:
            break;
        }

    }

    void prepare(dsp::ProcessSpec& spec) {
        mbComp.prepare(spec);
        msComp.prepare(spec);
        stereoComp.prepare(spec);
    }

private:
    // juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterBool* enabled = nullptr;

    MBComp mbComp;
    MSComp msComp;
    StereoComp stereoComp;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dynamics)
};