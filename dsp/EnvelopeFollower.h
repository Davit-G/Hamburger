#pragma once
#include <cmath> // std::abs

class EnvelopeFollower
{
public:
	EnvelopeFollower(bool log) {
		useLog = log;

		setAttackTime(3);
    	setReleaseTime(3);
	}

	~EnvelopeFollower() {

	};

	float processSample(float xn);
	float processSampleStereo(float xL, float xR);

	void prepareToPlay(double sampleRate, int samplesPerBlock);

	void setAttackTime(double attack_in_ms)
	{
		if (attackTime_mSec != attack_in_ms) { // saving performance
			attackTime_mSec = attack_in_ms;
			attackTime = std::exp(AUDIO_ENVELOPE_ANALOG_TC / (attack_in_ms *
															(float)sampleRate * 0.001));
		}
	}

	void setReleaseTime(double release_in_ms)
	{
		if (releaseTime_mSec != release_in_ms) { // saving performance
			releaseTime_mSec = release_in_ms;
			releaseTime = std::exp(AUDIO_ENVELOPE_ANALOG_TC / (release_in_ms *
															(float)sampleRate * 0.001));
		}
	}
	
	void setSampleRate(double sampleRateIn) {
		sampleRate = sampleRateIn;
		// recalc attack and release times
		setAttackTime(attackTime_mSec);
		setReleaseTime(releaseTime_mSec);
	}

private:
	float attackTime_mSec;
	float attackTime;
	float releaseTime_mSec;
	float releaseTime;
	
	bool useLog = true;
	bool useRMS = true;

	float lastEnvelope;
	double sampleRate;

	const double AUDIO_ENVELOPE_ANALOG_TC = -0.99967234081320612357829304641019; // ln(36.7%)
};
