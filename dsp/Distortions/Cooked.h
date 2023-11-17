/*
  ==============================================================================

    Cooked.h
    Created: 13 Jun 2021 5:13:49pm
    Author:  DavZ

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../SmoothParam.h"

//==============================================================================
/*
 */
class Cooked
{
public:
    Cooked(juce::AudioProcessorValueTreeState& treeState);
    ~Cooked();

    void processBlock(dsp::AudioBlock<float> &block);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    SmoothParam amount;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Cooked)
};
