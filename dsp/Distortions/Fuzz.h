#pragma once

 
#include "../SmoothParam.h"
#include "../../utils/Params.h"
#include "../EffectBase.h"

#include "../EnvelopeFollower.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO

class Fuzz : public MacroEffect
{
public:
	Fuzz(juce::AudioProcessorValueTreeState& treeState, SlotId slot);
	~Fuzz();

	void processBlock(juce::dsp::AudioBlock<float>& block) override;
	void prepare(juce::dsp::ProcessSpec& spec) override;

private:
    SmoothParam bias;

    EnvelopeFollower follower;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fuzz)
};
