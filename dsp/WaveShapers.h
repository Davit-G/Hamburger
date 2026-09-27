#pragma once

#include <cmath>

#include "juce_audio_basics/juce_audio_basics.h"

template <typename T>
int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}


inline float calcWSGain(float xn, float saturation, float asymmetry)
{
	return ((xn >= 0.0f && asymmetry > 0.0f) || (xn < 0.0f && asymmetry < 0.0f)) ? saturation * (1.0f + 4.0f * fabs(asymmetry)) : saturation;
}

inline float atanWaveShaper(float xn, float saturation) noexcept
{
	return atan(saturation * xn) / atan(saturation);
}

inline float tanhWaveShaper(float xn, float saturation) noexcept
{
	return tanh(saturation * xn) / tanh(saturation);
}

inline float softClipWaveShaper(float xn, float saturation)
{
	// --- un-normalized soft clipper from Reiss book
	return sgn(xn) * (1.0f - exp(-fabs(saturation * xn)));
}

inline float fuzzExp1WaveShaper(float xn, float saturation, float asymmetry)
{
	// --- setup gain
	float wsGain = calcWSGain(xn, saturation, asymmetry);
	return sgn(xn) * (1.0f - exp(-fabs(wsGain * xn))) / (1.0f - exp(-wsGain));
}

/*	A soft clip at 0db with a knee kneeDb wide centred on it, worked in dB like the compressor's soft knee: straight through
	below the knee, held at 0db above it, and bending between. A knee of nothing is a hard clip. */
inline float softClipperFunc(float x, float kneeDb)
{
	const float level = std::abs(x);

	if (level <= 0.0f)
		return x;

	const float halfKnee = kneeDb * 0.5f;
	const float db = juce::Decibels::gainToDecibels(level);

	if (db <= -halfKnee)
		return x;

	if (db >= halfKnee)
		return sgn(x);

	const float over = db + halfKnee;
	return sgn(x) * juce::Decibels::decibelsToGain(db - over * over / (2.0f * kneeDb));
}

inline bool isSoftClipperKnee(float x, float kneeDb)
{
	const float db = juce::Decibels::gainToDecibels(std::abs(x));
	return db > -kneeDb * 0.5f && db < kneeDb * 0.5f;
}

inline float tanhApprox1(float x)
{
	// this bends up at the ends (diverges away from 0)
	// reiss optimised, much faster, bit inaccurate, doesnt matter inside of -1 to 1 tho
	auto x2 = x * x;
	return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float tanhApprox2(float x) {
	// this bends down at the ends (converges to 0)
	// lambert continued fraction
	auto x2 = x * x;

	auto num = x * (135135.f + x2 * (17325.f + x2 * 378.f));
	auto den = 135135.f + x2 * (62370.f + x2 * (3150.f + x2 * 18.f));
	return num / den;
}

inline float approxTanhWaveshaper2(float x, float saturation)
{
	return tanhApprox2((saturation + 0.001f) * x) / tanhApprox2(saturation + 0.001f);
}

inline float approxTanhWaveshaper1(float x, float saturation)
{
	return tanhApprox1((saturation + 0.001f) * x) / tanhApprox1(saturation + 0.001f);
}