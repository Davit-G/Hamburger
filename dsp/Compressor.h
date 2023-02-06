#pragma once

#include "EnvelopeFollower.h"

#include <JuceHeader.h>

enum CompressionType
{
    Compression,
    Expansion
};

class Dynamics
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state, CompressionType type, float newRatio = 100.f, float newThreshold = -20.f, float newKneeWidth = 1.f, float inputGain = 0.f) : 
        envL(state, true), envR(state, true)
    {
        compressorType = type;

        // if (compressorType == CompressionType::Expansion)
        // {
        //     // outputGain_dB = -12.f;
        //     ratio = 3.f;
        // }

        ratio = newRatio;
        threshold = newThreshold;
        kneeWidth = newKneeWidth;

        inputGain_dB = inputGain;

    };
    ~Dynamics() {};

    void processBlock(juce::AudioBuffer<float>& dryBuffer);

    void prepareToPlay(double sampleRate, int samplesPerBlock);


private:

    float computeGain(float input_db);

    float ratio;
    float threshold;
    float kneeWidth;

    float inputGain_dB = 0.f;
    float outputGain_dB = 0.f;

    CompressionType compressorType;

    EnvelopeFollower envL; // TODO: make this so that it can be used for both left and right channels in one line, instead of having to declare two of them
    EnvelopeFollower envR;
};