/*
  ==============================================================================

    Jeff.cpp
    Created: 13 Jun 2021 5:57:58pm
    Author:  DavZ

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Jeff.h"

//==============================================================================
Jeff::Jeff(juce::AudioParameterFloat* param)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.
	saturationAmount = param;
    jassert(saturationAmount);
}

Jeff::~Jeff()
{
}


void Jeff::prepareToPlay(double sampleRate, int samplesPerBlock) {
	smoothedInput.reset(44100, 0.07);
	smoothedInput.setCurrentAndTargetValue(0.0);
}

void Jeff::processBlock(dsp::AudioBlock<float>& block) {

	// smoothedInput.setTargetValue(input);
	float nextJeff = saturationAmount->get() * 0.01f;

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		// float nextJeff = smoothedInput.getNextValue();

		auto x = rightDryData[sample];
		rightDryData[sample] = x + (x*nextJeff*sin(x * 2 * nextJeff * 20 * 3.14)) / 5;

		x = leftDryData[sample];
		leftDryData[sample] = x + (x*nextJeff*sin(x * 2 * nextJeff * 20 * 3.14)) / 5;
	}
}