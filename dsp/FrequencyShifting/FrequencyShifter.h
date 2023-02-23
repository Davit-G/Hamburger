/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "HilbertShifter.h"


class FrequencyShifter
{
public:
    
    FrequencyShifter() {};
    ~FrequencyShifter() {};

    
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    void processBlock (dsp::AudioBlock<float>& block);

    void setDryWetMix(float newMixValue);
    void setFrequencyShift(float newFrequencyShift);
    
    void setLFORate(float newRate);
    void setLFODepth(float newDepth);
    

private:
    //==============================================================================
    
    juce::AudioBuffer<float> processedBuffer;
    
    float m_dryWetMix { 1.f };
    
    HilbertShifter m_hilberFreqShifterL;
    HilbertShifter m_hilberFreqShifterR;

    float m_lowpassCutoff { 22000.f };

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> iirFilter;

    const int numStereoChannels { 2 };

    dsp::Oscillator<float> m_frequencyLFO;
    
    float m_lfoDepth { 1.f };
    float m_freqShift { 0.f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyShifter)
};
