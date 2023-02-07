/*
  ==============================================================================

    Sizzle.cpp
    Created: 13 Jun 2021 6:32:32pm
    Author:  DavZ

  ==============================================================================
*/
#include <JuceHeader.h>
#include "Sizzle.h"

//==============================================================================
Sizzle::Sizzle(juce::AudioProcessorValueTreeState &state)
:
treeStateRef(state), envelopeDetector(false)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.

	// initialise knob 
	knobValue = dynamic_cast<juce::AudioParameterFloat *>(treeStateRef.getParameter("sizzle"));
	jassert(knobValue); // makes sure it exists
}

Sizzle::~Sizzle()
{
}


void Sizzle::prepareToPlay(double sampleRate, int samplesPerBlock) {
	smoothedInput.reset(44100, 0.07);
	smoothedInput.setCurrentAndTargetValue(0.0);

	envelopeDetector.prepareToPlay(sampleRate, samplesPerBlock);
	envelopeDetector.setAttackTime(10);
	envelopeDetector.setReleaseTime(10);
}

void Sizzle::processBlock(AudioBuffer<float>& dryBuffer) {
	if (knobValue == nullptr) return;
	smoothedInput.setTargetValue(knobValue->get()*0.01f);
	
	if (knobValue->get() == 0) return;

	auto rightDryData = dryBuffer.getWritePointer(1);
	auto leftDryData = dryBuffer.getWritePointer(0);

	for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++) {
		float nextSizzle = smoothedInput.getNextValue();

		float envelope = envelopeDetector.processSampleStereo(leftDryData[sample], rightDryData[sample]);

		rightDryData[sample] = newSizzleFunction(rightDryData[sample], nextSizzle, envelope);
		leftDryData[sample] = newSizzleFunction(leftDryData[sample], nextSizzle, envelope);

	}
}