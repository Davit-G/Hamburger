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
	Redux();
	~Redux();

	void processBlock(float bitReductionValue, float downSampleValue, float reduxStereoValue, AudioBuffer<float>& dryBuffer);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Redux)
};
