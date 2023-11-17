#pragma once

#include <JuceHeader.h>
#include "../SmoothParam.h"

class PattyFuzz
{
public:
	PattyFuzz(juce::AudioProcessorValueTreeState& treeState);
	~PattyFuzz();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
	SmoothParam amount;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PattyFuzz)
};
