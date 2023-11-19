#include <cmath>

/**
@brief calculates sgn( ) of input
\param xn - the input value
\return -1 if xn is negative or +1 if xn is 0 or greater
*/
inline float sgn(float xn)
{
	return (xn > 0) - (xn < 0);
}

/**

@brief calculates gain of a waveshaper
\param xn - the input value
\param saturation  - the saturation control
\param asymmetry  - the degree of asymmetry
\return gain value
*/
inline float calcWSGain(float xn, float saturation, float asymmetry)
{
	double g = ((xn >= 0.0f && asymmetry > 0.0f) || (xn < 0.0f && asymmetry < 0.0f)) ? saturation * (1.0f + 4.0f*fabs(asymmetry)) : saturation;
	return g;
}

/**

@brief calculates arctangent waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline float atanWaveShaper(float xn, float saturation)
{
	return atan(saturation*xn) / atan(saturation);
}

/**

@brief calculates hyptan waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline float tanhWaveShaper(float xn, float saturation)
{
	return tanh(saturation*xn) / tanh(saturation);
}

/**
@brief calculates hyptan waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline float softClipWaveShaper(float xn, float saturation)
{
	// --- un-normalized soft clipper from Reiss book
	return sgn(xn)*(1.0f - exp(-fabs(saturation*xn)));
}

/**
@brief calculates fuzz exp1 waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline float fuzzExp1WaveShaper(float xn, float saturation, float asymmetry)
{
	// --- setup gain
	float wsGain = calcWSGain(xn, saturation, asymmetry);
	return sgn(xn)*(1.0f - exp(-fabs(wsGain*xn))) / (1.0f - exp(-wsGain));
}
