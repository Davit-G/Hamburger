#pragma once
#include "../SmoothParam.h"

class PostClip
{
public:
    PostClip(juce::AudioProcessorValueTreeState& treeState);

    ~PostClip();

    void processBlock(dsp::AudioBlock<float>& block);
    void prepare(dsp::ProcessSpec& spec);

private:
    SmoothParam gainKnob;
    SmoothParam kneeKnob;

    juce::AudioParameterBool *clipEnabled;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PostClip)
};