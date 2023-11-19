/*
  ==============================================================================

    Jeff.cpp
    Created: 13 Jun 2021 5:57:58pm
    Author:  DavZ

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Jeff.h"

//==============================================================================
Jeff::Jeff(juce::AudioProcessorValueTreeState& treeState) : amount(treeState, "noiseAmount") {}

Jeff::~Jeff()
{
}


void Jeff::prepare(dsp::ProcessSpec& spec) {
	amount.prepare(spec);
}

void Jeff::processBlock(dsp::AudioBlock<float>& block) {
	TRACE_EVENT("dsp", "Jeff::processBlock");
	amount.update();

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++) {
		float nextJeff = amount.get() * 0.01f;

		auto x = rightDryData[sample];
		rightDryData[sample] = x + (x * nextJeff * sin(x * nextJeff * 125.6f)) * 0.2f;

		x = leftDryData[sample];
		leftDryData[sample] = x + (x * nextJeff * sin(x * nextJeff * 125.6f)) * 0.2f;
	}
}