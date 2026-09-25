#include "Fuzz.h"

//==============================================================================
Fuzz::Fuzz(juce::AudioProcessorValueTreeState& treeState, SlotId slot)
	: MacroEffect(treeState, slot),
	  bias(getParam(ParamIDs::grillBias)),
	  timing(getParam(ParamIDs::grillDcTiming)),
	  follower(false) {}

Fuzz::~Fuzz() {}

void Fuzz::prepare(juce::dsp::ProcessSpec& spec) {
	follower.prepare(spec);
	bias.prepare(spec);

	appliedTiming = -1.0f;
}

void Fuzz::processBlock(juce::dsp::AudioBlock<float>& block) {
	#if PERFETTO
	// TRACE_EVENT("dsp", "Fuzz::processBlock");
	#endif // PERFETTO

	bias.update();

	if (const auto ms = timing.getRaw(); ms != appliedTiming) {
		follower.setAttackTime(ms);
		follower.setReleaseTime(ms);
		appliedTiming = ms;
	}


	for (int sample = 0; sample < static_cast<int>(block.getNumSamples()); sample++) {
		auto envelope = 0.0f;

		auto channels = block.getNumChannels();

		if (channels == 1)
			envelope = follower.processSample(block.getSample(0, sample));
		else if (channels == 2) {
			envelope = follower.processSampleStereo(block.getSample(0, sample), block.getSample(1, sample));
		}
		
		for (int channel = 0; channel < channels; channel++) {
			float biasAmt = bias.getNextValue(channel) * 3.0f;
			float x = block.getSample(channel, sample) + envelope * biasAmt;
			block.setSample(channel, sample, x);
		}
	}
}