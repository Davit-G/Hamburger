/*
  ==============================================================================

    Cooked.cpp
    Created: 13 Jun 2021 5:13:49pm
    Author:  DavZ

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Cooked.h"

//==============================================================================
Cooked::Cooked(juce::AudioParameterFloat* param)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.
	knobValue = param;
    jassert(knobValue);
}

Cooked::~Cooked()
{
}


void Cooked::prepareToPlay(double sampleRate, int samplesPerBlock) {
	smoothedInput.reset(sampleRate, 0.07);
	smoothedInput.setCurrentAndTargetValue(0.0);
}

void Cooked::processBlock(dsp::AudioBlock<float>& block) {
	smoothedInput.setTargetValue(knobValue->get() * 0.01f);
	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		float nextCooked = smoothedInput.getNextValue();

		//the original waveshaping thing

		if (nextCooked != 0.f) {
			auto x = rightDryData[sample] * (nextCooked * 20 + 1);
			rightDryData[sample] = (4 * (abs(0.25*x + 0.25 - round(0.25*x + 0.25)) - 0.25));

			x = leftDryData[sample] * (nextCooked * 20 + 1);
			leftDryData[sample] = (4 * (abs(0.25*x + 0.25 - round(0.25*x + 0.25)) - 0.25));
		}
	}
}