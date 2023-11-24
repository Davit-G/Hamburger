#pragma once

 

#include "./AllPassChain.h"   // might want to reintroduce with "quality" setting
// #include "./SIMDAllPassChain.h"
#include "SVFAllPassChain.h"

class PreDistortion
{
public:
    PreDistortion(juce::AudioProcessorValueTreeState &state) {
        quality = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("qualityFactor")); jassert(distoType);
        preDistortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("preDistortionEnabled")); jassert(preDistortionEnabled);

        allPassChain = std::make_unique<AllPassChain>(state);
        svfAllPass = std::make_unique<SVFAllPassChain>(state);
        // simdAllPass = std::make_unique<SIMDAllPassChain>(state);
    }
    ~PreDistortion() {}

    void processBlock(dsp::AudioBlock<float>& block) {
        int qualitySetting = quality->getIndex();

        if (preDistortionEnabled->get() == false) return;

        // todo: replace with enum?
        if (qualitySetting == 0) {
            // allPassChain->processBlock(block);
        }
        else if (qualitySetting == 1) {
            svfAllPass->processBlock(block);
        }   
    }

    void prepare(dsp::ProcessSpec& spec) {
        allPassChain->prepare(spec);
        svfAllPass->prepare(spec);
        // simdAllPass->prepare(spec);
    }

    void setSampleRate(float newSampleRate) { 
        sampleRate = newSampleRate;
    }

private:
    // juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *quality = nullptr;
    
    std::unique_ptr<SVFAllPassChain> svfAllPass = nullptr;
    std::unique_ptr<AllPassChain> allPassChain = nullptr;

    juce::AudioParameterBool* preDistortionEnabled = nullptr;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreDistortion)
};