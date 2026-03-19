#pragma once

#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "../SmoothParam.h"
#include "../../../utils/Params.h"

/* 
 * Dolby A-type noise reduction processor.
 * 
 * Frequency bands:
 * - Low: below 80 Hz
 * - Mid: 80 Hz to 3 kHz (derived from signal - low - high)
 * - High: above 3 kHz
 * - Extra High: above 9 kHz (stacked with high band for extra reduction)
 * 
 * Each band has a compressor with threshold -40 dB and ratio affected by brightness.
 */
class TypeAProcessor
{
public:
    TypeAProcessor(juce::AudioProcessorValueTreeState &treeState) : 
        brightnessParam(treeState, ParamIDs::TypeARatio),
        thresholdParam(treeState, ParamIDs::TypeAThreshold),
        outParam(treeState, ParamIDs::TypeAOut),
        tiltParam(treeState, ParamIDs::TypeATilt)
    {}

    void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock(juce::dsp::AudioBlock<float>& buffer);

private:
    void updateCompressorParameters();
    float calculateMakeupGain(float ratio, float threshold) const;

    SmoothParam brightnessParam; // ratio
    SmoothParam thresholdParam;
    SmoothParam outParam;
    SmoothParam tiltParam;

    double currentSampleRate = 44100.0;
    int numChannels = 2;

    // Crossover filters (12 dB/octave = 2nd order)
    // Low-pass at 80 Hz
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;
    // High-pass at 3 kHz
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highPassFilter;
    // High-pass at 9 kHz
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> extraHighPassFilter;

    // Compressors for each band
    juce::dsp::Compressor<float> lowCompressor;
    juce::dsp::Compressor<float> midCompressor;
    juce::dsp::Compressor<float> highCompressor;
    juce::dsp::Compressor<float> extraHighCompressor;

    // Makeup gain for each band
    juce::dsp::Gain<float> lowMakeupGain;
    juce::dsp::Gain<float> midMakeupGain;
    juce::dsp::Gain<float> highMakeupGain;
    juce::dsp::Gain<float> extraHighMakeupGain;

    // Temp buffers for band separation
    juce::AudioBuffer<float> lowBuffer;
    juce::AudioBuffer<float> midBuffer;
    juce::AudioBuffer<float> highBuffer;
    juce::AudioBuffer<float> extraHighBuffer;

    // Constants
    static constexpr float lowCrossover = 80.0f;
    static constexpr float midHighCrossover = 3000.0f;
    static constexpr float extraHighCrossover = 9000.0f;
    // static constexpr float threshold = -40.0f;
    static constexpr float baseRatio = 2.0f;
    static constexpr float attack = 20.0f;   // ms
    static constexpr float release = 100.0f; // ms

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TypeAProcessor)
};
