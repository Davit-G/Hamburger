#pragma once

#include "../EnvelopeFollower.h"

#include <JuceHeader.h>
using namespace juce;

enum CompressionType {
    EXPANDER,
    COMPRESSOR,
    UPWARDS_DOWNWARDS
};

class Compressor
{
public:
    Compressor(CompressionType ctype) : type(ctype), envelope(true) {}
    
    ~Compressor(){}

    void updateUpDown(float attack, float release, float makeup, float ratioLower, float ratioUpper, float thresholdLower, float thresholdUpper, float kneeWidth, float makeup_dB);
    void updateParameters(float attack, float release, float makeup, float ratio, float threshold, float kneeWidth, float makeup_dB);

    void processBlock(dsp::AudioBlock<float>& dryBuffer);
    float processOneSampleGainStereo(float sampleL, float sampleR);
    float processOneSampleGainMono(float sample);

    void prepare(dsp::ProcessSpec& spec);

    float attack = 0.0f;
    float release = 0.0f;
    float makeup = 0.0f;
    float ratioLower = 1.f;
    float ratioUpper = 1.f;
    float thresholdLower = 0.f;
    float thresholdUpper = 0.f;
    float kneeWidth = 0.f;
    float makeup_dB = 0.f;

private:
    CompressionType type;

    // juce::AudioProcessorValueTreeState &treeState;
    EnvelopeFollower envelope;
};