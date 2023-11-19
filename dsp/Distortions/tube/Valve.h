#include <JuceHeader.h>
#include "LossyIntegrator.h"

#include <cmath>

class ClassAValve
{
public:
	inline ClassAValve(void) {}
	~ClassAValve(void) {}

public:
	/** reset members to initialized state */
	void prepare(dsp::ProcessSpec& spec)
	{
		// --- do any other per-audio-run inits here
		sampleRate = spec.sampleRate;

		// --- integrators
		lossyIntegrator[0].calculateFilterCoeffs();
		lossyIntegrator[1].calculateFilterCoeffs();
		lossyIntegrator[0].prepare(spec);
		lossyIntegrator[1].prepare(spec);

		calculateCoefficients();

		lowShelvingFilter.prepare(spec);
		dcBlockingFilter.prepare(spec);
		upperBandwidthFilter1stOrder.prepare(spec);
		upperBandwidthFilter2ndOrder.prepare(spec);
	}

	void calculateCoefficients() {
		// --- low shelf
		lowShelvingFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, lowFrequencyShelf_Hz, 0.707f, juce::Decibels::decibelsToGain(lowFrequencyShelfGain_dB));
		lowShelvingFilter.coefficients = *lowShelvingFilterCoeffs;

		// --- output HPF
		dcBlockingFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, dcBlockingLF_Hz, 0.707f);
		dcBlockingFilter.coefficients = *dcBlockingFilterCoeffs;

		// --- LPF (upper edge), technically supposed to be second order
		upperBandwidthFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, millerHF_Hz, 0.707f);
		upperBandwidthFilter1stOrder.coefficients = *upperBandwidthFilterCoeffs;
		upperBandwidthFilter2ndOrder.coefficients = *upperBandwidthFilterCoeffs;
	}

	// --- do the valve emulation
	float processAudioSample(float xn)
	{
		// TRACE_DSP();

		float yn = 0.0f;
		
		// TRACE_EVENT_BEGIN("dsp", "valve grid conduction check");
		xn *= inputGain; // --- input scaling
		xn = doValveGridConduction(xn); // grid conduction check, must be done prior to waveshaping
		// TRACE_EVENT_END("dsp");

		// TRACE_EVENT_BEGIN("dsp", "lossy integrator dc offset");
		float dcOffset = lossyIntegrator[0].processAudioSample(xn); // detect the DC offset that the clipping may have caused
		dcOffset = fmin(dcOffset, 0.0f);// --- process only negative DC bias shifts
		// TRACE_EVENT_END("dsp");


		// --- save this - user may indicate it in a meter if they want
		//     Note that this is a bipolar value, but we only do DC shift for 
		//     *negative* values so meters should be aware
		dcOffsetDetected = fabs(dcOffset * dcShiftCoefficient);

		// the emulation itself
		// TRACE_EVENT_BEGIN("dsp", "valve emulation");
		yn = doValveEmulation(xn, dcOffsetDetected);
		// TRACE_EVENT_END("dsp");

		// TRACE_EVENT_BEGIN("dsp", "filters");
		yn = dcBlockingFilter.processSample(yn); // --- remove DC
		yn = lowShelvingFilter.processSample(yn); // --- LF Shelf
		yn = upperBandwidthFilter1stOrder.processSample(yn); // --- HF Edge
		yn = upperBandwidthFilter2ndOrder.processSample(yn);
		yn *= -outputGain; // --- (5) final output scaling and inversion
		// TRACE_EVENT_END("dsp");

		return yn;
	}

	// make these public cause I am lazy :D
	float lowFrequencyShelf_Hz = 10.0f; // using defaults from will pirkle
	float lowFrequencyShelfGain_dB = 0.0f;

	float dcBlockingLF_Hz = 10.0f;
	float millerHF_Hz = 10000.0f;

	float inputGain = 1.5f;
	float outputGain = 1.0f;
	
	float clipPointPositive = 4.0f;
	float clipPointNegative = -1.5f;
	float gridConductionThreshold = 1.5f;
	float dcShiftCoefficient = 1.0f;
	float waveshaperSaturation = 2.0f;

	float dcOffsetDetected = 0.0f;

private:
	float sampleRate = 0.0f;	///< sample rate

	inline double doValveGridConduction(float xn)
	{
		if (xn > 0.0f)
		{
			float clipDelta = xn - gridConductionThreshold;
			clipDelta = fmax(clipDelta, 0.0f);
			float compressionFactor = 0.4473253f + 0.5451584f*dsp::FastMathApproximations::exp(-0.3241584f * clipDelta);
			return compressionFactor * xn;
		}
		else
			return xn;
	}

	// --- main triode emulation - plenty of room here for experimentation
	inline float doValveEmulation(float xn, float variableDCOffset)
	{
		xn += variableDCOffset; // --- add the offset detected
		float yn = 0.0f;

		// --- NOTE: the whole transfer function is shifted so that the normal
		//           DC operating point is 0.0 to prevent massive
		//           clicks at the beginning of a selection due to the temporary DC offset
		//           that occurs before the HPF can react
		//
		
		if (xn > gridConductionThreshold) // +1.5 is where grid conduction starts
		{
			if (xn > clipPointPositive)
				yn = clipPointPositive;
			else
			{
				xn -= gridConductionThreshold; // --- scaling to get into the first quadrant for Arraya @ (0,0)

				float cpPosSub =  clipPointPositive - gridConductionThreshold;
				
				if (clipPointPositive > 1.0f)
					xn /= cpPosSub; // --- note that the signal should be clipped/compressed prior to calling this

				yn = xn * (1.5f) * (1.0f - (xn * xn) * 0.333333333f);
				yn *= cpPosSub; // --- scale by clip point positive
				yn += gridConductionThreshold; // --- undo scaling
			}
		}
		else if (xn > 0.0f) // --- ultra linear region
		{
			yn = xn; // --- fundamentally linear region of 3/2 power law
		}
		else // botom portion is tanh( ) waveshaper - EXPERIMENT!!
		{
			if (xn < clipPointNegative)
				yn = clipPointNegative;
			else
			{	
				float absCPNegative = fabs(clipPointNegative);

				if (clipPointNegative < -1.0f) // --- clip normalize
					xn /= absCPNegative;
				
				yn = dsp::FastMathApproximations::tanh(waveshaperSaturation * xn) / 
					dsp::FastMathApproximations::tanh(waveshaperSaturation); // --- the waveshaper
				
				yn *= absCPNegative; // --- undo clip normalize
			}
		}

		return yn;
	}

	LossyIntegrator lossyIntegrator[2];

	juce::dsp::IIR::Filter<float> lowShelvingFilter;
	juce::dsp::IIR::Coefficients<float>::Ptr lowShelvingFilterCoeffs;

	juce::dsp::IIR::Filter<float> dcBlockingFilter;
	juce::dsp::IIR::Coefficients<float>::Ptr dcBlockingFilterCoeffs;

	juce::dsp::IIR::Filter<float> upperBandwidthFilter1stOrder;
	juce::dsp::IIR::Filter<float> upperBandwidthFilter2ndOrder;
	juce::dsp::IIR::Coefficients<float>::Ptr upperBandwidthFilterCoeffs;
};