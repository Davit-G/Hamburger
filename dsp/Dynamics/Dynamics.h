#pragma once

 
#include "Compressor.h"

#include "../SmoothParam.h"
#include "MBComp.h"
#include "MSComp.h"
#include "StereoComp.h"
#include "TypeA.h"

#include "../EffectBase.h"

// todo: add compander class, omfg imagine multiband compander
// todo: add gate?

class Dynamics : public EffectBase
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state, ScopeDataCollector<float> &dataCollector) :
        mbComp(state, dataCollector),
        msComp(state, dataCollector),
        stereoComp(state, dataCollector),
        typeA(state, dataCollector)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter(SlotId{ModuleId::dynamics, 0}.type().getParamID())); jassert(distoType);
        enabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter(SlotId{ModuleId::dynamics, 0}.enabled().getParamID())); jassert(enabled);
    }
    ~Dynamics() override {}


    void processBlock(juce::dsp::AudioBlock<float>& block) override {
        int distoTypeIndex = distoType->getIndex();


        if (!enabled->get()) return;

        switch (distoTypeIndex)
        {
        case 0: // STEREO
            stereoComp.processBlock(block);
            break;
        case 1: // MB
            mbComp.processBlock(block);
            break;
        case 2: // MS
            msComp.processBlock(block);
            break;
        case 3: // TYPE A
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

    void prepare(juce::dsp::ProcessSpec& spec) override {
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