
#include "Redux.h"
 

//==============================================================================
Redux::Redux(juce::AudioProcessorValueTreeState &treeState) : downsample(treeState, "downsampleFreq"),
															  jitter(treeState, "downsampleJitter"),
															  bitReduction(treeState, "bitReduction")
{
}

Redux::~Redux()
{
}

void Redux::prepare(juce::dsp::ProcessSpec &spec)
{
	downsample.prepare(spec);
	jitter.prepare(spec);
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
			*antialiasingFilter[i].state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(this->sampleRate, fmin(downsample.getRaw() * 2.f - 40.0f, 20000.0), 0.7f);
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
	jitter.update();

	for (int sample = 0; sample < block.getNumSamples(); sample++)
	{
		float dsmplFreq = downsample.get();
		float downsamplingValue = sampleRate * 0.5f / dsmplFreq;

		float bitReductionValue = bitReduction.get();
		float jitterAmount = jitter.get();

		// sample and hold process L channel
		if (floor(fmodf(sample, downsamplingValue + jitterOffsetL)) == 0)
		{
			auto xL = leftDryData[sample];
			auto xR = rightDryData[sample];
			float posValues = powf(2, bitReductionValue);
			xL = fmodf(xL, 1 / posValues); // bit reduction process
			xR = fmodf(xR, 1 / posValues); // bit reduction process
			xL = leftDryData[sample] - xL;
			xR = rightDryData[sample] - xR;

			heldSampleL = xL;
			jitterOffsetL = jitterAmount * (rand() / (float)RAND_MAX) * 0.3f; // different rand value for both channels results in stereo :D

			heldSampleR = xR;
			jitterOffsetR = jitterAmount * (rand() / (float)RAND_MAX) * 0.3f;
		}

		leftDryData[sample] = heldSampleL;
		rightDryData[sample] = heldSampleR;
	}
}