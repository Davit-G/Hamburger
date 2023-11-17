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
Fuzz::Fuzz(juce::AudioProcessorValueTreeState& treeState) : amount(treeState, "fuzz") {}

Fuzz::~Fuzz() {}


void Fuzz::prepareToPlay(double sampleRate, int samplesPerBlock) {
	amount.prepareToPlay(sampleRate, samplesPerBlock);
}

void Fuzz::processBlock(dsp::AudioBlock<float>& block) {
	amount.update();

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		float nextPatty = 1.f - (amount.getNextValue() * 0.01f);
		
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