#pragma once
#include "../SmoothParam.h"

#include <melatonin_perfetto/melatonin_perfetto.h>
#include "../../utils/Params.h"

class PostClip
{
public:
    PostClip(juce::AudioProcessorValueTreeState& treeState);

    ~PostClip();

    void processBlock(juce::dsp::AudioBlock<float>& block);
    void prepare(juce::dsp::ProcessSpec& spec);

private:
    SmoothParam gainKnob;
    SmoothParam kneeKnob;

    juce::AudioParameterBool *clipEnabled;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PostClip)
};