#pragma once

 
#include "../SmoothParam.h"

#include "juce_dsp/juce_dsp.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

class DiodeWaveshape
{
public:
	DiodeWaveshape(juce::AudioProcessorValueTreeState& treeState);

	void processBlock(juce::dsp::AudioBlock<float>& block) noexcept;
	void prepare(juce::dsp::ProcessSpec& spec) noexcept;

private:
	SmoothParam amount;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiodeWaveshape)

    float threePiOverFour = (3.0f * juce::MathConstants<float>::pi) * 0.25f;
    float sinThreePiOverFour = juce::dsp::FastMathApproximations::sin(threePiOverFour);
};
