/*
  ==============================================================================

	Sizzle.cpp
	Created: 13 Jun 2021 6:32:32pm
	Author:  DavZ

  ==============================================================================
*/

#include "Sizzle.h"

//==============================================================================
Sizzle::Sizzle(juce::AudioProcessorValueTreeState &treeState)
	: envelopeDetector(true),
	  noiseAmount(treeState, ParamIDs::sizzleAmount),
	  filterTone(treeState, ParamIDs::sizzleFrequency),
	  fizzAmount(treeState, ParamIDs::fizzAmount),
	  filterQ(treeState, ParamIDs::sizzleQ) {}

Sizzle::~Sizzle()
{
}

void Sizzle::prepare(dsp::ProcessSpec &spec)
{
	this->sampleRate = spec.sampleRate;

	noiseAmount.prepare(spec);
	fizzAmount.prepare(spec);
	filterTone.prepare(spec);
	filterQ.prepare(spec);

	envelopeDetector.setAttackTime(6);
	envelopeDetector.setReleaseTime(2);
	envelopeDetector.prepare(spec);

	*filter.coefficients = dsp::IIR::ArrayCoefficients<float>::makeBandPass(spec.sampleRate, 10000.0f, 0.707f);
	filter.prepare(spec);
}

void Sizzle::processBlock(dsp::AudioBlock<float> &block)
{
	TRACE_EVENT("dsp", "Sizzle::processBlock");
	noiseAmount.update();
	if (noiseAmount.getRaw() == 0)
		return;

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	auto sizzleFreq = filterTone.getRaw();

	if (filterTone.isChanged())
		*filter.coefficients = dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, sizzleFreq, 0.707f);

	for (int sample = 0; sample < block.getNumSamples(); sample++)
	{
		float nextSizzle = noiseAmount.getNextValue() * 0.02f;
		float filtRandFloat = filter.processSample(random.nextFloat() * 2.0f - 1.0f) * 0.5f + 0.5f;

		float envelope = envelopeDetector.processSampleStereo(leftDryData[sample], rightDryData[sample]);
		float envelopeGain = juce::Decibels::decibelsToGain(envelope);

		rightDryData[sample] = newSizzleV3(rightDryData[sample], nextSizzle, envelopeGain, filtRandFloat);
		leftDryData[sample] = newSizzleV3(leftDryData[sample], nextSizzle, envelopeGain, filtRandFloat);
	}
}

void Sizzle::processBlockOG(dsp::AudioBlock<float> &block)
{
	TRACE_EVENT("dsp", "Sizzle::processBlock");
	fizzAmount.update();

	if (fizzAmount.getRaw() == 0)
		return;

	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	for (int sample = 0; sample < block.getNumSamples(); sample++)
	{
		float nextSizzle = fizzAmount.getNextValue() * 0.04f;

		rightDryData[sample] = oldSizzleFunction(rightDryData[sample], nextSizzle);
		leftDryData[sample] = oldSizzleFunction(leftDryData[sample], nextSizzle);
	}
}