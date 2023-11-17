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
Fuzz::Fuzz(juce::AudioProcessorValueTreeState& treeState) : bias(treeState, "bias"), follower(false) {}

Fuzz::~Fuzz() {}


void Fuzz::prepareToPlay(double sampleRate, int samplesPerBlock) {
	follower.prepareToPlay(sampleRate, samplesPerBlock);
	bias.prepareToPlay(sampleRate, samplesPerBlock);
	follower.setAttackTime(50);
	follower.setReleaseTime(50);
}

void Fuzz::processBlock(dsp::AudioBlock<float>& block) {
	bias.update();

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		auto envelope = follower.processSampleStereo(block.getSample(0, sample), block.getSample(1, sample));

		auto biasAmt = bias.get() * 1.4;
		
		float x = block.getSample(0, sample) + envelope * biasAmt;
		block.setSample(0, sample, x / (1 + fabs(x)));

		x = block.getSample(1, sample) + envelope * biasAmt;
		block.setSample(1, sample, x / (1 + fabs(x)));
	}
}