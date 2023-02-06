
#include "Compressor.h"

#include <JuceHeader.h>

float Dynamics::computeGain(float input_db)
{
    // this is the soft-clip function from the compressor
    // https://www.desmos.com/calculator/f8zazgtwpe

    // input is supposed to be in DB
    // output is supposed to be in gain

    float output = -100.0;

    if (compressorType == CompressionType::Expansion)
    {
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
    }
    else
    {
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
    }

    float gainReduction = 0.f;

    gainReduction = output - input_db;

    return juce::Decibels::decibelsToGain(gainReduction);
}

void Dynamics::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    envL.prepareToPlay(sampleRate, samplesPerBlock);
    envR.prepareToPlay(sampleRate, samplesPerBlock);
}

void Dynamics::processBlock(juce::AudioBuffer<float> &dryBuffer)
{
    auto leftDryData = dryBuffer.getWritePointer(0);
    auto rightDryData = dryBuffer.getWritePointer(1);

    for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++)
    {
        // input gain
        leftDryData[sample] *= pow(10.0, inputGain_dB / 20.0);
        rightDryData[sample] *= pow(10.0, inputGain_dB / 20.0);

        // --- get the envelope  ( d(n) )
        auto leftEnvelope = envL.processSample(leftDryData[sample]);
        auto rightEnvelope = envR.processSample(rightDryData[sample]);

        // --- makeup gain
        double makeupGain = pow(10.0, outputGain_dB / 20.0);

        // the compression step
        leftDryData[sample] *= computeGain(leftEnvelope) * makeupGain;
        rightDryData[sample] *= computeGain(rightEnvelope) * makeupGain;
    }
}