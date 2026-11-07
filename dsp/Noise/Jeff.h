#pragma once

#include "../../utils/Params.h"

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"
#if PERFETTO

#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO
#include "../SmoothParam.h"
#include "../EffectBase.h"

//==============================================================================
/*
*/
class Jeff : public MacroEffect
{
public:
	Jeff(juce::AudioProcessorValueTreeState& treeState, SlotId slot);
	~Jeff();

	void processBlock(juce::dsp::AudioBlock<float>& block) override;
	void prepare(juce::dsp::ProcessSpec& spec) override;

private:
	SmoothParam amount;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Jeff)
};
