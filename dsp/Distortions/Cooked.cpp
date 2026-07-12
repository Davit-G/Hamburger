#include "Cooked.h"

//==============================================================================
Cooked::Cooked(juce::AudioProcessorValueTreeState& treeState) :
amount(treeState, ParamIDs::fold)
// stages(treeState, ParamIDs::stages) 
{}

Cooked::~Cooked() {}

void Cooked::prepare(juce::dsp::ProcessSpec& spec) noexcept {
	amount.prepare(spec);
	// stages.prepare(spec);
	// stages.setTo(1.0f); // default to 1 stage
}

void Cooked::processBlock(juce::dsp::AudioBlock<float>& block) noexcept {
	#if PERFETTO
	// TRACE_EVENT("dsp", "Cooked::processBlock");
	#endif // PERFETTO
	
	if (amount.getRaw(0) == 0.f) {
		return;
	}
	
	amount.update();
	// stages.update();

	for (int channel = 0; channel < block.getNumChannels(); channel++) {
		for (int sample = 0; sample < block.getNumSamples(); sample++) {
			// float stagesClamped = std::max(1.0f, stages.getNextValue());
			float nextCooked = amount.getNextValue(channel) * 0.01f;

			auto dryData = block.getChannelPointer(channel);

			if (nextCooked != 0.f) {
				auto x = dryData[sample] * (nextCooked * 20.0f + 1.0f);
				dryData[sample] = (4.0f * (abs(0.25f*x + 0.25f - round(0.25f*x + 0.25f)) - 0.25f));
			}
		}
	}
};