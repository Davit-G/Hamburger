#pragma once

#include <JuceHeader.h>

#include "QuadratureOscillator.h"

class WeaverShifter
{
public:
    WeaverShifter()
    {

        preOscillator = std::make_unique<QuadratureOscillator>();
        postOscillator = std::make_unique<QuadratureOscillator>();
    };
    ~WeaverShifter(){

    };

    void setFrequencyShift(float newShift)
    {
        postOscillator->setFrequencyShift(newShift);
    };

    void prepareToPlay(float sampleRate, int newBlockSize)
    {
        preOscillator->prepareToPlay(sampleRate, newBlockSize);
        postOscillator->prepareToPlay(sampleRate, newBlockSize);

        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = newBlockSize;
        spec.numChannels = 2;

        upperBuffer.setSize(2, newBlockSize);
        upperBuffer.clear();
        lowerBuffer.setSize(2, newBlockSize);
        lowerBuffer.clear();

        upperBlock = dsp::AudioBlock<float>(upperBuffer);
        lowerBlock = dsp::AudioBlock<float>(lowerBuffer);

        for (int i = 0; i < 24; i++)
        {
            upperLP[i].prepare(spec);
            upperLP[i].reset();
            lowerLP[i].prepare(spec);
            lowerLP[i].reset();
        }

        for (const auto &filter : upperLP)
        {
            *filter.state = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, sampleRate * 0.25);
        }

        for (const auto &filter : lowerLP)
        {
            *filter.state = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, sampleRate * 0.25);
        }

        
    };

    void processBlock(dsp::AudioBlock<float> &block)
    {
        // // first quadrature oscillator, go
        upperBuffer.clear();
        lowerBuffer.clear();
        for (int i = 0; i < block.getNumSamples(); i++)
        {
            preOscillator->calculateNextSample();
            float sine = preOscillator->getSine();
            float cosine = preOscillator->getCosine();

            // upper buffer
            upperBuffer.setSample(0, i, block.getSample(0, i) * sine);
            upperBuffer.setSample(1, i, block.getSample(1, i) * sine);

            // lower buffer
            lowerBuffer.setSample(0, i, block.getSample(0, i) * cosine);
            lowerBuffer.setSample(1, i, block.getSample(1, i) * cosine);
        }

        // // filtering stage on both
        // for (int i = 0; i < 24; i++)
        // {
        //     upperLP[i].process(dsp::ProcessContextReplacing<float>(block));
        //     lowerLP[i].process(dsp::ProcessContextReplacing<float>(block));
        // }

        // // second quadrature oscillator, go
        for (int i = 0; i < block.getNumSamples(); i++)
        {
            postOscillator->calculateNextSample();
            float sine = postOscillator->getSine();
            float cosine = postOscillator->getCosine();

            // upper buffer
            upperBuffer.setSample(0, i, upperBuffer.getSample(0, i) * sine);
            upperBuffer.setSample(1, i, upperBuffer.getSample(1, i) * sine);

            // lower buffer
            lowerBuffer.setSample(0, i, lowerBuffer.getSample(0, i) * cosine);
            lowerBuffer.setSample(1, i, lowerBuffer.getSample(1, i) * cosine);


            
        }

        block.replaceWithSumOf(upperBlock, lowerBlock);
    };

private:
    AudioBuffer<float> upperBuffer;
    dsp::AudioBlock<float> upperBlock;

    AudioBuffer<float> lowerBuffer;
    dsp::AudioBlock<float> lowerBlock;

    std::unique_ptr<QuadratureOscillator> preOscillator = nullptr;
    std::unique_ptr<QuadratureOscillator> postOscillator = nullptr;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> upperLP[24];
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> lowerLP[24];

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeaverShifter);
};