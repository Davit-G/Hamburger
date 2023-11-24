#pragma once

 
#include "../SmoothParam.h"

//==============================================================================
/*
 */
class HardClip
{
public:
    HardClip(juce::AudioProcessorValueTreeState& treeState);
    ~HardClip();

    void processBlock(dsp::AudioBlock<float>& block);
    void prepare(dsp::ProcessSpec& spec);
private:
    SmoothParam gain;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardClip)
};