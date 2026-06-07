#pragma once

#include "SVFAllPassChain.h"
#include "Grunge.h"
#include "../../utils/Params.h"

class PreDistortion
{
public:
    PreDistortion(juce::AudioProcessorValueTreeState &state) : treeStateRef(state) {
        type = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter(ParamIDs::preDistortionType.getParamID())); jassert(type);
        preDistortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter(ParamIDs::preDistortionEnabled.getParamID())); jassert(preDistortionEnabled);

        svfAllPass = std::make_unique<SVFAllPassChain>(state);
        grungeDSP = std::make_unique<Grunge>(state);
    }
    ~PreDistortion() {}

    void processBlock(juce::dsp::AudioBlock<float>& block) {
        int typeSetting = type->getIndex();
        
        if (preDistortionEnabled->get() == false)
            return;

        switch (typeSetting) {
            case 0:
            {
                svfAllPass->processBlock(block);
                break;
            }
            case 1:
            {
                grungeDSP->processBlock(block);
                break;
            }
        }
    }

    void prepare(juce::dsp::ProcessSpec& spec) {
        svfAllPass->prepare(spec);
        grungeDSP->prepare(spec);
    }

    void setSampleRate(float newSampleRate) { 
        sampleRate = newSampleRate;
    }

private:
    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *type = nullptr;
    
    std::unique_ptr<SVFAllPassChain> svfAllPass = nullptr;
    std::unique_ptr<Grunge> grungeDSP = nullptr;

    juce::AudioParameterBool* preDistortionEnabled = nullptr;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreDistortion)
};