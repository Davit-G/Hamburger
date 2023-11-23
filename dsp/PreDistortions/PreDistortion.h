#pragma once

#include <JuceHeader.h>

// #include "./AllPassChain.h"
// #include "./SIMDAllPassChain.h"
#include "SVFAllPassChain.h"

class PreDistortion
{
public:
    PreDistortion(juce::AudioProcessorValueTreeState &state) {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("preDistortionType")); jassert(distoType);
        preDistortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("preDistortionEnabled")); jassert(preDistortionEnabled);

        // allPassChain = std::make_unique<AllPassChain>(state);
        svfAllPass = std::make_unique<SVFAllPassChain>(state);
        // simdAllPass = std::make_unique<SIMDAllPassChain>(state);
    }
    ~PreDistortion() {}

    void processBlock(dsp::AudioBlock<float>& block) {
        int distoTypeIndex = distoType->getIndex();

        

        if (preDistortionEnabled->get() == false) return;

        switch (distoTypeIndex)
        {
        case 0: // AllPass Chain
            // allPassChain->processBlock(block);
            // simdAllPass->processBlock(block);
            svfAllPass->processBlock(block);
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        default:
            break;
        }
    }

    void prepare(dsp::ProcessSpec& spec) {
        // allPassChain->prepare(spec);
        svfAllPass->prepare(spec);
        // simdAllPass->prepare(spec);
    }

    void setSampleRate(float newSampleRate) { 
        sampleRate = newSampleRate;
    }

private:
    // juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    // std::unique_ptr<AllPassChain> allPassChain;
    std::unique_ptr<SVFAllPassChain> svfAllPass;
    // std::unique_ptr<SIMDAllPassChain> simdAllPass;

    juce::AudioParameterBool* preDistortionEnabled = nullptr;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreDistortion)
};