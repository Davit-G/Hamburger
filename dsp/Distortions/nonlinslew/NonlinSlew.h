#pragma once

#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "../../SmoothParam.h"
#include "../../../utils/Params.h"
#include "../../EffectBase.h"

class NonlinSlew : public MacroEffect {
public:
    NonlinSlew(juce::AudioProcessorValueTreeState& treeState, SlotId slot)
    : MacroEffect(treeState, slot),
      alphaParam(getParam(ParamIDs::alphaParam)),
      slewSpeed(getParam(ParamIDs::slewSpeed)),
      directionality(getParam(ParamIDs::directionality)),
      type(nullptr),
      lastMode(-1)
    {
        type = dynamic_cast<juce::AudioParameterInt *>(treeState.getParameter(ParamIDs::slewType.id));
    };

    ~NonlinSlew() {

    }

    void prepare(juce::dsp::ProcessSpec& spec) override;
    void processBlock(juce::dsp::AudioBlock<float> &block) override;

private:
    void resetState();

    SmoothParam alphaParam;
    SmoothParam slewSpeed;
    SmoothParam directionality;
    juce::AudioParameterInt* type;

    std::vector<float> lastSampleBuf {};
    std::vector<float> emaBuf {}; // exponential moving average

    float sampleRateMultiplier = 1.0f; // sample rate mult from 44100
    float sampleRateMultInv = 1.0f; // inverse of above

    int lastMode;
};