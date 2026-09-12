#pragma once

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "../SmoothParam.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"

// #include <melatonin_perfetto/melatonin_perfetto.h>

class Grunge : public MacroEffect
{
public:
	Grunge(juce::AudioProcessorValueTreeState& treeState);
	~Grunge() {}

	void processBlock(juce::dsp::AudioBlock<float>& block) override;
	void prepare(juce::dsp::ProcessSpec& spec) override;

private:
	SmoothParam amount;
	SmoothParam tone;

    juce::dsp::DelayLine <float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    
    juce::dsp::IIR::Filter<float> dcBlockerL;
    juce::dsp::IIR::Filter<float> dcBlockerR;

    double sampleRate;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Grunge)
};
