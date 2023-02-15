#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
 */
class SoftClip
{
public:
    SoftClip(juce::AudioProcessorValueTreeState &state);
    ~SoftClip();

    void processBlock(dsp::AudioBlock<float>& block);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    juce::AudioProcessorValueTreeState &treeStateRef;

    juce::AudioParameterFloat *saturationKnob = nullptr;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftClip)
};