/*
  ==============================================================================

    Patty.cpp
    Created: 13 Jun 2021 6:45:25pm
    Author:  DavZ

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Fuzz.h"

//==============================================================================
Fuzz::Fuzz(juce::AudioParameterFloat* param)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.
	knobValue = param;
    jassert(knobValue);
}

Fuzz::~Fuzz()
{
}


void Fuzz::prepareToPlay(double sampleRate, int samplesPerBlock) {
	smoothedInput.reset(44100, 0.07);
	smoothedInput.setCurrentAndTargetValue(0.0);
}

void Fuzz::processBlock(dsp::AudioBlock<float>& block) {
	if (knobValue == nullptr) return;
	smoothedInput.setTargetValue(knobValue->get() * 0.01f);

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		float nextPatty = 1.f - smoothedInput.getNextValue();

		
		auto x = rightDryData[sample];
		if (x > 0) {
			rightDryData[sample] = powf(x, nextPatty);
		}
		else if (x < 0) {
			rightDryData[sample] = -powf(-x, nextPatty);
		}
		else {
			rightDryData[sample] = 0.f;
		}

		x = leftDryData[sample];
		if (x > 0) {
			leftDryData[sample] = powf(x, nextPatty);
		}
		else if (x < 0) {
			leftDryData[sample] = -powf(-x, nextPatty);
		}
		else {
			leftDryData[sample] = 0.f;
		}
	}
}