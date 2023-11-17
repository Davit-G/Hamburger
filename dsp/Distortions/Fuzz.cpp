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
Fuzz::Fuzz(juce::AudioProcessorValueTreeState& treeState) : amount(treeState, "fuzz"), follower(false) {}

Fuzz::~Fuzz() {}


void Fuzz::prepareToPlay(double sampleRate, int samplesPerBlock) {
	amount.prepareToPlay(sampleRate, samplesPerBlock);
	follower.prepareToPlay(sampleRate, samplesPerBlock);
	follower.setAttackTime(50);
	follower.setReleaseTime(50);
}

void Fuzz::processBlock(dsp::AudioBlock<float>& block) {
	amount.update();

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		float next = amount.getNextValue() + 1;

		auto envelope = follower.processSampleStereo(block.getSample(0, sample), block.getSample(1, sample));
		
		float x = block.getSample(0, sample) + envelope * 1.5;
		block.setSample(0, sample, x / 1 + fabs(x));

		x = block.getSample(1, sample) + envelope * 1.5;
		block.setSample(1, sample, x / 1 + fabs(x));
	}
}