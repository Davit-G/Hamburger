#pragma once

#include "../SmoothParam.h"
#include "WaveShapers.h"

//==============================================================================
/*
 */
class PhaseDist
{
public:
    PhaseDist(juce::AudioProcessorValueTreeState& treeState);
    ~PhaseDist() {}

    void processBlock(dsp::AudioBlock<float> &block);
    void prepare(dsp::ProcessSpec& spec);

private:
    SmoothParam amount;
    SmoothParam tone;
    SmoothParam normalise;

    float sampleRate;

    // delay line for phase distortion effect
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;

    // filter to remove harsh freqs from phase distorted signal
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> filter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseDist)
};
