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

void SoftClip::prepare(juce::dsp::ProcessSpec& spec) noexcept
{
    saturationKnob.prepare(spec);
}

void SoftClip::processBlock(juce::dsp::AudioBlock<float>& block) noexcept {
    TRACE_EVENT("dsp", "SoftClip::processBlock");
    saturationKnob.update();

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float saturationAmount = powf(saturationKnob.getNextValue() * 0.01f, 1.5f) * 100.0f * 0.24 + 0.00000001f;
        float outGain = juce::Decibels::decibelsToGain(juce::jmap(( 1.0f - saturationAmount * 0.05f), -4.0f, 0.0f));

        for (int channel = 0; channel < block.getNumChannels(); channel++)
        {
            float xn = block.getSample(channel, sample);
            block.setSample(channel, sample, atanWaveShaper(xn, saturationAmount) * outGain);
        }
    }
}