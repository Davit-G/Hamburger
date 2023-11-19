#include "Compressor.h"
#include "Curves.h"
#include <JuceHeader.h>

void Compressor::prepare(dsp::ProcessSpec& spec)
{
    envelope.prepare(spec);
}

void Compressor::processBlock(dsp::AudioBlock<float>& dryBuffer)
{
    TRACE_DSP();
    float makeupGain = juce::Decibels::decibelsToGain(makeup_dB);

    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        float leftSample = dryBuffer.getSample(0, sample);
        float rightSample = dryBuffer.getSample(1, sample);

        // TRACE_EVENT_BEGIN("dsp", "envelope.processSampleStereo");
        auto envelopeVal = envelope.processSampleStereo(leftSample, rightSample);
        // TRACE_EVENT_END("dsp");
        float gain = 0.0f;

        // TRACE_EVENT_BEGIN("dsp", "compute different gain types");
        switch (type)
        {
        case EXPANDER:
            gain = Curves::computeExpanderGain(envelopeVal, ratioLower, kneeWidth);
            break;
        case COMPRESSOR:
            gain = Curves::computeCompressorGain(envelopeVal, thresholdLower, ratioLower, kneeWidth);
            break;
        case UPWARDS_DOWNWARDS:
            gain = Curves::computeUpwardsDownwardsGain(envelopeVal, thresholdLower, thresholdUpper, ratioLower, ratioUpper, kneeWidth);
            break;
        }
        // TRACE_EVENT_END("dsp");

        // TRACE_EVENT_BEGIN("dsp", "apply gain");
        dryBuffer.setSample(0, sample, leftSample * gain * makeupGain);
        dryBuffer.setSample(1, sample, rightSample * gain * makeupGain);
        // TRACE_EVENT_END("dsp");
    }
}

void Compressor::updateParameters(float atk, float rel, float mkp, float rat, float thres, float knee, float mkpDB)
{
    TRACE_EVENT_BEGIN("dsp", "Compressor::updateParameters");
    envelope.setAttackTime(atk);
    envelope.setReleaseTime(rel);
    this->attack = atk;
    this->release = rel;
    this->makeup = mkp;
    this->ratioLower = rat;
    this->ratioUpper = rat;
    this->thresholdLower = thres;
    this->thresholdUpper = thres + 12.0f + knee * 2.0f;
    this->kneeWidth = knee;
    this->makeup_dB = mkp;
    TRACE_EVENT_END("dsp");
}

void Compressor::updateParameters(float atk, float rel, float mkp, float ratioLower, float ratioUpper, float thresholdLower, float thresholdUpper, float knee, float mkpDB)
{
    TRACE_EVENT_BEGIN("dsp", "Compressor::updateParameters");
    envelope.setAttackTime(atk);
    envelope.setReleaseTime(rel);
    this->attack = atk;
    this->release = rel;
    this->makeup = mkp;
    this->ratioLower = ratioLower;
    this->ratioUpper = ratioUpper;
    this->thresholdLower = thresholdLower;
    this->thresholdUpper = thresholdUpper;
    this->kneeWidth = knee;
    this->makeup_dB = mkp;
    TRACE_EVENT_END("dsp");
}