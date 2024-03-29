#include "SoftClipper.h"

template <typename T>
int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

SoftClip::SoftClip(juce::AudioProcessorValueTreeState& treeState) : saturationKnob(treeState, "saturationAmount") {}

SoftClip::~SoftClip()
{
}

void SoftClip::prepare(dsp::ProcessSpec& spec)
{
    saturationKnob.prepare(spec);
}

void SoftClip::processBlock(dsp::AudioBlock<float>& block) {
    TRACE_EVENT("dsp", "SoftClip::processBlock");
    saturationKnob.update();

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float saturationAmount = saturationKnob.getNextValue() * 0.2 + 0.00000001f;

        for (int channel = 0; channel < block.getNumChannels(); channel++)
        {
            float xn = block.getSample(channel, sample);
            block.setSample(channel, sample, atanWaveShaper(xn, saturationAmount));
        }
    }
}