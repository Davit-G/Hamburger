#include "Grunge.h"

Grunge::Grunge(juce::AudioProcessorValueTreeState &treeState) : amount(treeState, "grungeAmt"),
                                                                tone(treeState, "grungeTone")
{
}

void Grunge::prepare(juce::dsp::ProcessSpec &spec)
{
    amount.prepare(spec);
    tone.prepare(spec);

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(1);
    delayLine.setDelay(1);
    dcBlockerL.prepare(spec);
    dcBlockerR.prepare(spec);
    *dcBlockerL.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(spec.sampleRate, 24.0f);
    *dcBlockerR.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(spec.sampleRate, 24.0f);

    this->sampleRate = spec.sampleRate;
}

void Grunge::processBlock(juce::dsp::AudioBlock<float> &block)
{
    TRACE_EVENT("dsp", "Grunge::processBlock");
    if (amount.getRaw() == 0.0f)
        return;

    amount.update();
    tone.update();

    float toneFloat = tone.getRaw() * 61.0f + 0.5f;
    auto dcCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(sampleRate, toneFloat);

    *dcBlockerL.coefficients = dcCoeffs;
    *dcBlockerR.coefficients = dcCoeffs;

    for (int i = 0; i < block.getNumSamples(); i++)
    {
        float fbKnobAmt = amount.getNextValue() * 0.975f;
        float fbAmt = -(fbKnobAmt * fbKnobAmt) + 2.0f * fbKnobAmt;

        float sampleL = block.getSample(0, i);
        float sampleR = block.getSample(1, i);

        float delaySampleL = delayLine.popSample(0);
        float delaySampleR = delayLine.popSample(1);

        float fbSampleL = delaySampleL * fbAmt;
        float fbSampleR = delaySampleR * fbAmt;

        float dcBlockerLRes = dcBlockerL.processSample(sampleL + fbSampleR);
        float dcBlockerRRes = dcBlockerR.processSample(sampleR + fbSampleL);

        delayLine.pushSample(0, dcBlockerLRes);
        delayLine.pushSample(1, dcBlockerRRes);

        block.setSample(0, i, juce::dsp::FastMathApproximations::tanh((sampleL + delaySampleL)* 0.1f * (1 - fbKnobAmt * 0.7f)) * 10.0f);
        block.setSample(1, i, juce::dsp::FastMathApproximations::tanh((sampleR + delaySampleR)* 0.1f * (1 - fbKnobAmt * 0.7f)) * 10.0f);
    }
}