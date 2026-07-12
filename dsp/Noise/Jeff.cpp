#include "Jeff.h"

Jeff::Jeff(juce::AudioProcessorValueTreeState &treeState) : amount(treeState, ParamIDs::jeffAmount) {}

Jeff::~Jeff()
{
}

void Jeff::prepare(juce::dsp::ProcessSpec &spec)
{
	amount.prepare(spec);
}

void Jeff::processBlock(juce::dsp::AudioBlock<float> &block)
{
	// TRACE_EVENT("dsp", "Jeff::processBlock");
	amount.update();

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (size_t channel = 0; channel < block.getNumChannels(); channel++)
	{
		auto dryData = block.getChannelPointer(channel);

		for (int sample = 0; sample < block.getNumSamples(); sample++)
		{
			float nextJeff = amount.getNextValue(channel) * 0.01f;

			auto x = dryData[sample];
			auto a = x * nextJeff;
			dryData[sample] = x + (a * sin(a * 125.6f)) * 0.2f;
		}
	}
}