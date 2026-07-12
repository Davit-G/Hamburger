#include "DiodeWaveshape.h"

//==============================================================================
DiodeWaveshape::DiodeWaveshape(juce::AudioProcessorValueTreeState &treeState) : amount(treeState, ParamIDs::diode) {}

void DiodeWaveshape::prepare(juce::dsp::ProcessSpec &spec) noexcept
{
    amount.prepare(spec);
}

void DiodeWaveshape::processBlock(juce::dsp::AudioBlock<float> &block) noexcept
{
	#if PERFETTO
    // TRACE_EVENT("dsp", "DiodeWaveshape::processBlock");
    #endif // PERFETTO

    amount.update();

    for (int channel = 0; channel < block.getNumChannels(); channel++)
    {
        auto channelData = block.getChannelPointer(channel);

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float a = amount.getNextValue(channel) * 0.01f * 3.0f + 0.000001f;
            float amt = a*a*a;

            float blend = fmin(amt * 0.8f, 1.0f);
            auto x = channelData[sample];

            float newComponent = 0.0f;

            if (x > threePiOverFour / amt)
            {
                newComponent = sinThreePiOverFour;
            }
            else if (x < -threePiOverFour / amt)
            {
                newComponent = -sinThreePiOverFour;
            }
            else
            {
                newComponent = juce::dsp::FastMathApproximations::sin(x * amt);
            }

            channelData[sample] = newComponent * blend + x * (1.0f - blend);
        }
    }
}