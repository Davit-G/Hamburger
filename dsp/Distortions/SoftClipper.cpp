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

void SoftClip::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    saturationKnob.prepareToPlay(sampleRate, samplesPerBlock);
}

void SoftClip::processBlock(dsp::AudioBlock<float>& block) {
    saturationKnob.update();

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float saturationAmount = saturationKnob.getNextValue();

        float xn = block.getSample(0, sample);
        block.setSample(0, sample, sgn(xn) * (1.0 - exp(-fabs((saturationAmount * 0.1f + 1.f) * xn)))); // multiply audio on both channels by gain

        xn = block.getSample(1, sample);
        block.setSample(1, sample, sgn(xn) * (1.0 - exp(-fabs((saturationAmount * 0.1f + 1.f) * xn)))); // multiply audio on both channels by gain
    }
}