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

void Grunge::prepareToPlay(double sampleRate, int samplesPerBlock) {
	amount.prepareToPlay(sampleRate, samplesPerBlock);
    tone.prepareToPlay(sampleRate, samplesPerBlock);

    delayLine.prepare({ sampleRate, (juce::uint32)samplesPerBlock, 2 });
    delayLine.setMaximumDelayInSamples(1);
    delayLine.setDelay(1);
    dcBlockerL.prepare({ sampleRate, (juce::uint32)samplesPerBlock, 2 });
    dcBlockerR.prepare({ sampleRate, (juce::uint32)samplesPerBlock, 2 });
    dcBlockerL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 24.0f);
    dcBlockerR.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 24.0f);

    this->sampleRate = sampleRate;
}

void Grunge::processBlock(dsp::AudioBlock<float>& block) {
    TRACE_EVENT("dsp", "Grunge::processBlock");
	amount.update();
    tone.update();

    for (int i = 0; i < block.getNumSamples(); i++) {
        float fbKnobAmt = amount.getNextValue();
        float fbAmt = -powf(fbKnobAmt, 2.0f) + 2*fbKnobAmt;
        float toneFloat = tone.getNextValue() * 55 + 5;
    
        auto dcCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, toneFloat);

        dcBlockerL.coefficients = dcCoeffs;
        dcBlockerR.coefficients = dcCoeffs;

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