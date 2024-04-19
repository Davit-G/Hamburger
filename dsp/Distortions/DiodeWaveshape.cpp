#include "DiodeWaveshape.h"

//==============================================================================
DiodeWaveshape::DiodeWaveshape(juce::AudioProcessorValueTreeState &treeState) : amount(treeState, ParamIDs::diode) {}

void DiodeWaveshape::prepare(juce::dsp::ProcessSpec &spec) noexcept
{
    amount.prepare(spec);
}

void DiodeWaveshape::processBlock(juce::dsp::AudioBlock<float> &block) noexcept
{
    TRACE_EVENT("dsp", "DiodeWaveshape::processBlock");
    amount.update();

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float amt = powf(amount.getNextValue() * 0.01f * 3.0f + 0.000001f, 3);

        for (int channel = 0; channel < block.getNumChannels(); channel++)
        {
            auto dryData = block.getChannelPointer(channel);

            float blend = fmin(amt * 0.8f, 1.0f);
            auto x = dryData[sample];

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

            dryData[sample] = newComponent * blend + x * (1.0f - blend);
        }
    }
}