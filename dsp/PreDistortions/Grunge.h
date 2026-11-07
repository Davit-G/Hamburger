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
    
    /*  The tone, a highpass inside the feedback loop. A TPT state variable filter rather than a direct form IIR: swapping a
        direct form's coefficients under modulation spikes its gain for a moment, and with the loop at up to 0.999 that
        compounded to inf and took the host down. This one stays stable however fast its cutoff moves. */
    juce::dsp::StateVariableTPTFilter<float> dcBlocker;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Grunge)
};
