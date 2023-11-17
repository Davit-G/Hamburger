#pragma once

#include <JuceHeader.h>
#include "../SmoothParam.h"

#include "../EnvelopeFollower.h"

class Fuzz
{
public:
	Fuzz(juce::AudioProcessorValueTreeState& treeState);
	~Fuzz();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    SmoothParam bias;

    EnvelopeFollower follower;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fuzz)
};
