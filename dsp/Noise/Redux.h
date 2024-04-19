#pragma once

#include "../../utils/Params.h"

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

#include "../SmoothParam.h"

class Redux
{
public:
    Redux(juce::AudioProcessorValueTreeState& treeState);
    ~Redux();

    void processBlock(juce::dsp::AudioBlock<float>& block);
    void prepare(juce::dsp::ProcessSpec& spec);
    void antiAliasingStep(juce::dsp::AudioBlock<float>& block);

private:
    SmoothParam downsample;
    SmoothParam downsampleMix;
    SmoothParam bitReduction;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> antialiasingFilter[4];

    float heldSampleL = 0.0f;
    float heldSampleR = 0.0f;

    float oldDownsample = 1.0f;

    double sampleRate = 44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Redux)
};
