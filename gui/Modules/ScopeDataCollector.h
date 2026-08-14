#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>

#include <juce_dsp/juce_dsp.h>

#include "AudioBufferQueue.h"

//==============================================================================
template <typename SampleType>
class ScopeDataCollector
{
public:
    //==============================================================================
    ScopeDataCollector() {}

    void prepare(juce::dsp::ProcessSpec& spec);

    //==============================================================================
    void process(const SampleType *dataL, const SampleType *dataR, size_t numSamples);

    void capturePreDistortion(const SampleType *dataL, size_t numSamples, int oversamplingFactor);

    void capturePostDistortion(const SampleType *dataL, size_t numSamples, int oversamplingFactor);

    // the ui thread needs this to turn fft bin indices back into frequencies
    double getSampleRate() const noexcept { return currentSampleRate.load(); }

    //==============================================================================
    AudioBufferQueue<SampleType> audioBufferQueueL;
    AudioBufferQueue<SampleType> audioBufferQueueR;

    AudioBufferQueue<SampleType> audioBufferQueuePreDistortion;
    AudioBufferQueue<SampleType> audioBufferQueuePostDistortion;
private:
    std::atomic<double> currentSampleRate { 44100.0 };

    void prepareOversampler(std::unique_ptr<juce::dsp::Oversampling<SampleType>>& oversampler,
                            int oversamplingFactor,
                            size_t numSamples);

    std::unique_ptr<juce::dsp::Oversampling<SampleType>> preDistOversampling;
    std::unique_ptr<juce::dsp::Oversampling<SampleType>> postDistOversampling;
    juce::AudioBuffer<SampleType> preDistScratchBuffer;
    juce::AudioBuffer<SampleType> postDistScratchBuffer;

    float samplesReadPre, samplesReadPost;

    // size_t numCollected;
    // SampleType prevSample = SampleType(100);

    // static constexpr auto triggerLevel = SampleType(0.001);

    // enum class State
    // {
    //     waitingForTrigger,
    //     collecting
    // } state{State::waitingForTrigger};
};
