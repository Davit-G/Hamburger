/*
  ==============================================================================

    Patty.h
    Created: 13 Jun 2021 6:45:25pm
    Author:  DavZ

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../SmoothParam.h"

class Fuzz
{
public:
	Fuzz(juce::AudioProcessorValueTreeState& treeState);
	~Fuzz();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
	SmoothParam amount;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fuzz)
};
