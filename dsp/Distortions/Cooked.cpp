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
Cooked::Cooked(juce::AudioProcessorValueTreeState &state) : treeStateRef(state)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.

	// initialise knob 
	knobValue = dynamic_cast<juce::AudioParameterFloat *>(treeStateRef.getParameter("fold"));
	jassert(knobValue); // makes sure it exists
}

Cooked::~Cooked()
{
}


void Cooked::prepareToPlay(double sampleRate, int samplesPerBlock) {
	smoothedInput.reset(44100, 0.07);
	smoothedInput.setCurrentAndTargetValue(0.0);
}

void Cooked::processBlock(AudioBuffer<float>& dryBuffer) {
	smoothedInput.setTargetValue(knobValue->get() * 0.01f);
	auto rightDryData = dryBuffer.getWritePointer(1);
	auto leftDryData = dryBuffer.getWritePointer(0);

	for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++) {
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