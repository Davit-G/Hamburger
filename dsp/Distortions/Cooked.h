#pragma once

#include <JuceHeader.h>
#include "../SmoothParam.h"

//==============================================================================
/*
 */
class Cooked
{
public:
    Cooked(juce::AudioProcessorValueTreeState& treeState);
    ~Cooked();

    void processBlock(dsp::AudioBlock<float> &block);
    void prepare(dsp::ProcessSpec& spec);

private:
    SmoothParam amount;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Cooked)
};
