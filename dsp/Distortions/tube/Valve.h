

#include <JuceHeader.h>
#include "LossyIntegrator.h"

class ClassAValve
{
public:
	ClassAValve(void) {}
	~ClassAValve(void) {}

public:
	/** reset members to initialized state */
	void prepareToPlay(double _sampleRate, double samplesPerBlock)
	{
		dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2; // eh

		// --- do any other per-audio-run inits here
		sampleRate = _sampleRate;

		// --- integrators
		lossyIntegrator[0].calculateFilterCoeffs();
		lossyIntegrator[1].calculateFilterCoeffs();
		lossyIntegrator[0].prepareToPlay(_sampleRate);
		lossyIntegrator[1].prepareToPlay(_sampleRate);

		// --- low shelf
		lowShelvingFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(_sampleRate, lowFrequencyShelf_Hz, 0.707f, lowFrequencyShelfGain_dB);
		lowShelvingFilter.coefficients = *lowShelvingFilterCoeffs;

		// --- output HPF
		dcBlockingFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(_sampleRate, dcBlockingLF_Hz, 0.707f);
		dcBlockingFilter.coefficients = *dcBlockingFilterCoeffs;

		// --- LPF (upper edge), technically supposed to be second order
		upperBandwidthFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(_sampleRate, millerHF_Hz, 0.707f);
		upperBandwidthFilter1stOrder.coefficients = *upperBandwidthFilterCoeffs;
		upperBandwidthFilter2ndOrder.coefficients = *upperBandwidthFilterCoeffs;

		lowShelvingFilter.prepare(spec);
		dcBlockingFilter.prepare(spec);
		upperBandwidthFilter1stOrder.prepare(spec);
		upperBandwidthFilter2ndOrder.prepare(spec);
	}

	// --- do the valve emulation
	double processAudioSample(double xn)
	{
		double yn = 0.0;
		
		xn *= inputGain; // --- input scaling
		xn = doValveGridConduction(xn, gridConductionThreshold); // grid conduction check, must be done prior to waveshaping
		
		double dcOffset = lossyIntegrator[0].processAudioSample(xn); // detect the DC offset that the clipping may have caused
		dcOffset = fmin(dcOffset, 0.0);// --- process only negative DC bias shifts
		
		// --- save this - user may indicate it in a meter if they want
		//     Note that this is a bipolar value, but we only do DC shift for 
		//     *negative* values so meters should be aware
		dcOffsetDetected = fabs(dcOffset*dcShiftCoefficient);

		// the emulation itself
		yn = doValveEmulation(xn, 
								waveshaperSaturation, 
								gridConductionThreshold,
								dcOffsetDetected,
								clipPointPositive, 
								clipPointNegative);
		
		yn = dcBlockingFilter.processSample(yn); // --- remove DC
		// yn = lowShelvingFilter.processSample(yn); // --- LF Shelf
		// yn = upperBandwidthFilter1stOrder.processSample(yn); // --- HF Edge
		// yn = upperBandwidthFilter2ndOrder.processSample(yn);
		yn *= -outputGain; // --- (5) final output scaling and inversion

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
	double sampleRate = 0.0;	///< sample rate

	inline double doValveGridConduction(double xn, double gridConductionThreshold)
	{
		if (xn > 0.0)
		{
			double clipDelta = xn - gridConductionThreshold;
			clipDelta = fmax(clipDelta, 0.0);
			double compressionFactor = 0.4473253 + 0.5451584*exp(-0.3241584*clipDelta);
			return compressionFactor*xn;
		}
		else
			return xn;
	}

	// --- main triode emulation - plenty of room here for experimentation
	inline double doValveEmulation(double xn, double k, double gridConductionThreshold,
									double variableDCOffset, double clipPointPos,
									double clipPointNeg)
	{
		xn += variableDCOffset; // --- add the offset detected
		double yn = 0.0;

		// --- NOTE: the whole transfer function is shifted so that the normal
		//           DC operating point is 0.0 to prevent massive
		//           clicks at the beginning of a selection due to the temporary DC offset
		//           that occurs before the HPF can react
		//
		
		if (xn > gridConductionThreshold) // +1.5 is where grid conduction starts
		{
			if (xn > clipPointPos)
				yn = clipPointPos;
			else
			{
				xn -= gridConductionThreshold; // --- scaling to get into the first quadrant for Arraya @ (0,0)
				
				if (clipPointPos > 1.0)
					xn /= (clipPointPos - gridConductionThreshold); // --- note that the signal should be clipped/compressed prior to calling this

				yn = xn*(3.0 / 2.0)*(1.0 - (xn*xn) / 3.0);
				yn *= (clipPointPos - gridConductionThreshold); // --- scale by clip point positive
				yn += gridConductionThreshold; // --- undo scaling
			}
		}
		else if (xn > 0.0) // --- ultra linear region
		{
			yn = xn; // --- fundamentally linear region of 3/2 power law
		}
		else // botom portion is tanh( ) waveshaper - EXPERIMENT!!
		{
			if (xn < clipPointNeg)
				yn = clipPointNeg;
			else
			{	
				if (clipPointNeg < -1.0) // --- clip normalize
					xn /= fabs(clipPointNeg);
				yn = tanh(k*xn) / tanh(k); // --- the waveshaper
				yn *= fabs(clipPointNeg); // --- undo clip normalize
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