/*
  ==============================================================================

	Sizzle.h
	Created: 13 Jun 2021 6:32:32pm
	Author:  DavZ

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "pinknoise/pinknoise.h"

#include "../EnvelopeFollower.h"

//==============================================================================
/*
 */
class Sizzle
{
public:
	Sizzle(juce::AudioProcessorValueTreeState &state);
	~Sizzle();

	void processBlock(dsp::AudioBlock<float>& block);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	float whiteNoise()
	{
		return ((float)rand() / (RAND_MAX));
	}
	
	float pinkNoise()
	{
		return 0.f; // idk yet
	}

	float oldSizzleFunction(float inputSample, float nextSizzle)
	{
		return inputSample * (1 - whiteNoise() * (nextSizzle / 2)); // nextSizzle;
	}

	float newSizzleFunction(float inputSample, float nextSizzle, float envelope)
	{

		float audioClipped = std::fmin(1.f, std::fmax(0.f, inputSample));
		float noiseAmount = pow(audioClipped, 2) - 2 * audioClipped + 1;

		return inputSample + (random.nextFloat() * ((nextSizzle * envelope * noiseAmount) / 2)); // nextSizzle * (nextSizzle / 2)
	}

private:
	unsigned long xrng = 123456789;
	unsigned long yrng = 362436069;
	unsigned long zrng = 521288629;

	float fastNoise(void)
	{ // period 2^96-1
		unsigned long t;
		xrng ^= xrng << 16;
		xrng ^= xrng >> 5;
		xrng ^= xrng << 1;

		t = xrng;
		xrng = yrng;
		yrng = zrng;
		zrng = t ^ xrng ^ yrng;

		return (float)zrng * 2.3283064365386963e-10 * 2;
	}

	EnvelopeFollower envelopeDetector;
	juce::Random random;

	juce::AudioProcessorValueTreeState &treeStateRef;
	juce::AudioParameterFloat *knobValue = nullptr;
	SmoothedValue<float> smoothedInput;
};
