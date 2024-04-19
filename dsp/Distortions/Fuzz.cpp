#include "Fuzz.h"

//==============================================================================
Fuzz::Fuzz(juce::AudioProcessorValueTreeState& treeState) : bias(treeState, ParamIDs::grillBias), follower(false) {}

Fuzz::~Fuzz() {}

void Fuzz::prepare(dsp::ProcessSpec& spec) {
	follower.prepare(spec);
	bias.prepare(spec);
	follower.setAttackTime(50);
	follower.setReleaseTime(50);
}

void Fuzz::processBlock(dsp::AudioBlock<float>& block) {
	TRACE_EVENT("dsp", "Fuzz::processBlock");
	bias.update();


	for (int sample = 0; sample < static_cast<int>(block.getNumSamples()); sample++) {
		auto envelope = 0.0f;

		auto channels = block.getNumChannels();

		if (channels == 1)
			envelope = follower.processSample(block.getSample(0, sample));
		else if (channels == 2) {
			envelope = follower.processSampleStereo(block.getSample(0, sample), block.getSample(1, sample));
		}

		float biasAmt = bias.getNextValue() * 3.0f;

		for (int channel = 0; channel < channels; channel++) {
			float x = block.getSample(channel, sample) + envelope * biasAmt;
			block.setSample(channel, sample, x / (1 + fabs(x)));
		}
	}
}