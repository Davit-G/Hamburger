#pragma once

#include "../EnvelopeFollower.h"

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

// #include <melatonin_perfetto/melatonin_perfetto.h>

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

    void processBlock(juce::dsp::AudioBlock<float>& dryBuffer);
    float processOneSampleGainStereo(float sampleL, float sampleR);
    float processOneSampleGainMono(float sample);

    void prepare(juce::dsp::ProcessSpec& spec);

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