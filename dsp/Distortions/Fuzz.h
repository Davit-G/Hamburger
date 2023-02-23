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
class Fuzz
{
public:
	Fuzz(juce::AudioParameterFloat* param);
	~Fuzz();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
	juce::AudioParameterFloat *knobValue = nullptr;

	SmoothedValue<float> smoothedInput;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fuzz)
};
