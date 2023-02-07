
#include "Compander.h"

#include <JuceHeader.h>

float Compander::computeExpanderGain(float input_db)
{
    float output = -100.0;

    if (2 * (input_db - threshold) > kneeWidth)
    {
        // below knee
        output = input_db;
    }
    else if (2 * (input_db - threshold) <= -kneeWidth)
    {
        output = threshold + (input_db - threshold) * ratio;
    }
    else
    {
        output = input_db + ((1 / ratio) * pow((input_db - threshold + kneeWidth / 2), 2) / (2 * kneeWidth));
    }

    float gainReduction = 0.f;

    gainReduction = output - input_db;

    return juce::Decibels::decibelsToGain(gainReduction);
}

float Compander::computeCompressorGain(float input_db)
{
    // this is the soft-clip function from the compressor
    // https://www.desmos.com/calculator/f8zazgtwpe

    // input is supposed to be in DB
    // output is supposed to be in gain

    float output = -100.0;

    if (2 * (input_db - threshold) < -kneeWidth)
    {
        // below knee
        output = input_db;
    }
    else if (2 * (input_db - threshold) > kneeWidth)
    {
        output = threshold + (input_db - threshold) / ratio;
    }
    else
    {
        output = input_db + ((1 / ratio - 1) * pow((input_db - threshold + kneeWidth / 2), 2) / (2 * kneeWidth));
    }


    float gainReduction = 0.f;

    gainReduction = output - input_db;

    return juce::Decibels::decibelsToGain(gainReduction);
}

void Compander::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    compressorEnv.prepareToPlay(sampleRate, samplesPerBlock);
    expanderEnv.prepareToPlay(sampleRate, samplesPerBlock);
}

void Compander::processExpanderBlock(juce::AudioBuffer<float> &dryBuffer)
{
    auto leftDryData = dryBuffer.getWritePointer(0);
    auto rightDryData = dryBuffer.getWritePointer(1);

    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        // --- get the envelope  ( d(n) )
        auto envelope = expanderEnv.processSampleStereo(leftDryData[sample], rightDryData[sample]);

        // --- compute the gain
        float gain = computeExpanderGain(envelope);

        // the compression step
        leftDryData[sample] *= gain;
        rightDryData[sample] *= gain;
    }
}

void Compander::processCompressorBlock(juce::AudioBuffer<float> &dryBuffer)
{
    auto leftDryData = dryBuffer.getWritePointer(0);
    auto rightDryData = dryBuffer.getWritePointer(1);

    // --- makeup gain, might need to change to smoothedValue soon
    double makeupGain = juce::Decibels::decibelsToGain(makeup_dB);

    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        // --- get the envelope  ( d(n) )
        auto envelope = compressorEnv.processSampleStereo(leftDryData[sample], rightDryData[sample]);

        // --- compute the gain
        float gain = computeCompressorGain(envelope);

        // the compression step
        leftDryData[sample] *= gain * makeupGain;
        rightDryData[sample] *= gain * makeupGain;
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