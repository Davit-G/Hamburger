#pragma once

#include "../SmoothParam.h"
#include "../WaveShapers.h"
#include "../../utils/Params.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

//==============================================================================
/*
 */
class PhaseDist
{
public:
    PhaseDist(juce::AudioProcessorValueTreeState& treeState);
    ~PhaseDist() {}

    void processBlock(juce::dsp::AudioBlock<float> &block) noexcept;
    void prepare(juce::dsp::ProcessSpec& spec) noexcept;

private:
    SmoothParam amount;
    SmoothParam tone;
    SmoothParam stereo;
    SmoothParam rectify;

    float sampleRate;
    float sampleRateMult;

    // delay line for phase distortion effect
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;

    // filter to remove harsh freqs from phase distorted signal
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> filter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseDist)
};
