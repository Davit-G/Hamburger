#include "Grunge.h"

Grunge::Grunge(juce::AudioProcessorValueTreeState &treeState)
    : MacroEffect(treeState, SlotId{ModuleId::module2, 0}),
      amount(getParam(ParamIDs::grungeAmt)),
      tone(getParam(ParamIDs::grungeTone))
{
}

void Grunge::prepare(juce::dsp::ProcessSpec &spec)
{
    amount.prepare(spec);
    tone.prepare(spec);

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(1);
    delayLine.setDelay(1);
    // the same butterworth highpass the direct form one was
    dcBlocker.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    dcBlocker.setResonance(1.0f / juce::MathConstants<float>::sqrt2);
    dcBlocker.setCutoffFrequency(24.0f);
    dcBlocker.prepare(spec);
}

void Grunge::processBlock(juce::dsp::AudioBlock<float> &block)
{
    amount.update();
    tone.update();

    dcBlocker.setCutoffFrequency(tone.getRaw(0) * 61.0f + 0.5f);

    for (int i = 0; i < block.getNumSamples(); i++)
    {
        float fbKnobAmt = amount.getNextValue(0) * 0.975f;
        float fbAmt = -(fbKnobAmt * fbKnobAmt) + 2.0f * fbKnobAmt;

        float sampleL = block.getSample(0, i);
        float sampleR = block.getSample(1, i);

        float delaySampleL = delayLine.popSample(0);
        float delaySampleR = delayLine.popSample(1);

        float fbSampleL = delaySampleL * fbAmt;
        float fbSampleR = delaySampleR * fbAmt;

        float dcBlockerLRes = dcBlocker.processSample(0, sampleL + fbSampleR);
        float dcBlockerRRes = dcBlocker.processSample(1, sampleR + fbSampleL);

        delayLine.pushSample(0, dcBlockerLRes);
        delayLine.pushSample(1, dcBlockerRRes);

        block.setSample(0, i, juce::dsp::FastMathApproximations::tanh((sampleL + delaySampleL)* 0.1f * (1 - fbKnobAmt * 0.7f)) * 10.0f);
        block.setSample(1, i, juce::dsp::FastMathApproximations::tanh((sampleR + delaySampleR)* 0.1f * (1 - fbKnobAmt * 0.7f)) * 10.0f);
    }
}