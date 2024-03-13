#pragma once

#include "WaveShapers.h"
#include "../SmoothParam.h"

class SoftClip
{
public:
    SoftClip(juce::AudioProcessorValueTreeState& treeState);

    ~SoftClip();

    void processBlock(dsp::AudioBlock<float>& block);
    void prepare(dsp::ProcessSpec& spec);
private:
    SmoothParam saturationKnob;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftClip)
};