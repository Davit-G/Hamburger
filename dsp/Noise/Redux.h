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
    void prepare(dsp::ProcessSpec& spec);
    void antiAliasingStep(dsp::AudioBlock<float>& block);

private:
    SmoothParam downsample;
    SmoothParam jitter;
    SmoothParam bitReduction;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> antialiasingFilter[4];

    float jitterOffsetL = 0.0f;
    float jitterOffsetR = 0.0f;

    float heldSampleL = 0.0f;
    float heldSampleR = 0.0f;

    float oldDownsample = 1.0f;

    double sampleRate = 44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Redux)
};
