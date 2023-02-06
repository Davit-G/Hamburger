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
class Sizzle : public Component
{
public:
	Sizzle(juce::AudioProcessorValueTreeState &state);
	~Sizzle();

	void processBlock(AudioBuffer<float>& dryBuffer);
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	float whiteNoise() {
		return ((double)rand() / (RAND_MAX));
	}

	float pinkNoise() {
		return 0.f; //idk yet
	}


	float oldSizzleFunction(float inputSample, float nextSizzle) {
		return inputSample * (1 - whiteNoise() * (nextSizzle / 2)); //nextSizzle;
	}

	float newSizzleFunction(float inputSample, float nextSizzle, float sampleIsFromLeftChannel) {
		if (sampleIsFromLeftChannel) envelopeValue = envelopeDetector.processSample(inputSample);

		float audioClipped = std::fmin(1.f, std::fmax(0.f, inputSample));
		float noiseAmount = pow(audioClipped, 2) - 2*audioClipped + 1;

		return inputSample + (whiteNoise() * ((nextSizzle * envelopeValue * noiseAmount) / 2)); //nextSizzle * (nextSizzle / 2)
	}

private:
	EnvelopeFollower envelopeDetector;

	float envelopeValue = 0.f;

	juce::AudioProcessorValueTreeState &treeStateRef;
	juce::AudioParameterFloat *knobValue = nullptr;
	SmoothedValue<float> smoothedInput;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sizzle)
};
