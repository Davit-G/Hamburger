#pragma once

#include "SVFAllPassChain.h"
#include "Grunge.h"
#include "../../utils/Params.h"

#include "../EffectBase.h"

class PreDistortion : public EffectBase
{
public:
    PreDistortion(juce::AudioProcessorValueTreeState &state) : treeStateRef(state) {
        type = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter(SlotId{ModuleId::module2, 0}.type().getParamID())); jassert(type);
        preDistortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter(SlotId{ModuleId::module2, 0}.enabled().getParamID())); jassert(preDistortionEnabled);

        svfAllPass = std::make_unique<SVFAllPassChain>(state);
        grungeDSP = std::make_unique<Grunge>(state);
    }
    ~PreDistortion() {}


    void processBlock(juce::dsp::AudioBlock<float>& block) override {
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

    void prepare(juce::dsp::ProcessSpec& spec) override {
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