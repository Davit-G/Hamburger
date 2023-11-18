/*
  ==============================================================================

	Redux.cpp
	Created: 8 Dec 2021 5:01:30pm
	Author:  DavZ

  ==============================================================================
*/

#include "Redux.h"
#include <JuceHeader.h>

//==============================================================================
Redux::Redux(juce::AudioProcessorValueTreeState &treeState) : downsample(treeState, "downsampleFreq"),
															  jitter(treeState, "downsampleJitter"),
															  bitReduction(treeState, "bitReduction")
{
}

Redux::~Redux()
{
}

void Redux::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	downsample.prepareToPlay(sampleRate, samplesPerBlock);
	jitter.prepareToPlay(sampleRate, samplesPerBlock);
	bitReduction.prepareToPlay(sampleRate, samplesPerBlock);
	this->sampleRate = sampleRate;

	dsp::ProcessSpec spec;
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = 2;

	for (int i = 0; i < 4; i++)
	{
		*antialiasingFilter[i].state = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 7000, 1.0f);
		antialiasingFilter[i].reset();
		antialiasingFilter[i].prepare(spec);
	}
}

void Redux::antiAliasingStep(dsp::AudioBlock<float> &block)
{
	downsample.update();
	for (int i = 0; i < 4; i++)
	{
		*antialiasingFilter[i].state = *dsp::IIR::Coefficients<float>::makeLowPass(this->sampleRate, fmin(downsample.getRaw() * 2.f - 40.0f, 20000.0), 0.7f);
		antialiasingFilter[i].process(juce::dsp::ProcessContextReplacing<float>(block));
	}
}

void Redux::processBlock(dsp::AudioBlock<float> &block)
{
	
	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	downsample.update();
	bitReduction.update();
	jitter.update();

	
	for (int sample = 0; sample < block.getNumSamples(); sample++)
	{
		float dsmplFreq = downsample.get();
		float downsamplingValue = sampleRate / (2 * dsmplFreq);

		float bitReductionValue = bitReduction.get();
		float jitterAmount = jitter.get();

		if (downsamplingValue == 0.0f)
		{
			downsamplingValue = 0.0001f;
		}

		auto x = rightDryData[sample];
		float posValues = powf(2, bitReductionValue);
		x = fmodf(x, 1 / posValues); // bit reduction process

		// sample and hold process
		if (floor(fmodf(sample, downsamplingValue + jitterOffsetL)) == 0)
		{
			x = rightDryData[sample] - x;
			jitterOffsetL = jitterAmount * (rand() / (float)RAND_MAX) * 0.3;
		}
		else
		{
			x = rightDryData[sample - int(floor(fmodf(sample, downsamplingValue + jitterOffsetL)))];
		}

		rightDryData[sample] = x;

		x = leftDryData[sample];
		posValues = powf(2, bitReductionValue); // all the possible vertical steps;
		x = fmodf(x, 1 / posValues);			// bit reduction process

		// sample and hold process
		if (floor(fmodf(sample, downsamplingValue + jitterOffsetR)) == 0)
		{
			x = leftDryData[sample] - x;
			jitterOffsetR = jitterAmount * (rand() / (float)RAND_MAX) * 0.3;
		}
		else
		{
			x = leftDryData[sample - int(floor(fmodf(sample, downsamplingValue + jitterOffsetR)))];
		}

		leftDryData[sample] = x;
	}
}