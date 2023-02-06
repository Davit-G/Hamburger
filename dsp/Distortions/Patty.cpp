/*
  ==============================================================================

    Patty.cpp
    Created: 13 Jun 2021 6:45:25pm
    Author:  DavZ

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Patty.h"

//==============================================================================
Patty::Patty(juce::AudioProcessorValueTreeState &state) 
	: treeStateRef(state)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.

	// initialise knob D
	knobValue = dynamic_cast<juce::AudioParameterFloat *>(treeStateRef.getParameter("fuzz"));
    jassert(knobValue); // makes sure it exists
}

Patty::~Patty()
{
}


void Patty::prepareToPlay(double sampleRate, int samplesPerBlock) {
	smoothedInput.reset(44100, 0.07);
	smoothedInput.setCurrentAndTargetValue(0.0);
}

void Patty::processBlock(AudioBuffer<float>& dryBuffer) {
	smoothedInput.setTargetValue(knobValue->get() * 0.01f);
	auto rightDryData = dryBuffer.getWritePointer(1);
	auto leftDryData = dryBuffer.getWritePointer(0);

	for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++) {
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