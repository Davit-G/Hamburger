#pragma once

 
#include "../SmoothParam.h"
#include "../../utils/Params.h"
#include "../EffectBase.h"

#include "juce_dsp/juce_dsp.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO

class PattyFuzz : public MacroEffect
{
public:
	PattyFuzz(juce::AudioProcessorValueTreeState& treeState, SlotId slot);

	void processBlock(juce::dsp::AudioBlock<float>& block) override;
	void prepare(juce::dsp::ProcessSpec& spec) override;

private:
	SmoothParam amount;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PattyFuzz)
};
