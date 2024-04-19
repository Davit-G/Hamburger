#pragma once

 
#include "../SmoothParam.h"
#include "../../utils/Params.h"

#include "../EnvelopeFollower.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

class Fuzz
{
public:
	Fuzz(juce::AudioProcessorValueTreeState& treeState);
	~Fuzz();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepare(dsp::ProcessSpec& spec);

private:
    SmoothParam bias;

    EnvelopeFollower follower;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fuzz)
};
