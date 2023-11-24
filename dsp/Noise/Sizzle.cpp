/*
  ==============================================================================

    Sizzle.cpp
    Created: 13 Jun 2021 6:32:32pm
    Author:  DavZ

  ==============================================================================
*/
 
#include "Sizzle.h"

//==============================================================================
Sizzle::Sizzle(juce::AudioProcessorValueTreeState& treeState)
:
envelopeDetector(false),
noiseAmount(treeState, "noiseAmount") {}

Sizzle::~Sizzle()
{
}


void Sizzle::prepare(dsp::ProcessSpec& spec) {
	noiseAmount.prepare(spec);

	envelopeDetector.prepare(spec);
	envelopeDetector.setAttackTime(10);
	envelopeDetector.setReleaseTime(10);
}

void Sizzle::processBlock(dsp::AudioBlock<float>& block) {
	TRACE_EVENT("dsp", "Sizzle::processBlock");
	noiseAmount.update();
	if (noiseAmount.getRaw() == 0) return;

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		float nextSizzle = noiseAmount.getNextValue() * 0.01f;

		float envelope = juce::Decibels::decibelsToGain(envelopeDetector.processSampleStereo(leftDryData[sample], rightDryData[sample]));

		rightDryData[sample] = newSizzleFunction(rightDryData[sample], nextSizzle, envelope);
		leftDryData[sample] = newSizzleFunction(leftDryData[sample], nextSizzle, envelope);

	}
}