#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
 */
class Erosion
{
public:
    Erosion(juce::AudioParameterFloat* param1, juce::AudioParameterFloat* param2, juce::AudioParameterFloat* param3) : delayLine(200) {
        erosionAmount = param1;
        jassert(erosionAmount);
        erosionFrequency = param2;
        jassert(erosionAmount);
        erosionQ = param3;
        jassert(erosionAmount);
    }
    ~Erosion() {

    };

    void processBlock(dsp::AudioBlock<float>& block, double sampleRate);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    juce::AudioParameterFloat *erosionAmount = nullptr;
    juce::AudioParameterFloat *erosionFrequency = nullptr;
    juce::AudioParameterFloat *erosionQ = nullptr;

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