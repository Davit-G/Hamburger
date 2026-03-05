#pragma once

 
#include "Compressor.h"

#include "../SmoothParam.h"
#include "MBComp.h"
#include "MSComp.h"
#include "StereoComp.h"
#include "TypeA.h"

// todo: add compander class, omfg imagine multiband compander
// todo: add gate?

class Dynamics
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state) :
        mbComp(state),
        msComp(state),
        stereoComp(state),
        typeA(state)
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
            // "TYPE A"
            typeA.processBlock(block);
            break;
        case 4: // "DUAL-MONO"
            // currently same as stereo :(
            stereoComp.processBlock(block);
            break;
        default:
            break;
        }

    }

    void prepare(dsp::ProcessSpec& spec) {
        mbComp.prepare(spec);
        msComp.prepare(spec);
        stereoComp.prepare(spec);
        typeA.prepareToPlay(spec.sampleRate, spec.maximumBlockSize, spec.numChannels);
    }

private:
    // juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterBool* enabled = nullptr;

    MBComp mbComp;
    MSComp msComp;
    StereoComp stereoComp;
    TypeAProcessor typeA;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dynamics)
};