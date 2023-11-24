/*
  ==============================================================================

	Sizzle.h
	Created: 13 Jun 2021 6:32:32pm
	Author:  DavZ

  ==============================================================================
*/

#pragma once

 

#include "../SmoothParam.h"
#include "../EnvelopeFollower.h"

//==============================================================================
/*
 */
class Sizzle
{
public:
	Sizzle(juce::AudioProcessorValueTreeState& treeState);
	~Sizzle();

	void processBlock(dsp::AudioBlock<float>& block);
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

	float newSizzleFunction(float inputSample, float nextSizzle, float envelope)
	{

		float audioClipped = std::fmin(1.f, std::fmax(0.f, inputSample));
		float amt = powf(audioClipped, 2.0f) - 2.0f * audioClipped + 1.0f;

		return inputSample + (random.nextFloat() * ((nextSizzle * envelope * amt) * 0.5f)); // nextSizzle * (nextSizzle / 2)
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
	
	SmoothParam noiseAmount;
};
