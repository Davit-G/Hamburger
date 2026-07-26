#pragma once

#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "../../SmoothParam.h"
#include "../../../utils/Params.h"

class NonlinSlew {
public:
    NonlinSlew(juce::AudioProcessorValueTreeState& treeState) 
    : alphaParam(treeState, ParamIDs::alphaParam),
    slewSpeed(treeState, ParamIDs::slewSpeed),
    directionality(treeState, ParamIDs::directionality),
    type(nullptr),
    lastMode(-1)
    {
        type = dynamic_cast<juce::AudioParameterInt *>(treeState.getParameter(ParamIDs::slewType.getParamID()));
    };

    ~NonlinSlew() {

    }

    void prepare(juce::dsp::ProcessSpec& spec);
    void processBlock(juce::dsp::AudioBlock<float> &block);

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