#pragma once

// #include "../shared/chowdsp_utils/chowdsp_SIMDAudioBlock.h"
#include "HysteresisProcessing.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "../../SmoothParam.h"
#include "../../../utils/Params.h"

/* Hysteresis Processor for tape. */
class TapeDistortionProcessor
{
public:
    TapeDistortionProcessor(juce::AudioProcessorValueTreeState &treeState) : 
        driveParam(treeState, ParamIDs::tapeDrive),
        biasParam(treeState, ParamIDs::tapeBias),
        widthParam(treeState, ParamIDs::tapeWidth)
    {}

    void prepareToPlay (double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock (juce::dsp::AudioBlock<float>& buffer);
    float getLatencySamples() const noexcept;

    // all params are scaled from [0, 1]
    enum class Param
    {
        Drive,
        Saturation,
        Bias,
    };

    void setParameter (Param param, float newValue);

private:
    void setDrive (float newDrive);
    void setSaturation (float newSat);
    void setWidth (float newWidth);
    double calcMakeup();

    template <typename T>
    void process (juce::dsp::AudioBlock<T>& block);
    template <typename T>
    void processSmooth (juce::dsp::AudioBlock<T>& block);

    std::vector<juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>> drive;
    std::vector<juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>> width;
    std::vector<juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>> sat;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative> makeup;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<double>, juce::dsp::IIR::Coefficients<double>> iirFilter;

    std::unique_ptr<juce::dsp::Oversampling<double>> oversampling;
    std::vector<HysteresisProcessing> hProcs;

    float clipLevel = 20.0f;

    juce::AudioBuffer<double> doubleBuffer;

    SmoothParam driveParam;
    SmoothParam biasParam;
    SmoothParam widthParam;

#if HYSTERESIS_USE_SIMD
    using Vec2 = xsimd::batch<double>;
    chowdsp::AudioBlock<Vec2> interleavedBlock;
    chowdsp::AudioBlock<double> zeroBlock;

    juce::HeapBlock<char> interleavedBlockData, zeroData;
    std::vector<const double*> channelPointers;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDistortionProcessor)
};
