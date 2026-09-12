#pragma once

 
#include "../SmoothParam.h"
#include "../../utils/Params.h"
#include "../EffectBase.h"

#include "juce_dsp/juce_dsp.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO

class DiodeWaveshape : public MacroEffect
{
public:
	DiodeWaveshape(juce::AudioProcessorValueTreeState& treeState, SlotId slot);

	void processBlock(juce::dsp::AudioBlock<float>& block) noexcept override;
	void prepare(juce::dsp::ProcessSpec& spec) noexcept override;

private:
	SmoothParam amount;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiodeWaveshape)

    float threePiOverFour = (3.0f * juce::MathConstants<float>::pi) * 0.25f;
    float sinThreePiOverFour = juce::dsp::FastMathApproximations::sin(threePiOverFour);
};
