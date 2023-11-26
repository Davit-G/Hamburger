#include "Fuzz.h"

//==============================================================================
Fuzz::Fuzz(juce::AudioProcessorValueTreeState& treeState) : bias(treeState, "bias"), follower(false) {}

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
		auto envelope = follower.processSampleStereo(block.getSample(0, sample), block.getSample(1, sample));

		float biasAmt = bias.get() * 3.0f;
		
		float x = block.getSample(0, sample) + envelope * biasAmt;
		block.setSample(0, sample, x / (1 + fabs(x)));

		x = block.getSample(1, sample) + envelope * biasAmt;
		block.setSample(1, sample, x / (1 + fabs(x)));
	}
}