#include "HardClip.h"

HardClip::HardClip(juce::AudioProcessorValueTreeState& treeState) : gain(treeState, "saturationAmount") {}

HardClip::~HardClip()
{
}

void HardClip::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    gain.prepareToPlay(sampleRate, samplesPerBlock);
}

void HardClip::processBlock(dsp::AudioBlock<float>& block) {
    gain.update();
    
    float saturationAmount = gain.getRaw();
	for (int channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto *channelData = block.getChannelPointer(channel);

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float xn = channelData[sample];
            // hard clipping 
            channelData[sample] = std::min(1.f, std::max(-1.f, xn * (saturationAmount * 0.06f + 1.f))); // multiply audio on both channels by gain
        }
    }
}