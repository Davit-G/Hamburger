#pragma once

#include "../shared/chowdsp_utils/chowdsp_SIMDAudioBlock.h"
#include "HysteresisProcessing.h"
#include "DCBlocker.h"

/* Hysteresis Processor for tape. */
class TapeDistortionProcessor
{
public:
    TapeDistortionProcessor() = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock (juce::AudioBuffer<float>& buffer);
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
    void process (chowdsp::AudioBlock<T>& block);
    template <typename T>
    void processSmooth (chowdsp::AudioBlock<T>& block);

    std::vector<juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>> drive;
    std::vector<juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>> width;
    std::vector<juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>> sat;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative> makeup;

    std::unique_ptr<juce::dsp::Oversampling<double>> oversampling;
    std::vector<HysteresisProcessing> hProcs;
    std::vector<DCBlocker> dcBlocker;

    static constexpr double dcFreq = 35.0;
    float clipLevel = 20.0f;

    juce::AudioBuffer<double> doubleBuffer;

#if HYSTERESIS_USE_SIMD
    using Vec2 = xsimd::batch<double>;
    chowdsp::AudioBlock<Vec2> interleavedBlock;
    chowdsp::AudioBlock<double> zeroBlock;

    juce::HeapBlock<char> interleavedBlockData, zeroData;
    std::vector<const double*> channelPointers;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDistortionProcessor)
};
