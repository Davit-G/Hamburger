/*
  ==============================================================================

    Cooked.cpp
    Created: 13 Jun 2021 5:13:49pm
    Author:  DavZ

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Cooked.h"

//==============================================================================
Cooked::Cooked(juce::AudioProcessorValueTreeState& treeState) : amount(treeState, "fold") {}

Cooked::~Cooked() {}

void Cooked::prepare(dsp::ProcessSpec& spec) {
	amount.prepare(spec);
}

void Cooked::processBlock(dsp::AudioBlock<float>& block) {
	TRACE_EVENT("dsp", "Cooked::processBlock");
	amount.update();

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		float nextCooked = amount.getNextValue() * 0.01f;

		//the original waveshaping thing
		if (nextCooked != 0.f) {
			auto x = rightDryData[sample] * (nextCooked * 20 + 1);
			rightDryData[sample] = (4 * (abs(0.25*x + 0.25 - round(0.25*x + 0.25)) - 0.25));

			x = leftDryData[sample] * (nextCooked * 20 + 1);
			leftDryData[sample] = (4 * (abs(0.25*x + 0.25 - round(0.25*x + 0.25)) - 0.25));
		}
	}
};