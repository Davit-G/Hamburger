#pragma once

#include <JuceHeader.h>


class RingMod
{
public:
    RingMod(){

    };
    ~RingMod(){};

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        
    };

    void processBlock(dsp::AudioBlock<float> &block)
    {
       
    };

    void setFrequencyShift(float newFrequencyShift)
    {
        freqShift = newFrequencyShift;
    };

private:
    dsp::Oscillator carrier;

    float freqShift;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RingMod)
};
