#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"

class DCBlockingHighPass {
public:
    DCBlockingHighPass() {}
    ~DCBlockingHighPass() {}

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
    }
    
    void processBlock(juce::dsp::AudioBlock<float> &block)
    {
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
    std::unique_ptr<juce::AudioBuffer<double>> bufferDouble;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<double>, juce::dsp::IIR::Coefficients<double>> iirFilter;
};