#pragma once

#include <JuceHeader.h>

#include "../SmoothParam.h"
//==============================================================================
/*
 */
class Erosion
{
public:
    Erosion(juce::AudioProcessorValueTreeState& treeState) : delayLine(200),
        erosionAmount(treeState, "noiseAmount"),
        erosionFrequency(treeState, "noiseFrequency"),
        erosionQ(treeState, "noiseQ") {}
    
    ~Erosion() {}

    void processBlock(dsp::AudioBlock<float>& block);
    void prepare(dsp::ProcessSpec& spec);

private:
    SmoothParam erosionAmount;
    SmoothParam erosionFrequency;
    SmoothParam erosionQ;

    // stereo delay line
    dsp::DelayLine<float, dsp::DelayLineInterpolationTypes::Linear> delayLine;

    // iir filter
    // juce::dsp::IIR::Filter<float> iirFilter;
    dsp::IIR::Filter<float> iirFilter;

    // noise generator
    juce::Random random;

    float oldSampleRate;
    float oldSamplesPerBlock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Erosion)
};