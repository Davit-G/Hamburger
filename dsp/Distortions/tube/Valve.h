#pragma once 
#include "LossyIntegrator.h"

#include <cmath>
#include <JuceHeader.h>

template <typename SampleType>
class ClassAValve
{
public:
	ClassAValve(void) {}
	~ClassAValve(void) {}

public:
	/** reset members to initialized state */
	void prepare(dsp::ProcessSpec &spec)
	{
		// --- do any other per-audio-run inits here
		sampleRate = spec.sampleRate;

		// --- integrators
		lossyIntegrator[0].calculateFilterCoeffs();
		lossyIntegrator[0].prepare(spec);
		lossyIntegrator[1].calculateFilterCoeffs();
		lossyIntegrator[1].prepare(spec);

		calculateCoefficients();

		lowShelvingFilter.prepare(spec);
		dcBlockingFilter.prepare(spec);
		upperBandwidthFilter1stOrder.prepare(spec);
		upperBandwidthFilter2ndOrder.prepare(spec);
	}

	void calculateCoefficients()
	{
		// --- low shelf
		*lowShelvingFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(sampleRate, lowFrequencyShelf_Hz, 0.707f, juce::Decibels::decibelsToGain(lowFrequencyShelfGain_dB));

		// --- output HPF
		*dcBlockingFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(sampleRate, dcBlockingLF_Hz);

		// --- LPF (upper edge), technically supposed to be second order
		auto upperBandwidthFilterCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, millerHF_Hz, 0.89f);
		*upperBandwidthFilter1stOrder.state = upperBandwidthFilterCoeffs;
		*upperBandwidthFilter2ndOrder.state = upperBandwidthFilterCoeffs;
	}

	void processBlock(const dsp::ProcessContextReplacing<SampleType> &context)
	{
		auto inputBlock = context.getInputBlock();
		auto outputBlock = context.getOutputBlock();
		auto channels = outputBlock.getNumChannels();

		for (int ch = 0; ch < channels; ++ch)
		{
			auto* input = inputBlock.getChannelPointer(ch);
			auto* output = outputBlock.getChannelPointer(ch);

			for (int i = 0; i < inputBlock.getNumSamples(); ++i)
			{
				output[i] = processAudioSampleOld(input[i], ch);
			}
		}

		dcBlockingFilter.process(context);			  // --- remove DC
		lowShelvingFilter.process(context);			  // --- LF Shelf
		upperBandwidthFilter1stOrder.process(context); // --- HF Edge
		// upperBandwidthFilter2ndOrder.process(context);
		outputBlock.multiplyBy(-outputGain); // --- (5) final output scaling and inversion
	}

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
	float dcShiftAdditional = 0.0f;
	float waveshaperSaturation = 2.0f;

	float blend = 1.0f; // from 0 to 1

	float dcOffsetDetected = 0.0f;

	// --- do the valve emulation
	SampleType processAudioSampleOld(SampleType xn, int channel = 0)
	{

		auto og = xn;
		xn *= inputGain + dcShiftAdditional;				   // --- input scaling
		xn = doValveGridConductionOld(xn); // grid conduction check, must be done prior to waveshaping

		// float dcOffset = (lossyIntegrator).processAudioSample(dsp::SIMDRegister<float>(xn)).get(0); // detect the DC offset that the clipping may have caused
		float dcOffset = lossyIntegrator[channel].processAudioSample(xn); // detect the DC offset that the clipping may have caused
		dcOffset = fmin(dcOffset, 0.0f);

		// --- save this - user may indicate it in a meter if they want
		//     Note that this is a bipolar value, but we only do DC shift for
		//     *negative* values so meters should be aware
		// dcOffsetDetected = fabs(dcOffset * dcShiftCoefficient);

		auto yn = doValveEmulationOld(xn, dcOffset * dcShiftCoefficient);

		float clippedBlend = fmin(0.5, blend) * 2.0f;

		yn = yn * clippedBlend + (og * (1.0f - clippedBlend));

		return yn;
	}

	SampleType doValveGridConductionOld(SampleType xn)
	{
		if (xn > 0.0f)
		{
			float clipDelta = xn - gridConductionThreshold;
			clipDelta = fmax(clipDelta, 0.0f);
			float compressionFactor = 0.4473253f + 0.5451584f * dsp::FastMathApproximations::exp(-0.3241584f * clipDelta);
			return compressionFactor * xn;
		}
		else
			return xn;
	}

	// --- main triode emulation - plenty of room here for experimentation
	SampleType doValveEmulationOld(SampleType xn, SampleType variableDCOffset)
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

				float cpPosSub = clipPointPositive - gridConductionThreshold;

				if (clipPointPositive > 1.0f)
					xn /= cpPosSub; // --- note that the signal should be clipped/compressed prior to calling this

				yn = xn * (1.5f) * (1.0f - (xn * xn) * 0.333333333f);
				yn *= cpPosSub;				   // --- scale by clip point positive
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

private:
	float sampleRate = 0.0f; ///< sample rate

	LossyIntegrator lossyIntegrator[2];

	dsp::ProcessorDuplicator<dsp::IIR::Filter<SampleType>, dsp::IIR::Coefficients<float>> lowShelvingFilter;
	dsp::ProcessorDuplicator<dsp::IIR::Filter<SampleType>, dsp::IIR::Coefficients<float>> dcBlockingFilter;
	dsp::ProcessorDuplicator<dsp::IIR::Filter<SampleType>, dsp::IIR::Coefficients<float>> upperBandwidthFilter1stOrder;
	dsp::ProcessorDuplicator<dsp::IIR::Filter<SampleType>, dsp::IIR::Coefficients<float>> upperBandwidthFilter2ndOrder;
};