/*
  ==============================================================================

    Cooked.h
    Created: 13 Jun 2021 5:13:49pm
    Author:  DavZ

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
 */
class Cooked
{
public:
    Cooked(juce::AudioParameterFloat* param);
    ~Cooked();

    void processBlock(dsp::AudioBlock<float> &block);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    juce::AudioParameterFloat *knobValue = nullptr;
    SmoothedValue<float> smoothedInput;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Cooked)
};
