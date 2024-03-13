#include "SoftClipper.h"
// todo: Soft clipping is very expensive and also unintuitive, replace with something else.

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
        float saturationAmount = saturationKnob.getNextValue() * 0.01 + 0.00000001f;

        float knee = (saturationAmount + 0.05) * 80.0f;
        float multX = 1.f + saturationAmount * 5.0f;

        float xn = block.getSample(0, sample);
        block.setSample(0, sample, atanWaveShaper(xn, saturationAmount * 10.0f));

        xn = block.getSample(1, sample);
        block.setSample(1, sample, atanWaveShaper(xn, saturationAmount * 10.0f));
    }
}