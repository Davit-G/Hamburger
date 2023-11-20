#include <JuceHeader.h>
#include "LossyIntegrator.h"

#include <cmath>

static dsp::SIMDRegister<float> vectanh(dsp::SIMDRegister<float> x) noexcept
{
	auto x2 = x * x;
	auto numerator = x * (x2 * (x2 * (x2 + 378) + 17325) + 135135);
	auto denominator = x2 * (x2 * (x2 * 28 + 3150) + 62370) + 135135;

	auto ting = dsp::SIMDRegister<float>::expand(0.0f); 
	for (int i = 0; i < ting.size(); i++)
	{
		ting[i] = numerator[i] / denominator[i];
	}
	return ting;
}

static dsp::SIMDRegister<float> vecexp(dsp::SIMDRegister<float> x) noexcept
{
	auto numerator = x * (x * (x * (x + 20) + 180) + 840) + 1680;
	auto denominator = x * (x * (x * (x - 20) + 180 - 840) + 1680);

	auto ting = dsp::SIMDRegister<float>::expand(0.0f); 
	for (int i = 0; i < ting.size(); i++)
	{
		ting[i] = numerator[i] / denominator[i];
	}
	return ting;
}

class ClassAValve
{
public:
	inline ClassAValve(void) {}
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
		*lowShelvingFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, lowFrequencyShelf_Hz, 0.707f, juce::Decibels::decibelsToGain(lowFrequencyShelfGain_dB));
		;

		// --- output HPF
		*dcBlockingFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, dcBlockingLF_Hz, 0.707f);
		;

		// --- LPF (upper edge), technically supposed to be second order
		auto upperBandwidthFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, millerHF_Hz, 0.707f);
		*upperBandwidthFilter1stOrder.state = *upperBandwidthFilterCoeffs;
		*upperBandwidthFilter2ndOrder.state = *upperBandwidthFilterCoeffs;
	}

	void processBlock(dsp::AudioBlock<float> &block)
	{
		// TRACE_DSP();
		
		// left channel
		auto leftChannel = block.getChannelPointer(0);
		for (int i = 0; i < block.getNumSamples(); i++)
		{
			leftChannel[i] = processAudioSampleOld(leftChannel[i], 0);
		}

		// right channel
		auto rightChannel = block.getChannelPointer(1);
		for (int i = 0; i < block.getNumSamples(); i++)
		{
			rightChannel[i] = processAudioSampleOld(rightChannel[i], 1);
		}

		dcBlockingFilter.process(dsp::ProcessContextReplacing<float>(block));			  // --- remove DC
		lowShelvingFilter.process(dsp::ProcessContextReplacing<float>(block));			  // --- LF Shelf
		upperBandwidthFilter1stOrder.process(dsp::ProcessContextReplacing<float>(block)); // --- HF Edge
		upperBandwidthFilter2ndOrder.process(dsp::ProcessContextReplacing<float>(block));
		block.multiplyBy(-outputGain); // --- (5) final output scaling and inversion
									   // TRACE_EVENT_END("dsp");
	}

	// void processBlockSIMD(dsp::AudioBlock<float> &block)
	// {
	// 	// TRACE_DSP();

	// 	// prepare for SIMD iteration
	// 	auto size = dsp::SIMDRegister<float>::size(); // --- get the SIMD vector size
	// 	auto numSamples = block.getNumSamples();	  // --- get the number of samples in the block
	// 	auto remainder = numSamples % size;			  // --- get the remainder (0-3)

	// 	// left channel
	// 	auto *x = block.getChannelPointer(0); // --- get the left input channel

	// 	for (int i = 0; i < numSamples - remainder; i += size)
	// 	{
	// 		auto xn = dsp::SIMDRegister<float>::fromRawArray(x + i); // --- load input samples (x)
	// 		auto yn = processAudioSample(xn);						 // --- process samples
	// 		// yn.copyToRawArray(x + i);								 // --- store output samples (y)
			
	// 		for (int j = 0; j < size; j++)
	// 		{
	// 			x[i + j] = yn[j];
	// 		}
	// 	}

	// 	dsp::SIMDRegister<float> xn = dsp::SIMDRegister<float>::fromRawArray(x + numSamples - remainder);
	// 	// 

	// 	auto leftoverL = processAudioSample(xn);
	// 	for (int i = 0; i < remainder; i++)
	// 	{
	// 		x[numSamples - remainder + i] = leftoverL[i];
	// 	}

	// 	// right channel
	// 	x = block.getChannelPointer(1); // --- get the right input channel

	// 	for (int i = 0; i < numSamples - remainder; i += size)
	// 	{
	// 		auto xn = dsp::SIMDRegister<float>::fromRawArray(x + i); // --- load input samples (x)
	// 		auto yn = processAudioSample(xn);						 // --- process samples
	// 		// yn.copyToRawArray(x + i);								 // --- store output samples (y)

	// 		for (int j = 0; j < size; j++)
	// 		{
	// 			x[i + j] = yn[j];
	// 		}
	// 	}

	// 	dsp::SIMDRegister<float> xn2 = dsp::SIMDRegister<float>::fromRawArray(x + numSamples - remainder);
	// 	// process leftover samples
	// 	auto leftoverR = processAudioSample(xn2);
	// 	for (int i = 0; i < remainder; i++)
	// 	{
	// 		x[numSamples - remainder + i] = leftoverR[i];
	// 	}

	// 	dcBlockingFilter.process(dsp::ProcessContextReplacing<float>(block));			  // --- remove DC
	// 	lowShelvingFilter.process(dsp::ProcessContextReplacing<float>(block));			  // --- LF Shelf
	// 	upperBandwidthFilter1stOrder.process(dsp::ProcessContextReplacing<float>(block)); // --- HF Edge
	// 	upperBandwidthFilter2ndOrder.process(dsp::ProcessContextReplacing<float>(block));
	// 	block.multiplyBy(-outputGain); // --- (5) final output scaling and inversion
	// 								   // TRACE_EVENT_END("dsp");
	// }

	// --- do the valve emulation
	// dsp::SIMDRegister<float> processAudioSample(dsp::SIMDRegister<float> xn)
	// {
	// 	xn *= inputGain;				// --- input scaling
	// 	dsp::SIMDRegister<float> yn = doValveGridConduction(xn); // grid conduction check, must be done prior to waveshaping
	// 	dsp::SIMDRegister<float> dcOffset = lossyIntegrator.processAudioSample(yn); // detect the DC offset that the clipping may have caused
	// 	dcOffset = dsp::SIMDRegister<float>::min(dcOffset, 0.0f);  // --- process only negative DC bias shifts

	// 	// yn = doValveEmulation(yn, dcOffsetDetected);

	// 	auto out = dsp::SIMDRegister<float>::expand(0.0f);
	// 	for (int i = 0; i < xn.size(); i++)
	// 	{
	// 		// out[i] = doValveEmulationOld(yn.get(i), dcOffset.get(i));
	// 		out[i] = yn[i];
	// 	}

	// 	return out;
	// }

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

	// --- do the valve emulation
	float processAudioSampleOld(float xn, int channel = 0)
	{
		// TRACE_DSP();

		float yn = 0.0f;

		// TRACE_EVENT_BEGIN("dsp", "valve grid conduction check");
		xn *= inputGain;				   // --- input scaling
		xn = doValveGridConductionOld(xn); // grid conduction check, must be done prior to waveshaping
		// TRACE_EVENT_END("dsp");

		// TRACE_EVENT_BEGIN("dsp", "lossy integrator dc offset");
		// float dcOffset = (lossyIntegrator).processAudioSample(dsp::SIMDRegister<float>(xn)).get(0); // detect the DC offset that the clipping may have caused
		float dcOffset = lossyIntegrator[channel].processAudioSample(xn); // detect the DC offset that the clipping may have caused
		dcOffset = fmin(dcOffset, 0.0f);															   // --- process only negative DC bias shifts
		// TRACE_EVENT_END("dsp");

		// --- save this - user may indicate it in a meter if they want
		//     Note that this is a bipolar value, but we only do DC shift for
		//     *negative* values so meters should be aware
		dcOffsetDetected = fabs(dcOffset * dcShiftCoefficient);

		// the emulation itself
		// TRACE_EVENT_BEGIN("dsp", "valve emulation");
		yn = doValveEmulationOld(xn, dcOffsetDetected);
		// TRACE_EVENT_END("dsp");

		// TRACE_EVENT_BEGIN("dsp", "filters");
		// yn = dcBlockingFilter.processSample(yn); // --- remove DC
		// yn = lowShelvingFilter.processSample(yn); // --- LF Shelf
		// yn = upperBandwidthFilter1stOrder.processSample(yn); // --- HF Edge
		// yn = upperBandwidthFilter2ndOrder.processSample(yn);
		// yn *= -outputGain; // --- (5) final output scaling and inversion
		// TRACE_EVENT_END("dsp");

		return yn;
	}

	float doValveGridConductionOld(float xn)
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

	dsp::SIMDRegister<float> select(dsp::SIMDRegister<uint32_t> condition, dsp::SIMDRegister<float> trueVal, dsp::SIMDRegister<float> falseVal)
	{
		return (trueVal & condition) + (falseVal & (~condition));
	}

	// --- main triode emulation - plenty of room here for experimentation
	dsp::SIMDRegister<float> doValveEmulation(dsp::SIMDRegister<float> xn, dsp::SIMDRegister<float> variableDCOffset)
	{
		auto oneReg = dsp::SIMDRegister<float>::expand(1.0f);
		auto negOneReg = dsp::SIMDRegister<float>::expand(-1.0f);
		auto zeroReg = dsp::SIMDRegister<float>::expand(0.0f);

		
		xn += variableDCOffset; // --- add the offset detected
		auto yn = zeroReg;

		// --- NOTE: the whole transfer function is shifted so that the normal
		//           DC operating point is 0.0 to prevent massive
		//           clicks at the beginning of a selection due to the temporary DC offset
		//           that occurs before the HPF can react
		//

		float absCPNegative = fabs(clipPointNegative);
		float cpPosSub = clipPointPositive - gridConductionThreshold;


		auto gridConductionCondition = dsp::SIMDRegister<float>::greaterThan(xn, gridConductionThreshold);
		auto waveshaperCondition = dsp::SIMDRegister<float>::lessThanOrEqual(xn, clipPointPositive) & (~gridConductionCondition);
		auto ultraLinearCondition = dsp::SIMDRegister<float>::greaterThanOrEqual(xn, zeroReg) & (~dsp::SIMDRegister<float>::lessThanOrEqual(xn, clipPointPositive)) & (~gridConductionCondition);

		auto gridCondYn = zeroReg;
		auto waveShaperYn = zeroReg;
		auto ultraLinear = zeroReg;

		auto underPositive = dsp::SIMDRegister<float>::lessThanOrEqual(xn, clipPointPositive);
		auto overNegative = dsp::SIMDRegister<float>::greaterThanOrEqual(xn, clipPointNegative);

		auto negUnder = dsp::SIMDRegister<float>::lessThan(clipPointNegative, negOneReg);
		auto posOver = dsp::SIMDRegister<float>::greaterThan(clipPointPositive, oneReg);

		auto scaledXn = xn - gridConductionThreshold; // --- scaling to get into the first quadrant for Arraya @ (0,0)

		auto waveshaperXn = xn;
		scaledXn *= select(posOver, dsp::SIMDRegister<float>::expand(1 / cpPosSub), oneReg);
		waveshaperXn *= select(negUnder, dsp::SIMDRegister<float>::expand(1 / absCPNegative), oneReg);

		gridCondYn = select(underPositive,
							(scaledXn * (1.5f)) * ((scaledXn * scaledXn) * -0.333333333f + 1.0f) * cpPosSub + gridConductionThreshold,
							dsp::SIMDRegister<float>::expand(clipPointPositive));

		waveShaperYn = select(overNegative,
							  (vectanh(waveshaperXn * waveshaperSaturation) * dsp::SIMDRegister<float>::expand(absCPNegative)) * dsp::SIMDRegister<float>::expand(1 / tanh(waveshaperSaturation)),
							  dsp::SIMDRegister<float>::expand(clipPointNegative));

		ultraLinear = xn; // --- fundamentally linear region of 3/2 power law

		return (gridCondYn & gridConductionCondition) + (waveShaperYn & waveshaperCondition) + (ultraLinear & ultraLinearCondition);
	}
	// --- main triode emulation - plenty of room here for experimentation
	float doValveEmulationOld(float xn, float variableDCOffset)
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

	dsp::SIMDRegister<float> doValveGridConduction(dsp::SIMDRegister<float> xn)
	{
		// auto mask = dsp::SIMDRegister<float>::greaterThan(xn, dsp::SIMDRegister<float>::expand(0.0f));

		// auto vn = dsp::SIMDRegister<float>::expand(0.0f)

		// auto clipDelta = xn - dsp::SIMDRegister<float>::expand(gridConductionThreshold);
		// auto otherClipDelta = dsp::SIMDRegister<float>::max(clipDelta, dsp::SIMDRegister<float>::expand(0.0f));
		// auto compressionFactor = xn * (exp(otherClipDelta * -0.3241584f) * 0.5451584f + 0.4473253f);

		// return select(mask, compressionFactor, xn);
		// return (compressionFactor & mask) + (xn & (~mask));
		return xn;
	}

	// // --- main triode emulation - plenty of room here for experimentation
	// dsp::SIMDRegister<float> doValveEmulation(dsp::SIMDRegister<float> xn, dsp::SIMDRegister<float> variableDCOffset)
	// {
	// 	xn += variableDCOffset; // --- add the offset detected
	// 	auto yn = dsp::SIMDRegister<float>(0.0f);

	// 	auto gridConductionCondition = dsp::SIMDRegister<float>::greaterThan(xn, gridConductionThreshold);
	// 	auto ultraLinearCondition = dsp::SIMDRegister<float>::greaterThan(xn, 0.0f);
	// 	auto abovePositiveClipPoint = dsp::SIMDRegister<float>::greaterThan(xn, clipPointPositive);
	// 	auto belowNegativeClipPoint = dsp::SIMDRegister<float>::lessThan(xn, clipPointNegative);

	// 	// Clip point positive calculation
	// 	auto clipPointPositiveCalc = select(gridConductionCondition & abovePositiveClipPoint, dsp::SIMDRegister<float>(clipPointPositive), dsp::SIMDRegister<float>(0.0f));

	// 	// The other stuff
	// 	auto newXn = select(gridConductionCondition & ~abovePositiveClipPoint, xn - gridConductionThreshold, dsp::SIMDRegister<float>(0.0f));
	// 	auto minused = clipPointPositive - gridConductionThreshold;
	// 	if (minused != 0.0f)
	// 	{ // Avoid division by zero
	// 		newXn = newXn * (((newXn * newXn) * -0.333333333f + 1.0f) * 1.5f) * (1 / minused) + gridConductionThreshold;
	// 	}

	// 	// Ultra linear calculation
	// 	auto ultraLinearCalc = select(ultraLinearCondition & ~gridConductionCondition, xn, dsp::SIMDRegister<float>(0.0f));

	// 	// Clip point negative calculation
	// 	auto clipPointNegativeCalc = select(belowNegativeClipPoint, dsp::SIMDRegister<float>(clipPointNegative), dsp::SIMDRegister<float>(0.0f));

	// 	// Bottom portion tanh waveshaper
	// 	auto newXn2 = select(~belowNegativeClipPoint & ~gridConductionCondition, xn, dsp::SIMDRegister<float>(0.0f));
	// 	auto absClip = fabs(clipPointNegative);
	// 	if (absClip != 0.0f)
	// 	{ // Avoid division by zero
	// 		newXn2 = vectanh(newXn2 * waveshaperSaturation) * (1 / dsp::FastMathApproximations::tanh(waveshaperSaturation)) * absClip;
	// 	}

	// 	// Combine all parts
	// 	yn = clipPointPositiveCalc + newXn + ultraLinearCalc + clipPointNegativeCalc + newXn2;

	// 	return yn;
	// }

	LossyIntegrator lossyIntegrator[2];

	dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> lowShelvingFilter;
	dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> dcBlockingFilter;
	dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> upperBandwidthFilter1stOrder;
	dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> upperBandwidthFilter2ndOrder;
};