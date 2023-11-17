/*
  ==============================================================================

    Redux.h
    Created: 8 Dec 2021 5:01:30pm
    Author:  DavZ

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../SmoothParam.h"

class Redux : public Component
{
public:
    Redux(juce::AudioProcessorValueTreeState& treeState);
    ~Redux();

    void processBlock(dsp::AudioBlock<float>& block);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    SmoothParam downsample;
    SmoothParam jitter;
    SmoothParam bitReduction;

    float jitterOffsetL = 0.0f;
    float jitterOffsetR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Redux)
};
