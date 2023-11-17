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
Sizzle::Sizzle(juce::AudioParameterFloat* param)
:
envelopeDetector(false)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.

	knobValue = param;
    jassert(knobValue);
}

Sizzle::~Sizzle()
{
}


void Sizzle::prepareToPlay(double sampleRate, int samplesPerBlock) {
	smoothedInput.reset(sampleRate, 0.07);
	smoothedInput.setCurrentAndTargetValue(0.0);

	envelopeDetector.prepareToPlay(sampleRate, samplesPerBlock);
	envelopeDetector.setAttackTime(10);
	envelopeDetector.setReleaseTime(10);
}

void Sizzle::processBlock(dsp::AudioBlock<float>& block) {
	if (knobValue == nullptr) return;
	smoothedInput.setTargetValue(knobValue->get()*0.01f);
	
	if (knobValue->get() == 0) return;

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		float nextSizzle = smoothedInput.getNextValue();

		float envelope = envelopeDetector.processSampleStereo(leftDryData[sample], rightDryData[sample]);

		rightDryData[sample] = newSizzleFunction(rightDryData[sample], nextSizzle, envelope);
		leftDryData[sample] = newSizzleFunction(leftDryData[sample], nextSizzle, envelope);

	}
}