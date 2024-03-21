#pragma once

#include "SVFAllPassChain.h"
#include "Grunge.h"

class PreDistortion
{
public:
    PreDistortion(juce::AudioProcessorValueTreeState &state) {
        // quality = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("qualityFactor")); jassert(quality);
        preDistortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("preDistortionEnabled")); jassert(preDistortionEnabled);

        svfAllPass = std::make_unique<SVFAllPassChain>(state);
        grungeDSP = std::make_unique<Grunge>(state);
    }
    ~PreDistortion() {}

    void processBlock(dsp::AudioBlock<float>& block) {
        // int qualitySetting = quality->getIndex();

        if (preDistortionEnabled->get() == false) return;

        svfAllPass->processBlock(block);
        grungeDSP->processBlock(block);
    }

    void prepare(dsp::ProcessSpec& spec) {
        svfAllPass->prepare(spec);
        grungeDSP->prepare(spec);
    }

    void setSampleRate(float newSampleRate) { 
        sampleRate = newSampleRate;
    }

private:
    // juce::AudioProcessorValueTreeState &treeStateRef;
    // juce::AudioParameterChoice *quality = nullptr;
    
    std::unique_ptr<SVFAllPassChain> svfAllPass = nullptr;
    std::unique_ptr<Grunge> grungeDSP = nullptr;

    juce::AudioParameterBool* preDistortionEnabled = nullptr;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreDistortion)
};