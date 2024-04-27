#pragma once

#include "PhaseDist.h"

inline float weirdRectify(float x, float a) {
	return a * powf(abs(x) * 1.5f, 2.0f) + x * (1.0f - a);
}

//==============================================================================
PhaseDist::PhaseDist(juce::AudioProcessorValueTreeState& treeState) : 
	amount(treeState, ParamIDs::phaseAmount),
	tone(treeState, ParamIDs::phaseDistTone),
	normalise(treeState, ParamIDs::phaseDistNormalise),
	rectify(treeState, ParamIDs::phaseRectify)
	{};

void PhaseDist::prepare(juce::dsp::ProcessSpec& spec) noexcept {
	this->sampleRate = spec.sampleRate;

	amount.prepare(spec);
	tone.prepare(spec);
	normalise.prepare(spec);
	rectify.prepare(spec);

	*filter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(spec.sampleRate, 20000.0f, 0.707f);
	filter.prepare(spec);

	delayLine.setMaximumDelayInSamples(spec.sampleRate);
	delayLine.prepare(spec);
};

void PhaseDist::processBlock(juce::dsp::AudioBlock<float>& block) noexcept {
	TRACE_EVENT("dsp", "PhaseDist::processBlock");
	amount.update();
	tone.update();
	normalise.update();
	rectify.update();

	// we want to move to the delay line
	for (int i = 0; i < block.getNumSamples(); i++) {
		delayLine.pushSample(0, block.getSample(0, i));
		delayLine.pushSample(1, block.getSample(1, i));
	}

	// filter the phase buffer
	*filter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, tone.getRaw(), 1.1f);
	filter.process(juce::dsp::ProcessContextReplacing<float>(block));

	// apply the distortion
	for (int i = 0; i < block.getNumSamples(); i++) {
		const float nextAmt = amount.get() * 0.01f;
		auto amt = nextAmt * nextAmt * nextAmt * 1200.f;

		const float nextNormalise = normalise.get() * 10.0f;

		float left = block.getSample(0, i);
		float right = block.getSample(1, i);

		float mono = (fmin(left, right) + fmax(left, right)) / 2.0f;

		mono = weirdRectify(mono, rectify.get());

		float normalised = approxTanhWaveshaper1(mono, 4.0f + 0.00001f);

		auto phaseShiftL = (normalised + 1.0f) * amt * (sampleRate / 44100.0f);
		phaseShiftL = phaseShiftL + nextNormalise * sin(mono * 3.0f);
		
		auto leftProcessed = delayLine.popSample(0, phaseShiftL);
		auto rightProcessed = delayLine.popSample(1, phaseShiftL);

		block.setSample(0, i, leftProcessed);
		block.setSample(1, i, rightProcessed);
	}
};