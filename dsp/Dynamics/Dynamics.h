#pragma once

 
#include "Compressor.h"

#include "../SmoothParam.h"
#include "MBComp.h"
#include "MSComp.h"
#include "StereoComp.h"
#include "TypeA.h"
#include "Transient.h"
#include "Opto.h"

#include "../EffectBase.h"

// todo: add compander class, omfg imagine multiband compander
// todo: add gate?

class Dynamics : public EffectBase
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state, ScopeDataCollector<float> &dataCollector) :
        mbComp(state, dataCollector, threeBands),
        msComp(state, dataCollector),
        stereoComp(state, dataCollector),
        typeA(state, dataCollector),
        transient(state, dataCollector),
        mbTransient(state, dataCollector, threeBands),
        opto(state, dataCollector),
        mbOpto(state, dataCollector, threeBands)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter(SlotId{ModuleId::dynamics, 0}.type().getParamID())); jassert(distoType);
        enabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter(SlotId{ModuleId::dynamics, 0}.enabled().getParamID())); jassert(enabled);
    }
    ~Dynamics() override {}


    void setLinearCrossovers(bool linear) { threeBands.setLinear(linear); }

    // linear, every type is delayed as much as the multiband ones, so the latency doesn't move with the type
    int getLatencySamples() const override { return threeBands.isLinear() ? threeBands.getLatency() : 0; }

    Crossovers getCrossovers() const override
    {
        return enabled->get() && isMultiband() && ! threeBands.isLinear() ? ThreeBands::crossovers() : Crossovers {};
    }

    void processBlock(juce::dsp::AudioBlock<float>& block) override {
        int distoTypeIndex = distoType->getIndex();


        if (!enabled->get()) return;

        if (threeBands.isLinear() && ! isMultiband())
            threeBands.delayBy(block);

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
        case 4: // TRANSIENT
            transient.processBlock(block);
            break;
        case 5: // MB TRANSIENT
            mbTransient.processBlock(block);
            break;
        case 6: // OPTO
            opto.processBlock(block);
            break;
        case 7: // MB OPTO
            mbOpto.processBlock(block);
            break;
        default:
            break;
        }

    }

    void prepare(juce::dsp::ProcessSpec& spec) override {
        threeBands.prepare(spec);
        mbComp.prepare(spec);
        msComp.prepare(spec);
        stereoComp.prepare(spec);
        typeA.prepareToPlay(spec.sampleRate, spec.maximumBlockSize, spec.numChannels);
        transient.prepare(spec);
        mbTransient.prepare(spec);
        opto.prepare(spec);
        mbOpto.prepare(spec);
    }

private:
    // MB, MB TRANSIENT and MB OPTO
    bool isMultiband() const { const auto type = distoType->getIndex(); return type == 1 || type == 5 || type == 7; }

    // juce::AudioProcessorValueTreeState &treeStateRef;

    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterBool* enabled = nullptr;

    ThreeBands threeBands;
    MBComp mbComp;
    MSComp msComp;
    StereoComp stereoComp;
    TypeAProcessor typeA;
    TransientShaper transient;
    MBTransientShaper mbTransient;
    Opto opto;
    MBOpto mbOpto;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dynamics)
};