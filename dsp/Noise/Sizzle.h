#pragma once

#include "../../utils/Params.h"

#include "../SmoothParam.h"
#include "../EnvelopeFollower.h"

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

class Sizzle
{
public:
	Sizzle(juce::AudioProcessorValueTreeState& treeState);
	~Sizzle();

	void processBlock(dsp::AudioBlock<float>& block);
	void processBlockOG(dsp::AudioBlock<float>& block);
	void prepare(dsp::ProcessSpec& spec);

	void setSampleRate(float sampleRate) { envelopeDetector.setSampleRate(sampleRate);}

	static float whiteNoise()
	{
		return ((float)rand() / (RAND_MAX));
	}
	
	float pinkNoise()
	{
		return 0.f; // idk yet
	}

	float oldSizzleFunction(float inputSample, float nextSizzle)
	{
		return inputSample * (1.0f - whiteNoise() * (nextSizzle * 0.5f)); // nextSizzle;
	}

	float newSizzleFunction(float inputSample, float nextSizzle, float envelope, float nextRand)
	{
		float audioClipped = std::fmin(1.f, std::fmax(0.f, inputSample));
		float amt = audioClipped * audioClipped - 2.0f * audioClipped + 1.0f;

		return inputSample + (nextRand * ((nextSizzle * envelope * amt) * 0.5f)); // nextSizzle * (nextSizzle / 2)
	}

	float newSizzleV3(float inputSample, float nextSizzle, float envelope, float nextRand)
	{
		float sign = inputSample < 0 ? -1.0f : 1.0f;
		float rectifiedSample = fabs(inputSample);
		float sizzleLevel = nextSizzle * 4.0f * pow(fmax(envelope - rectifiedSample, 0.0f), 0.8f + 0.5f / filterQ.getRaw());

		return inputSample + (sizzleLevel * nextRand - (sizzleLevel * 0.5f)) * sign;
	}

private:
	EnvelopeFollower envelopeDetector;
	juce::Random random;
	
	SmoothParam noiseAmount;
	SmoothParam filterTone;
	SmoothParam filterQ;
	
	// filter to filter out sizzle tone
	dsp::IIR::Filter<float> filter;

	double sampleRate;
};
