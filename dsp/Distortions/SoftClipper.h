#pragma once

#include <JuceHeader.h>

#include "../SmoothParam.h"

class SoftClip
{
public:
    SoftClip(juce::AudioProcessorValueTreeState& treeState);

    ~SoftClip();

    void processBlock(dsp::AudioBlock<float>& block);
    void legacyProcessBlock(dsp::AudioBlock<float>& block);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    float softClipper(float input, float threshold, float knee, float ratio);


private:
    SmoothParam saturationKnob;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftClip)
};