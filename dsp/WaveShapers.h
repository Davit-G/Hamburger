#pragma once

#include <cmath>

inline float sgn(float xn)
{
	return (xn > 0.f) - (xn < 0.f);
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

inline float softClipperFunc(float x, float threshold, float knee, float ratio = 0.00000001f)
{
	// this is the soft-clip function from the compressor
	// https://www.desmos.com/calculator/f8zazgtwpe

	float sign = sgn(x); // rectify it so we stay in the positive domain
	x = sign * x;

	float output = 0.0;

	if (2 * (x - threshold) < -knee)
	{
		output = x;
	}
	else if (2 * (x - threshold) > knee)
	{
		output = threshold + (x - threshold) * ratio;
	}
	else
	{
		output = x + ((ratio - 1.0f) * pow((x - threshold + knee * 0.5f), 2.0f) / (2.0f * knee));
	}

	return output * sign; // return it to the original sign afterwards
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