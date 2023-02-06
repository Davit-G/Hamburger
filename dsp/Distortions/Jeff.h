/*
  ==============================================================================

    Jeff.h
    Created: 13 Jun 2021 5:57:58pm
    Author:  DavZ

  ==============================================================================
*/

#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class Jeff : public Component
{
public:
	Jeff();
	~Jeff();

	void processBlock(float input, AudioBuffer<float>& dryBuffer);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
	SmoothedValue<float> smoothedInput;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Jeff)
};
