/*
  ==============================================================================

    Redux.h
    Created: 8 Dec 2021 5:01:30pm
    Author:  DavZ

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class Redux : public Component
{
public:
    Redux(juce::AudioParameterFloat *downsampling, juce::AudioParameterFloat *bitReduction);
    ~Redux();

    void processBlock(dsp::AudioBlock<float>& block);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    juce::AudioParameterFloat *downsampling;
    juce::AudioParameterFloat *bitReduction;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Redux)
};
