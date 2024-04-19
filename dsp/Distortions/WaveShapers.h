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