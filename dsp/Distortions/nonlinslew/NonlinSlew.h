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
      type(getParam(ParamIDs::slewType)),
      lastMode(-1)
    {}

    ~NonlinSlew() {

    }

    void prepare(juce::dsp::ProcessSpec& spec) override;
    void processBlock(juce::dsp::AudioBlock<float> &block) override;

private:
    void resetState();

    SmoothParam alphaParam;
    SmoothParam slewSpeed;
    SmoothParam directionality;
    MacroParam& type; // each slot's own, they sound different

    std::vector<float> lastSampleBuf {};
    std::vector<float> emaBuf {}; // exponential moving average

    // the input's and output's power, followed to keep the output at the input's level
    std::vector<float> inPower {}, outPower {};
    float levelCoefficient = 0.0f;

    float sampleRate = 44100.0f;

    int lastMode;
};