#pragma once 
#include "LossyIntegrator.h"

#include <cmath>
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"

template <typename SampleType>
class ClassAValve
{
public:
	ClassAValve(void) {}
	~ClassAValve(void) {}

public:
	void prepare(juce::dsp::ProcessSpec &spec)
	{
		sampleRate = spec.sampleRate;

		lossyIntegrator[0].calculateFilterCoeffs();
		lossyIntegrator[0].prepare(spec);
		lossyIntegrator[1].calculateFilterCoeffs();
		lossyIntegrator[1].prepare(spec);

		calculateCoefficients();

		lowShelvingFilter[0].prepare(spec);
		dcBlockingFilter[0].prepare(spec);
		upperBandwidthFilter1stOrder[0].prepare(spec);
		upperBandwidthFilter2ndOrder[0].prepare(spec);

		lowShelvingFilter[1].prepare(spec);
		dcBlockingFilter[1].prepare(spec);
		upperBandwidthFilter1stOrder[1].prepare(spec);
		upperBandwidthFilter2ndOrder[1].prepare(spec);
	}

	void calculateCoefficients()
	{
		// --- low shelf
		*lowShelvingFilter[0].coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(sampleRate, lowFrequencyShelf_Hz, 0.707f, juce::Decibels::decibelsToGain(lowFrequencyShelfGain_dB));
		lowShelvingFilter[1].coefficients = lowShelvingFilter[0].coefficients;

		// --- output HPF
		*dcBlockingFilter[0].coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(sampleRate, dcBlockingLF_Hz);
		dcBlockingFilter[1].coefficients = dcBlockingFilter[0].coefficients;

		// --- LPF (upper edge), technically supposed to be second order
		auto upperBandwidthFilterCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, millerHF_Hz, 0.89f);
		*upperBandwidthFilter1stOrder[0].coefficients = upperBandwidthFilterCoeffs;
		*upperBandwidthFilter2ndOrder[0].coefficients = upperBandwidthFilterCoeffs;
		upperBandwidthFilter1stOrder[1].coefficients = upperBandwidthFilter1stOrder[0].coefficients;
		upperBandwidthFilter2ndOrder[1].coefficients = upperBandwidthFilter2ndOrder[0].coefficients;
	}

	void processBlock(const juce::dsp::ProcessContextReplacing<SampleType> &context)
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
				output[i] = processAudioSample(input[i], ch);
			}
		}

		dcBlockingFilter.process(context);			  // --- remove DC
		lowShelvingFilter.process(context);			  // --- LF Shelf
		upperBandwidthFilter1stOrder.process(context); // --- HF Edge
		// upperBandwidthFilter2ndOrder.process(context);
		outputBlock.multiplyBy(-outputGain); // --- (5) final output scaling and inversion
	}

	/**
	 * Process only the stuff that happens at the end of the processing chain
	*/
	void processEnd(juce::dsp::ProcessContextReplacing<SampleType> &context)
	{
		dcBlockingFilter.process(context);			  // --- remove DC
		lowShelvingFilter.process(context);			  // --- LF Shelf
		upperBandwidthFilter1stOrder.process(context); // --- HF Edge
		// upperBandwidthFilter2ndOrder.process(context);
		context.getOutputBlock().multiplyBy(-outputGain); // --- (5) final output scaling and inversion
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
	SampleType processAudioSample(SampleType xn, int channel)
	{

		auto og = xn;
		xn *= inputGain;

		// xn = (abs(dcShiftAdditional) + 1)*(dcShiftAdditional * - (xn * xn) + xn);
		xn = doValveGridConduction(xn); // grid conduction check, must be done prior to waveshaping

		// float dcOffset = (lossyIntegrator).processAudioSample(dsp::SIMDRegister<float>(xn)).get(0); // detect the DC offset that the clipping may have caused
		float dcOffset = lossyIntegrator[channel].processAudioSample(xn); // detect the DC offset that the clipping may have caused
		dcOffset = fmin(dcOffset, 0.0f);

		// --- save this - user may indicate it in a meter if they want
		//     Note that this is a bipolar value, but we only do DC shift for
		//     *negative* values so meters should be aware
		// dcOffsetDetected = fabs(dcOffset * dcShiftCoefficient);

		auto yn = doValveEmulation(xn, dcOffset * dcShiftCoefficient * 0.0f);

		// float clippedBlend = fmin(0.5, blend) * 2.0f;

		// yn = yn * clippedBlend + (og * (1.0f - clippedBlend));

		yn = dcBlockingFilter[channel].processSample(yn);			  // --- remove DC
		yn = lowShelvingFilter[channel].processSample(yn);			  // --- LF Shelf
		yn = upperBandwidthFilter1stOrder[channel].processSample(yn); // --- HF Edge

		// upperBandwidthFilter2ndOrder.process(context);

		yn *= -outputGain; // --- (5) final output scaling and inversion

		return yn;
	}

	SampleType doValveGridConduction(SampleType xn)
	{
		if (xn > 0.0f)
		{
			float clipDelta = xn - gridConductionThreshold;
			clipDelta = fmax(clipDelta, 0.0f);
			float compressionFactor = 0.4473253f + 0.5451584f * juce::dsp::FastMathApproximations::exp(-0.3241584f * clipDelta);
			return compressionFactor * xn;
		}
		else
			return xn;
	}

	// --- main triode emulation - plenty of room here for experimentation
	SampleType doValveEmulation(SampleType xn, SampleType variableDCOffset)
	{
		xn += variableDCOffset; // --- add the offset detected
		float yn = 0.0f;

		if (xn > gridConductionThreshold)
		{
			if (xn > clipPointPositive)
				yn = clipPointPositive;
			else
			{
				xn -= gridConductionThreshold;

				float cpPosSub = clipPointPositive - gridConductionThreshold;

				if (clipPointPositive > 1.0f)
					xn /= cpPosSub;

				yn = xn * (1.5f) * (1.0f - (xn * xn) * 0.333333333f);
				yn *= cpPosSub;
				yn += gridConductionThreshold;
			}
		}
		else if (xn > 0.0f)
		{
			yn = xn; 
		}
		else 
		{
			if (xn < clipPointNegative)
				yn = clipPointNegative;
			else
			{
				float absCPNegative = fabs(clipPointNegative);

				if (clipPointNegative < -1.0f) // --- clip normalize
					xn /= absCPNegative;

				yn = juce::dsp::FastMathApproximations::tanh(waveshaperSaturation * xn) /
					 juce::dsp::FastMathApproximations::tanh(waveshaperSaturation);

				yn *= absCPNegative; // --- undo clip normalize
			}
		}

		return yn;
	}

private:
	float sampleRate = 44100.0f; ///< sample rate

	LossyIntegrator lossyIntegrator[2];

	juce::dsp::IIR::Filter<SampleType> lowShelvingFilter[2];
	juce::dsp::IIR::Filter<SampleType> dcBlockingFilter[2];
	juce::dsp::IIR::Filter<SampleType> upperBandwidthFilter1stOrder[2];
	juce::dsp::IIR::Filter<SampleType> upperBandwidthFilter2ndOrder[2];
};