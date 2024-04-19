#pragma once

 
#include "Compressor.h"

#include "../SmoothParam.h"
#include "MBComp.h"
#include "MSComp.h"
#include "StereoComp.h"

// todo: add compander class, omfg imagine multiband compander
// todo: add gate?

class Dynamics
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state) :
        mbComp(state),
        msComp(state),
        stereoComp(state)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter(ParamIDs::compressionType.getParamID())); jassert(distoType);
        enabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter(ParamIDs::compressionOn.getParamID())); jassert(enabled);
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