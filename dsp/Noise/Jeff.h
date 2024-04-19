#pragma once

#include "../../utils/Params.h"

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

#include "../SmoothParam.h"

//==============================================================================
/*
*/
class Jeff
{
public:
	Jeff(juce::AudioProcessorValueTreeState& treeState);
	~Jeff();

	void processBlock(juce::dsp::AudioBlock<float>& block);
	void prepare(juce::dsp::ProcessSpec& spec);

private:
	SmoothParam amount;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Jeff)
};
