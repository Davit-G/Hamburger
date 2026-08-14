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

    void accumulateClipping(float sampleL, float sampleR) {
        float sample = fabs(fmax(sampleL, sampleR));

        constexpr float coeff = 0.0001f; // clip level decay
        float current = clipLevel.load(std::memory_order_relaxed);

        if (sample > current) {
            // jump
            clipLevel.store(sample, std::memory_order_relaxed);
        } else {
            // decay
            clipLevel.store(current * (1.0f - coeff), std::memory_order_relaxed);
        }
    }

    std::atomic<float> clipLevel;

private:
    std::atomic<double> currentSampleRate { 44100.0 };

    // dataL here is already the oversampled signal, so bringing it back down to base rate
    // for the scope just means keeping every (2^oversamplingFactor)th sample - the main
    // oversampling chain has already anti-aliased it before we ever see it
    static void decimate(juce::AudioBuffer<SampleType>& scratch, const SampleType* dataL, size_t numSamples, int oversamplingFactor);

    // if either queue had to silently overflow-reset, the two are no longer reading the
    // same point in time relative to each other - force both back in lockstep rather than
    // let them drift apart permanently
    void resyncPrePostQueues();

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
