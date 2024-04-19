
#include "Redux.h"
 

//==============================================================================
Redux::Redux(juce::AudioProcessorValueTreeState &treeState) : downsample(treeState, ParamIDs::downsampleFreq),
															  downsampleMix(treeState, ParamIDs::downsampleMix),
															  bitReduction(treeState, ParamIDs::bitReduction)
{
}

Redux::~Redux()
{
}

void Redux::prepare(juce::dsp::ProcessSpec &spec)
{
	downsample.prepare(spec);
	downsampleMix.prepare(spec);
	bitReduction.prepare(spec);
	this->sampleRate = spec.sampleRate;

	for (int i = 0; i < 4; i++)
	{
		*antialiasingFilter[i].state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, 7000.0f, 1.0f);
		antialiasingFilter[i].reset();
		antialiasingFilter[i].prepare(spec);
	}
}

void Redux::antiAliasingStep(juce::dsp::AudioBlock<float> &block)
{
	TRACE_EVENT("dsp", "Redux::antiAliasingStep");

	downsample.update();
	if (oldDownsample != downsample.getRaw())
	{
		for (int i = 0; i < 4; i++)
		{
			*antialiasingFilter[i].state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(this->sampleRate, fmin(downsample.getRaw() * 2.f - 40.0f, 20000.0), 0.7f);
		}
	}
	for (int i = 0; i < 4; i++)
	{
		antialiasingFilter[i].process(juce::dsp::ProcessContextReplacing<float>(block));
	}
}

void Redux::processBlock(juce::dsp::AudioBlock<float> &block)
{
	TRACE_EVENT("dsp", "Redux::processBlock");
	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	downsample.update();
	bitReduction.update();
	downsampleMix.update();

	for (int sample = 0; sample < block.getNumSamples(); sample++)
	{
		float dsmplFreq = downsample.get();
		float downsamplingValue = sampleRate * 0.5f / dsmplFreq;

		float bitReductionValue = bitReduction.getNextValue();
		float mixAmount = downsampleMix.getNextValue();

		// sample and hold process L channel
		if (floor(fmodf(sample, downsamplingValue)) == 0)
		{
			auto xL = leftDryData[sample];
			auto xR = rightDryData[sample];

			// float posValues = powf(2, bitReductionValue);

			// faster version?
			float amt = bitReductionValue;
    
			long long int result = 2;
			result <<= (int)amt;
			
			float remainder = powf(2, (float)(amt - (int)amt));
			float posValues = remainder * result;
			float invertPosValues = 1 / posValues;

			// xL = fmodf(xL, 1 / posValues); // bit reduction process
			// xR = fmodf(xR, 1 / posValues); // bit reduction process

			// faster version?
			xL = xL - (long long int)(xL * posValues) * invertPosValues;
			xR = xR - (long long int)(xR * posValues) * invertPosValues;

			xL = leftDryData[sample] - xL;
			xR = rightDryData[sample] - xR;

			heldSampleL = xL;
			heldSampleR = xR;
		}

		leftDryData[sample] = heldSampleL * mixAmount + leftDryData[sample] * (1 - mixAmount);
		rightDryData[sample] = heldSampleR * mixAmount + rightDryData[sample] * (1 - mixAmount);
	}
}