#pragma once

 

#include "../SmoothParam.h"

class Grunge
{
public:
	Grunge(juce::AudioProcessorValueTreeState& treeState);
	~Grunge() {}

	void processBlock(dsp::AudioBlock<float>& block);
	void prepare(dsp::ProcessSpec& spec);

private:
	SmoothParam amount;
	SmoothParam tone;

    juce::dsp::DelayLine <float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    
    juce::dsp::IIR::Filter<float> dcBlockerL;
    juce::dsp::IIR::Filter<float> dcBlockerR;

    double sampleRate;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Grunge)
};
