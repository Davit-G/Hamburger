/*
  ==============================================================================

    Patty.h
    Created: 13 Jun 2021 6:45:25pm
    Author:  DavZ

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


//==============================================================================
/*
*/
class Patty : public Component
{
public:
	Patty(juce::AudioProcessorValueTreeState &state);
	~Patty();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
	juce::AudioProcessorValueTreeState &treeStateRef;
	juce::AudioParameterFloat *knobValue = nullptr;

	SmoothedValue<float> smoothedInput;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Patty)
};
