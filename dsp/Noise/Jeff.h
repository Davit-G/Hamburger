/*
  ==============================================================================

    Jeff.h
    Created: 13 Jun 2021 5:57:58pm
    Author:  DavZ

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../SmoothParam.h"

//==============================================================================
/*
*/
class Jeff
{
public:
	Jeff(juce::AudioProcessorValueTreeState& treeState);
	~Jeff();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
	SmoothParam amount;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Jeff)
};
