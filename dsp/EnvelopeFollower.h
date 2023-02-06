#pragma once
#include <cmath> // std::abs

#include <JuceHeader.h>



class EnvelopeFollower : public juce::Component
{
public:
	EnvelopeFollower(bool log) {
		useLog = log;
	}

	EnvelopeFollower(juce::AudioProcessorValueTreeState& state, bool log) 
	: treeStateRef(&state)
	{
		useLog = log;
		
	};

	~EnvelopeFollower() {

	};

	float processSample(float input);

	void prepareToPlay(double sampleRate, int samplesPerBlock);

	void setAttackTime(double attack_in_ms)
	{
		if (attackTime_mSec == attack_in_ms)
			return;
		attackTime_mSec = attack_in_ms;
		attackTime = std::exp(AUDIO_ENVELOPE_ANALOG_TC / (attack_in_ms *
														 (float)sampleRate * 0.001));
	}

	void setReleaseTime(double release_in_ms)
	{
		if (releaseTime_mSec == release_in_ms)
			return;
		releaseTime_mSec = release_in_ms;
		releaseTime = std::exp(AUDIO_ENVELOPE_ANALOG_TC / (release_in_ms *
														  (float)sampleRate * 0.001));
	}
	
	void setSampleRate(double sampleRateIn) {
		sampleRate = sampleRateIn;
	}

private:
	juce::AudioProcessorValueTreeState *treeStateRef = nullptr;

	juce::AudioParameterFloat *knobValue = nullptr;
	juce::AudioParameterFloat *knobValue2 = nullptr;

	float attackTime_mSec;
	float attackTime;
	float releaseTime_mSec;
	float releaseTime;
	
	bool useLog = true;

	float lastEnvelope;

	

	double sampleRate;

	const double AUDIO_ENVELOPE_ANALOG_TC = -0.99967234081320612357829304641019; // ln(36.7%)


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeFollower)
};
