
#include "Compander.h"
#include "Curves.h"
#include <JuceHeader.h>

void Compander::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    compressorEnv.prepareToPlay(sampleRate, samplesPerBlock);
    expanderEnv.prepareToPlay(sampleRate, samplesPerBlock);
}

void Compander::processExpanderBlock(juce::AudioBuffer<float> &dryBuffer)
{
    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        float leftSample = dryBuffer.getSample(0, sample);
        float rightSample = dryBuffer.getSample(1, sample);

        auto envelope = expanderEnv.processSampleStereo(leftSample, rightSample);
        float gain = Curves::computeExpanderGain(envelope, ratio, kneeWidth);

        dryBuffer.setSample(0, sample, leftSample * gain);
        dryBuffer.setSample(1, sample, rightSample * gain);
    }
}

void Compander::processCompressorBlock(juce::AudioBuffer<float> &dryBuffer)
{
    double makeupGain = juce::Decibels::decibelsToGain(makeup_dB);

    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        float leftSample = dryBuffer.getSample(0, sample);
        float rightSample = dryBuffer.getSample(1, sample);

        auto envelope = compressorEnv.processSampleStereo(leftSample, rightSample);
        float gain = Curves::computeCompressorGain(envelope, threshold, ratio, kneeWidth);

        dryBuffer.setSample(0, sample, leftSample * gain * makeupGain);
        dryBuffer.setSample(1, sample, rightSample * gain * makeupGain);
    }
}

void Compander::processUpwardsDownwardsBlock(juce::AudioBuffer<float> &dryBuffer) {
    double makeupGain = juce::Decibels::decibelsToGain(makeup_dB);

    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        float leftSample = dryBuffer.getSample(0, sample);
        float rightSample = dryBuffer.getSample(1, sample);
        
        auto envelope = compressorEnv.processSampleStereo(leftSample, rightSample);
        float gain = Curves::computeUpwardsDownwardsGain(envelope, threshold, threshold, ratio, ratio, kneeWidth);

        dryBuffer.setSample(0, sample, leftSample * gain * makeupGain);
        dryBuffer.setSample(1, sample, rightSample * gain * makeupGain);
    }
}

void Compander::updateParameters()
{
    compressorEnv.setAttackTime(attackKnob->get());
    compressorEnv.setReleaseTime(releaseKnob->get());
    expanderEnv.setAttackTime(attackKnob->get());
    expanderEnv.setReleaseTime(releaseKnob->get());

    threshold = thresholdKnob->get();
    ratio = ratioKnob->get();
    makeup_dB = makeupKnob->get();
}