/*
  ==============================================================================

    Jeff.cpp
    Created: 13 Jun 2021 5:57:58pm
    Author:  DavZ

  ==============================================================================
*/

 
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
		auto a = x * nextJeff;
		rightDryData[sample] = x + (a * sin(a * 125.6f)) * 0.2f;

		x = leftDryData[sample];
		a = x * nextJeff;
		leftDryData[sample] = x + (a * sin(a * 125.6f)) * 0.2f;
	}
}