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
    
    ~Erosion() {};

    void processBlock(dsp::AudioBlock<float>& block, double sampleRate);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    SmoothParam erosionAmount;
    SmoothParam erosionFrequency;
    SmoothParam erosionQ;

    // stereo delay line
    dsp::DelayLine<float, dsp::DelayLineInterpolationTypes::Linear> delayLine;

    // iir filter
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> iirFilter;

    // noise generator
    juce::Random random;

    // random noise buffer
    AudioBuffer<float> randomBuffer;
    dsp::AudioBlock<float> randomBlock;

    double oldSampleRate;
    double oldSamplesPerBlock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Erosion)
};