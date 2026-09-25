#pragma once
#include <cmath> // std::abs

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

class EnvelopeFollower
{
public:
	EnvelopeFollower(bool log) {
		useLog = log;

		setAttackTime(50);
    	setReleaseTime(500);
	}

	~EnvelopeFollower() {}

	float processSample(float xn);
	float processSampleStereo(float xL, float xR);

	void prepare(juce::dsp::ProcessSpec& spec);

	void setAttackTime(float attack_in_ms)
	{
		// if (attackTime_mSec != attack_in_ms) { // saving performance
			attackTime_mSec = attack_in_ms;
			attackTime = timeConstant(attack_in_ms);
		// }
	}

	void setReleaseTime(float release_in_ms)
	{
		// if (releaseTime_mSec != release_in_ms) { // saving performance
			releaseTime_mSec = release_in_ms;
			releaseTime = timeConstant(release_in_ms);
		// }
	}
	
	void setSampleRate(float sampleRateIn) {
		sampleRate = sampleRateIn;
		// recalc attack and release times
		setAttackTime(attackTime_mSec);
		setReleaseTime(releaseTime_mSec);
	}

private:
	float timeConstant(float ms) const
	{
		return ms > 0.0f ? std::exp(AUDIO_ENVELOPE_ANALOG_TC / (ms * sampleRate * 0.001f)) : 0.0f;
	}

	float attackTime_mSec;
	float attackTime;
	float releaseTime_mSec;
	float releaseTime;
	
	bool useLog = true;
	// bool useRMS = true; i guess it always uses RMS so ignore that

	float lastEnvelope;
	float sampleRate;

	const double AUDIO_ENVELOPE_ANALOG_TC_DOUBLE = -0.99967234081320612357829304641019; // ln(36.7%)
	const float AUDIO_ENVELOPE_ANALOG_TC = -0.99967234081320612357829304641019f; // ln(36.7%)
};
