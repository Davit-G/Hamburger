#include "SoftClipper.h"

template <typename T>
int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

SoftClip::SoftClip(juce::AudioParameterFloat* param)
{
    saturationKnob = param;
    jassert(saturationKnob);
}

SoftClip::~SoftClip()
{
}

void SoftClip::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    
}

void SoftClip::processBlock(dsp::AudioBlock<float>& block) {
    float saturationAmount = saturationKnob->get();
	for (int channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto *channelData = block.getChannelPointer(channel);

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float xn = channelData[sample];
            // float saturation = 1.3;
            channelData[sample] = sgn(xn) * (1.0 - exp(-fabs((saturationAmount * 0.1f + 1.f) * xn))); // multiply audio on both channels by gain
        }
    }
}