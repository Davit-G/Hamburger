#include <cmath>

/**
@brief calculates sgn( ) of input
\param xn - the input value
\return -1 if xn is negative or +1 if xn is 0 or greater
*/
inline double sgn(double xn)
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
inline double calcWSGain(double xn, double saturation, double asymmetry)
{
	double g = ((xn >= 0.0 && asymmetry > 0.0) || (xn < 0.0 && asymmetry < 0.0)) ? saturation * (1.0 + 4.0*fabs(asymmetry)) : saturation;
	return g;
}

/**

@brief calculates arctangent waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline double atanWaveShaper(double xn, double saturation)
{
	return atan(saturation*xn) / atan(saturation);
}

/**

@brief calculates hyptan waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline double tanhWaveShaper(double xn, double saturation)
{
	return tanh(saturation*xn) / tanh(saturation);
}

/**
@brief calculates hyptan waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline double softClipWaveShaper(double xn, double saturation)
{
	// --- un-normalized soft clipper from Reiss book
	return sgn(xn)*(1.0 - exp(-fabs(saturation*xn)));
}

/**
@brief calculates fuzz exp1 waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline double fuzzExp1WaveShaper(double xn, double saturation, double asymmetry)
{
	// --- setup gain
	double wsGain = calcWSGain(xn, saturation, asymmetry);
	return sgn(xn)*(1.0 - exp(-fabs(wsGain*xn))) / (1.0 - exp(-wsGain));
}
