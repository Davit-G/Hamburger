#include "PattyFuzz.h"

//==============================================================================
PattyFuzz::PattyFuzz(juce::AudioProcessorValueTreeState &treeState) : amount(treeState, ParamIDs::diode) {}

void PattyFuzz::prepare(juce::dsp::ProcessSpec &spec)
{
	amount.prepare(spec);
}

void PattyFuzz::processBlock(juce::dsp::AudioBlock<float> &block)
{
	TRACE_EVENT("dsp", "PattyFuzz::processBlock");
	amount.update();

	for (int channel = 0; channel < block.getNumChannels(); channel++)
	{
		auto dryData = block.getChannelPointer(channel);
		for (int sample = 0; sample < block.getNumSamples(); sample++)
		{
			float nextPatty = 1.f - (amount.getNextValue() * 0.006f);
			auto x = dryData[sample];
			if (x > 0)
			{
				dryData[sample] = powf(x, nextPatty);
			}
			else if (x < 0)
			{
				dryData[sample] = -powf(-x, nextPatty);
			}
			else
			{
				dryData[sample] = 0.f;
			}
		}
	}
}