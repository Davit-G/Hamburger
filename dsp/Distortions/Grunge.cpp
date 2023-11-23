/*
  ==============================================================================

    Patty.cpp
    Created: 13 Jun 2021 6:45:25pm
    Author:  DavZ

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Grunge.h"

//==============================================================================
Grunge::Grunge(juce::AudioProcessorValueTreeState& treeState) :
    amount(treeState, "grungeAmt"),
    tone(treeState, "grungeTone") 
{}

void Grunge::prepare(dsp::ProcessSpec& spec) {
	amount.prepare(spec);
    tone.prepare(spec);

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(1);
    delayLine.setDelay(1);
    dcBlockerL.prepare(spec);
    dcBlockerR.prepare(spec);
    dcBlockerL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 24.0f);
    dcBlockerR.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 24.0f);

    this->sampleRate = spec.sampleRate;
}

void Grunge::processBlock(dsp::AudioBlock<float>& block) {
    TRACE_EVENT("dsp", "Grunge::processBlock");
    if (amount.getRaw() == 0.0f) return;

	amount.update();
    tone.update();

    float toneFloat = tone.getRaw() * 55.0f + 5.0f;
    auto dcCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, toneFloat);

    dcBlockerL.coefficients = dcCoeffs;
    dcBlockerR.coefficients = dcCoeffs;

    for (int i = 0; i < block.getNumSamples(); i++) {
        float fbKnobAmt = amount.getNextValue();
        float fbAmt = - (fbKnobAmt * fbKnobAmt) + 2.0f * fbKnobAmt;

        float sampleL = block.getSample(0, i);
        float sampleR = block.getSample(1, i);

        float delaySampleL = delayLine.popSample(0);
        float delaySampleR = delayLine.popSample(1);

        float fbSampleL = delaySampleL * fbAmt;
        float fbSampleR = delaySampleR * fbAmt;

        delayLine.pushSample(0, dcBlockerL.processSample(sampleL + fbSampleR));
        delayLine.pushSample(1, dcBlockerR.processSample(sampleR + fbSampleL));

        block.setSample(0, i, sampleL + delaySampleL);
        block.setSample(1, i, sampleR + delaySampleR);
    }
}