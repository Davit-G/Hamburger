#pragma once

#include "PhaseDist.h"

inline float weirdRectify(float x, float a) {
	return a * powf(abs(x) * 1.5f, 2.0f) + x * (1.0f - a);
}

//==============================================================================
PhaseDist::PhaseDist(juce::AudioProcessorValueTreeState& treeState) : 
	amount(treeState, ParamIDs::phaseAmount),
	tone(treeState, ParamIDs::phaseDistTone),
	stereo(treeState, ParamIDs::phaseDistStereo),
	rectify(treeState, ParamIDs::phaseRectify)
	{};

void PhaseDist::prepare(juce::dsp::ProcessSpec& spec) noexcept {
	this->sampleRate = spec.sampleRate;

	sampleRateMult = spec.sampleRate / 44100.0f;

	amount.prepare(spec);
	tone.prepare(spec);
	stereo.prepare(spec);
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
	stereo.update();
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

		const float nextStereo = stereo.get() * 2.0f;

		float l = block.getSample(0, i);
		float r = block.getSample(1, i);

		float mono = (fmin(l, r) + fmax(l, r)) / 2.0f;

		float left = l * nextStereo - mono;
		float right = r * nextStereo - mono;

		auto rectAmt = rectify.get();

		auto lRect = weirdRectify(left, rectAmt);
		auto rRect = weirdRectify(right, rectAmt);

		float normalisedL = approxTanhWaveshaper1(lRect, 4.00001f);
		float normalisedR = approxTanhWaveshaper1(rRect, 4.00001f);

		auto phaseShiftL = (normalisedL + 1.0f) * amt * sampleRateMult;
		auto phaseShiftR = (normalisedR + 1.0f) * amt * sampleRateMult;
		
		auto leftProcessed = delayLine.popSample(0, phaseShiftL);
		auto rightProcessed = delayLine.popSample(1, phaseShiftR);

		block.setSample(0, i, leftProcessed);
		block.setSample(1, i, rightProcessed);
	}
};