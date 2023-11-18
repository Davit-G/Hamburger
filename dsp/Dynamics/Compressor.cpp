#include "Compressor.h"
#include "Curves.h"
#include <JuceHeader.h>

void Compressor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    envelope.prepareToPlay(sampleRate, samplesPerBlock);
}

void Compressor::processBlock(dsp::AudioBlock<float>& dryBuffer)
{
    double makeupGain = juce::Decibels::decibelsToGain(makeup_dB);

    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        float leftSample = dryBuffer.getSample(0, sample);
        float rightSample = dryBuffer.getSample(1, sample);

        auto envelopeVal = envelope.processSampleStereo(leftSample, rightSample);
        float gain = 0.0f;

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

        dryBuffer.setSample(0, sample, leftSample * gain * makeupGain);
        dryBuffer.setSample(1, sample, rightSample * gain * makeupGain);
    }
}

void Compressor::updateParameters(float attack, float release, float makeup, float ratio, float threshold, float kneeWidth, float makeup_dB)
{
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
}

void Compressor::updateParameters(float attack, float release, float makeup, float ratioLower, float ratioUpper, float thresholdLower, float thresholdUpper, float kneeWidth, float makeup_dB)
{
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
}