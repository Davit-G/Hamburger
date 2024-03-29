#pragma once

#include "PhaseDist.h"

//==============================================================================
PhaseDist::PhaseDist(juce::AudioProcessorValueTreeState& treeState) : 
	amount(treeState, "phaseAmount"),
	tone(treeState, "phaseDistTone"),
	normalise(treeState, "phaseDistNormalise")
	{};

void PhaseDist::prepare(dsp::ProcessSpec& spec) {
	this->sampleRate = spec.sampleRate;

	amount.prepare(spec);
	tone.prepare(spec);
	normalise.prepare(spec);

	*filter.state = dsp::IIR::ArrayCoefficients<float>::makeLowPass(spec.sampleRate, 20000.0f, 0.707f);
	filter.prepare(spec);

	delayLine.setMaximumDelayInSamples(spec.sampleRate);
	delayLine.prepare(spec);
};

void PhaseDist::processBlock(dsp::AudioBlock<float>& block) {
	TRACE_EVENT("dsp", "PhaseDist::processBlock");
	amount.update();
	tone.update();
	normalise.update();

	// we want to move to the delay line
	for (int i = 0; i < block.getNumSamples(); i++) {
		delayLine.pushSample(0, block.getSample(0, i));
		delayLine.pushSample(1, block.getSample(1, i));
	}

	// filter the phase buffer
	*filter.state = dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, tone.getRaw(), 0.707f);
	filter.process(dsp::ProcessContextReplacing<float>(block));

	// apply the distortion
	for (int i = 0; i < block.getNumSamples(); i++) {
		auto amt = (amount.get()) / 100.0f;

		float left = block.getSample(0, i);
		float right = block.getSample(1, i);

		float mono = (fmin(left, right) + fmax(left, right)) / 2.0f;

		float normalised = tanhWaveShaper(mono, normalise.getNextValue() * 10.0f + 0.00001f);

		auto phaseShiftL = (normalised + 2.0f) * amt * 50;
		// auto phaseShiftR = (right + 2.0f) * amt * 40;
		
		auto leftProcessed = delayLine.popSample(0, phaseShiftL);
		auto rightProcessed = delayLine.popSample(1, phaseShiftL);

		block.setSample(0, i, leftProcessed);
		block.setSample(1, i, rightProcessed);
	}
};