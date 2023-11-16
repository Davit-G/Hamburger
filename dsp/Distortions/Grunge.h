#pragma once

#include <JuceHeader.h>


class Grunge
{
public:
	Grunge(juce::AudioParameterFloat* amountParam, juce::AudioParameterFloat* toneParam);
	~Grunge();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
	juce::AudioParameterFloat *amountKnob = nullptr;
	juce::AudioParameterFloat *toneKnob = nullptr;

	SmoothedValue<float> smoothedAmount;
	SmoothedValue<float> smoothedTone;

    juce::dsp::DelayLine <float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    
    juce::dsp::IIR::Filter<float> dcBlockerL;
    juce::dsp::IIR::Filter<float> dcBlockerR;

    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    double sampleRate;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Grunge)
};
