#pragma once

 
#include "../SmoothParam.h"
#include "../../utils/Params.h"

#include "juce_dsp/juce_dsp.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO

class PattyFuzz
{
public:
	PattyFuzz(juce::AudioProcessorValueTreeState& treeState);

	void processBlock(juce::dsp::AudioBlock<float>& block);
	void prepare(juce::dsp::ProcessSpec& spec);

private:
	SmoothParam amount;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PattyFuzz)
};
