#pragma once

#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "../../SmoothParam.h"
#include "../../../utils/Params.h"

class NonlinSlew {
public:
    NonlinSlew(juce::AudioProcessorValueTreeState& treeState) 
    : emaParam(treeState, ParamIDs::emaParam), 
    alphaParam(treeState, ParamIDs::alphaParam),
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

    SmoothParam emaParam;
    SmoothParam alphaParam;
    SmoothParam slewSpeed;
    SmoothParam directionality;
    juce::AudioParameterInt* type;

    std::vector<float> lastSampleBuf {};
    std::vector<float> emaBuf {}; // exponential moving average

    int lastMode;
};