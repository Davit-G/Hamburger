#include "DiodeWaveshape.h"

//==============================================================================
DiodeWaveshape::DiodeWaveshape(juce::AudioProcessorValueTreeState &treeState) : amount(treeState, "diode") {}

void DiodeWaveshape::prepare(juce::dsp::ProcessSpec &spec)
{
	amount.prepare(spec);
}

void DiodeWaveshape::processBlock(juce::dsp::AudioBlock<float> &block)
{
	TRACE_EVENT("dsp", "DiodeWaveshape::processBlock");
	amount.update();

	for (int channel = 0; channel < block.getNumChannels(); channel++)
	{
		auto dryData = block.getChannelPointer(channel);
		for (int sample = 0; sample < block.getNumSamples(); sample++)
		{
            const auto amountNext = amount.getNextValue() * 0.01f * 3.0f + 0.000001f;
			float amt = amountNext * amountNext * amountNext;

            float blend = fmin(amt * 0.8f, 1.0f);
            auto x = dryData[sample];

            float newComponent = 0.0f;

            if (x > threePiOverFour / amt) {
                newComponent = sinThreePiOverFour;
            } else if (x < -threePiOverFour / amt) {
                newComponent = -sinThreePiOverFour;
            } else {
                newComponent = juce::dsp::FastMathApproximations::sin(x * amt);
            }

            dryData[sample] = newComponent * blend + x * (1.0f - blend);
		}
	}
}