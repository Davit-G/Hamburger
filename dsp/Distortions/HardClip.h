#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
 */
class HardClip
{
public:
    HardClip(juce::AudioParameterFloat* param);
    ~HardClip();

    void processBlock(dsp::AudioBlock<float>& block);
    void prepareToPlay(double sampleRate, int samplesPerBlock);
private:

    juce::AudioParameterFloat *saturationKnob = nullptr;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardClip)
};