#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>

#include <juce_dsp/juce_dsp.h>

#include "../../utils/LevelMeter.h"
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

    AudioBufferQueue<SampleType> audioBufferQueueSpectrum;
    AudioBufferQueue<SampleType> audioBufferQueueInputSpectrum;

    // the lows and the highs metered apart as well, so a hi hat's spike shows even over a loud kick. only while the start
    // page's drive knob is on screen to show them
    void captureInput(const SampleType *dataL, size_t numSamples)
    {
        audioBufferQueueInputSpectrum.push(dataL, numSamples);

        if (! inputBandsWanted.load(std::memory_order_relaxed))
            return;

        const auto rate = getSampleRate();
        const auto lowCoefficient = (SampleType) (1.0 - std::exp(-juce::MathConstants<double>::twoPi * lowSplitHz / rate));
        const auto highCoefficient = (SampleType) (1.0 - std::exp(-juce::MathConstants<double>::twoPi * highSplitHz / rate));

        SampleType lowPeak {}, highPeak {};

        for (size_t i = 0; i < numSamples; ++i)
        {
            lowState += lowCoefficient * (dataL[i] - lowState);
            highState += highCoefficient * (dataL[i] - highState);

            lowPeak = std::max(lowPeak, std::abs(lowState));
            highPeak = std::max(highPeak, std::abs(dataL[i] - highState));
        }

        // once a block, rather than touching the meters every sample
        inputLows.accumulate((float) lowPeak);
        inputHighs.accumulate((float) highPeak);
    }

    // two clones cause the frame rate is not the same between them and this updates on gui only (idk whyyy)
    LevelMeter levelMeter {0.1f} ;  // used for actual visualisation on scope screen
    LevelMeter clipIndicator; // used for post clipping dot

    LevelMeter band1 {0.1f}; // could be low band, channel L, or mids channel
    LevelMeter band2 {0.1f}; // could be mid band, channel R, or sides channel
    LevelMeter band3 {0.1f}; // could be high band
    LevelMeter band4 {0.1f}; // could be extra high band for type A

    // the transient shaper's gain per band, furthest from nothing since the scope last took it
    std::array<std::atomic<float>, 3> transientGainDb {};

    // the loudest coming in, below and above the splits, read and cleared every frame by the start page's drive knob
    LevelMeter inputLows, inputHighs;
    std::atomic<bool> inputBandsWanted { false };

private:
    std::atomic<double> currentSampleRate { 44100.0 };

    static constexpr double lowSplitHz = 150.0, highSplitHz = 3000.0;
    SampleType lowState {}, highState {};

    // dataL here is already the oversampled signal, so bringing it back down to base rate
    // for the scope just means keeping every (2^oversamplingFactor)th sample - the main
    // oversampling chain has already anti-aliased it before we ever see it
    static void decimate(juce::AudioBuffer<SampleType>& scratch, const SampleType* dataL, size_t numSamples, int oversamplingFactor);

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
