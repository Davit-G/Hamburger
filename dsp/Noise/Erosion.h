#pragma once

#include "../../utils/Params.h"

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include <melatonin_perfetto/melatonin_perfetto.h>
#include "../SmoothParam.h"
//==============================================================================
/*
 */
class Erosion
{
public:
    Erosion(juce::AudioProcessorValueTreeState& treeState) : delayLine(200),
        erosionAmount(treeState, ParamIDs::erosionAmount),
        erosionFrequency(treeState, ParamIDs::noiseFrequency),
        erosionQ(treeState, ParamIDs::noiseQ) {}
    
    ~Erosion() {}

    void processBlock(juce::dsp::AudioBlock<float>& block);
    void prepare(juce::dsp::ProcessSpec& spec);

private:
    SmoothParam erosionAmount;
    SmoothParam erosionFrequency;
    SmoothParam erosionQ;

    // stereo delay line
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;

    // iir filter
    // juce::dsp::IIR::Filter<float> iirFilter;
    juce::dsp::IIR::Filter<float> iirFilter;

    // noise generator
    juce::Random random;

    float oldSampleRate;
    float oldSamplesPerBlock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Erosion)
};