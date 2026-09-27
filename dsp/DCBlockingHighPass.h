#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"

/*  Linear, it takes away a running average of the signal, averaged three times over. Past 20Hz that average is next to
    nothing, so the signal comes through with its phase all but untouched and no latency, where the iir turns the lows. */
class DCBlockingHighPass {
public:
    DCBlockingHighPass() {}
    ~DCBlockingHighPass() {}

    static constexpr double averagedHz = 20.0;
    static constexpr int averages = 3;

    void setLinear (bool shouldBeLinear) { linear = shouldBeLinear; }

    void prepare(juce::dsp::ProcessSpec &spec)
    {
        // init iir filter
        iirFilter.reset();
        *iirFilter.state = juce::dsp::IIR::ArrayCoefficients<double>::makeHighPass(spec.sampleRate, 8.0f);
        iirFilter.prepare(spec);

        const int safeMaxBlockSize = juce::jmax(static_cast<int>(spec.maximumBlockSize), 8192);

        bufferDouble = std::make_unique<juce::AudioBuffer<double>>(spec.numChannels, spec.maximumBlockSize);

        bufferDouble->setSize(spec.numChannels, safeMaxBlockSize);
        bufferDouble->clear();

        averageLength = juce::jmax (1, juce::roundToInt (spec.sampleRate / averagedHz));
        history.assign ((size_t) (averages * (int) spec.numChannels * averageLength), 0.0f);
        sums.assign ((size_t) (averages * (int) spec.numChannels), 0.0);
        position = 0;
    }
    
    void processBlock(juce::dsp::AudioBlock<float> &block)
    {
        if (linear)
            return subtractAverage (block);

        const int numChannels = static_cast<int>(block.getNumChannels());
        const int numSamples  = static_cast<int>(block.getNumSamples());

        // resize double buffer (yes i know it could allocate, but this fixes audio issues)
        bufferDouble->setSize(numChannels, numSamples, false, false, true);
        auto blockDouble = juce::dsp::AudioBlock<double>(*bufferDouble);
        auto activeBlockDouble = blockDouble.getSubBlock(0, (size_t)numSamples);
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const float* src = block.getChannelPointer(channel);
            double* dest     = activeBlockDouble.getChannelPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                dest[sample] = static_cast<double>(src[sample]);
            }
        }
        
        auto doubleContext = juce::dsp::ProcessContextReplacing<double>(activeBlockDouble);
        iirFilter.process(doubleContext);
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const double* src = activeBlockDouble.getChannelPointer(channel);
            float* dest       = block.getChannelPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                dest[sample] = static_cast<float>(src[sample]);
            }
        }
    }

private:
    // each average adds what it stores and takes away what it drops, so the sums never drift from what's held
    void subtractAverage (juce::dsp::AudioBlock<float>& block)
    {
        const auto prepared = sums.size() / averages;
        const auto numChannels = juce::jmin (block.getNumChannels(), prepared);

        for (size_t n = 0; n < block.getNumSamples(); ++n)
        {
            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto& sample = block.getChannelPointer (ch)[n];
                auto averaged = (double) sample;

                for (size_t stage = 0; stage < averages; ++stage)
                {
                    const auto row = stage * prepared + ch;
                    auto& held = history[row * (size_t) averageLength + (size_t) position];
                    const auto stored = (float) averaged;

                    sums[row] += (double) stored - (double) held;
                    held = stored;
                    averaged = sums[row] / averageLength;
                }

                sample -= (float) averaged;
            }

            position = (position + 1) % averageLength;
        }
    }

    bool linear = false;
    int averageLength = 1, position = 0;
    std::vector<float> history;
    std::vector<double> sums;

    std::unique_ptr<juce::AudioBuffer<double>> bufferDouble;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<double>, juce::dsp::IIR::Coefficients<double>> iirFilter;
};