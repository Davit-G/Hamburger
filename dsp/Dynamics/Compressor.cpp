#include "Compressor.h"
#include "Curves.h"
#include <JuceHeader.h>

void Compressor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    envelope.prepareToPlay(sampleRate, samplesPerBlock);
}

void Compressor::processBlock(dsp::AudioBlock<float>& dryBuffer)
{
    TRACE_DSP();
    double makeupGain = juce::Decibels::decibelsToGain(makeup_dB);

    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        float leftSample = dryBuffer.getSample(0, sample);
        float rightSample = dryBuffer.getSample(1, sample);

        TRACE_EVENT_BEGIN("dsp", "envelope.processSampleStereo");
        auto envelopeVal = envelope.processSampleStereo(leftSample, rightSample);
        TRACE_EVENT_END("dsp");
        float gain = 0.0f;

        TRACE_EVENT_BEGIN("dsp", "compute different gain types");
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
        TRACE_EVENT_END("dsp");

        TRACE_EVENT_BEGIN("dsp", "apply gain");
        dryBuffer.setSample(0, sample, leftSample * gain * makeupGain);
        dryBuffer.setSample(1, sample, rightSample * gain * makeupGain);
        TRACE_EVENT_END("dsp");
    }
}

void Compressor::updateParameters(float attack, float release, float makeup, float ratio, float threshold, float kneeWidth, float makeup_dB)
{
    TRACE_EVENT_BEGIN("dsp", "Compressor::updateParameters");
    envelope.setAttackTime(attack);
    envelope.setReleaseTime(release);
    this->attack = attack;
    this->release = release;
    this->makeup = makeup;
    this->ratioLower = ratio;
    this->ratioUpper = ratio;
    this->thresholdLower = threshold;
    this->thresholdUpper = threshold + 12.0f + kneeWidth * 2;
    this->kneeWidth = kneeWidth;
    this->makeup_dB = makeup;
    TRACE_EVENT_END("dsp");
}

void Compressor::updateParameters(float attack, float release, float makeup, float ratioLower, float ratioUpper, float thresholdLower, float thresholdUpper, float kneeWidth, float makeup_dB)
{
    TRACE_EVENT_BEGIN("dsp", "Compressor::updateParameters");
    envelope.setAttackTime(attack);
    envelope.setReleaseTime(release);
    this->attack = attack;
    this->release = release;
    this->makeup = makeup;
    this->ratioLower = ratioLower;
    this->ratioUpper = ratioUpper;
    this->thresholdLower = thresholdLower;
    this->thresholdUpper = thresholdUpper;
    this->kneeWidth = kneeWidth;
    this->makeup_dB = makeup;
    TRACE_EVENT_END("dsp");
}