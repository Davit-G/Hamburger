#pragma once

#include "juce_dsp/juce_dsp.h"
#include "juce_events/juce_events.h"

/*  Linear phase band splits. Every band but the top is the difference between the low passes either side of it, all taken
    from the same input, and the top is the input delayed by as much as the low passes delay it, less the highest one. So the
    bands always sum back to the input, only delayed, and whatever's mixed back with them only needs the same delay.

    The low passes are windowed sincs, designed on the message thread whenever a cutoff moves and handed over to the audio
    thread, where the convolutions crossfade to them. */
class LinearPhaseSplit : private juce::Timer
{
public:
    static constexpr int maxSplits = 3;
    static constexpr double latencySeconds = 0.02;

    LinearPhaseSplit() { startTimerHz (30); }
    ~LinearPhaseSplit() override { stopTimer(); }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        latency = juce::roundToInt (spec.sampleRate * latencySeconds);

        delay.setMaximumDelayInSamples (latency);
        delay.prepare (spec);
        delay.setDelay ((float) latency);

        for (auto& lowpass : lowpasses)
            lowpass.prepare (spec);

        for (auto& low : lows)
            low.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize, false, false, true);

        sampleRate = spec.sampleRate;
        redesign = true;
    }

    int getLatency() const { return latency; }

    // lowest first
    void setCutoffs (const float* hz, int count)
    {
        for (int i = 0; i < count; ++i)
            wantedHz[(size_t) i] = hz[i];
    }

    // numSplits + 1 bands into the buffers, lowest first
    void split (const juce::dsp::AudioBlock<float>& input, juce::AudioBuffer<float>* bands, int numSplits)
    {
        takeKernels();

        const auto numSamples = input.getNumSamples();
        const auto numChannels = input.getNumChannels();
        auto blockOf = [&] (juce::AudioBuffer<float>& buffer)
        {
            return juce::dsp::AudioBlock<float> (buffer).getSubBlock (0, numSamples).getSubsetChannelBlock (0, numChannels);
        };

        for (int i = 0; i < numSplits; ++i)
        {
            auto low = blockOf (lows[(size_t) i]);
            low.copyFrom (input);
            lowpasses[(size_t) i].process (juce::dsp::ProcessContextReplacing<float> (low));
        }

        auto top = blockOf (bands[numSplits]);
        top.copyFrom (input);
        delayBy (top);

        for (int i = 0; i < numSplits; ++i)
        {
            auto band = blockOf (bands[i]);
            band.copyFrom (blockOf (lows[(size_t) i]));

            if (i > 0)
                band.subtract (blockOf (lows[(size_t) i - 1]));
        }

        if (numSplits > 0)
            top.subtract (blockOf (lows[(size_t) numSplits - 1]));
    }

    // for whatever isn't split, to keep the same latency
    void delayBy (juce::dsp::AudioBlock<float>& block)
    {
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* samples = block.getChannelPointer (ch);

            for (size_t n = 0; n < block.getNumSamples(); ++n)
            {
                delay.pushSample ((int) ch, samples[n]);
                samples[n] = delay.popSample ((int) ch);
            }
        }
    }

private:
    // a convolution can only be handed a new kernel on the audio thread, so a new one waits here until it can be
    struct Pending
    {
        juce::SpinLock lock;
        juce::AudioBuffer<float> kernel;
        double rate = 44100.0;
        bool waiting = false;
    };

    void takeKernels()
    {
        for (size_t i = 0; i < pending.size(); ++i)
        {
            const juce::SpinLock::ScopedTryLockType tryLock (pending[i].lock);

            if (tryLock.isLocked() && pending[i].waiting)
            {
                lowpasses[i].loadImpulseResponse (std::move (pending[i].kernel), pending[i].rate,
                                                  juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::no,
                                                  juce::dsp::Convolution::Normalise::no);
                pending[i].waiting = false;
            }
        }
    }

    void timerCallback() override
    {
        const auto currentRate = sampleRate.load();
        const auto all = redesign.exchange (false);

        if (currentRate <= 0.0)
            return;

        for (size_t i = 0; i < pending.size(); ++i)
        {
            const auto hz = wantedHz[i].load();

            if (! all && std::abs (hz - designedHz[i]) <= designedHz[i] * 0.002f)
                continue;

            designedHz[i] = hz;
            auto kernel = design (hz, currentRate);

            const juce::SpinLock::ScopedLockType lock (pending[i].lock);
            pending[i].rate = currentRate;
            pending[i].kernel = std::move (kernel);
            pending[i].waiting = true;
        }
    }

    // a blackman windowed sinc, scaled to exactly unity at DC
    static juce::AudioBuffer<float> design (float hz, double rate)
    {
        const auto half = juce::roundToInt (rate * latencySeconds);
        const auto cycles = juce::jlimit (0.0, 0.5, (double) hz / rate);

        juce::AudioBuffer<float> kernel (1, half * 2 + 1);
        auto* taps = kernel.getWritePointer (0);
        auto sum = 0.0;

        for (int n = -half; n <= half; ++n)
        {
            const auto sinc = n == 0 ? 2.0 * cycles : std::sin (juce::MathConstants<double>::twoPi * cycles * n) / (juce::MathConstants<double>::pi * n);
            const auto turn = juce::MathConstants<double>::pi * n / (half + 1);
            const auto window = 0.42 + 0.5 * std::cos (turn) + 0.08 * std::cos (2.0 * turn);

            taps[n + half] = (float) (sinc * window);
            sum += sinc * window;
        }

        if (sum > 0.0)
            kernel.applyGain ((float) (1.0 / sum));

        return kernel;
    }

    std::array<juce::dsp::Convolution, maxSplits> lowpasses;
    std::array<juce::AudioBuffer<float>, maxSplits> lows;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> delay;
    int latency = 0;

    std::array<Pending, maxSplits> pending;
    std::array<std::atomic<float>, maxSplits> wantedHz { 200.0f, 1000.0f, 5000.0f };
    std::array<float, maxSplits> designedHz {};
    std::atomic<double> sampleRate { 0.0 };
    std::atomic<bool> redesign { false };
};
