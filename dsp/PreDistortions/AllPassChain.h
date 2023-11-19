#pragma once

#include <JuceHeader.h>


//==============================================================================
/*
 */
class AllPassChain
{
public:
    AllPassChain(juce::AudioProcessorValueTreeState& treeState) : 
        allPassFrequency(treeState, "allPassFreq"), 
        allPassQ(treeState, "allPassQ"), 
        allPassAmount(treeState, "allPassAmount") {}
    ~AllPassChain(){

    }

    void processBlock(dsp::AudioBlock<float> &block)
    {
        TRACE_EVENT("dsp", "AllPassChain::processBlock");
        allPassAmount.update();
        allPassFrequency.update();
        allPassQ.update();

        float amount = allPassAmount.getRaw();
        float freq = allPassFrequency.getRaw();
        float q = allPassQ.getRaw();

        if ((oldAllPassFreq != freq) || (oldAllPassQ != q) || (oldAllPassAmount != amount))
        {
            for (const auto &allPassFilter : allPassFilters)
            {
                *allPassFilter.state = *dsp::IIR::Coefficients<float>::makeAllPass(oldSampleRate, freq, q);
            }
        }

        for (int i = 0; i < amount; i++)
        {
            allPassFilters[i].process(dsp::ProcessContextReplacing<float>(block));
        }
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        allPassFrequency.prepareToPlay(sampleRate, samplesPerBlock);
        allPassQ.prepareToPlay(sampleRate, samplesPerBlock);
        allPassAmount.prepareToPlay(sampleRate, samplesPerBlock);

        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;

        for (const auto &allPassFilter : allPassFilters)
        {
            *allPassFilter.state = *dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, allPassFrequency.getRaw(), allPassQ.getRaw());
        }

        for (int i = 0; i < 50; i++) {
            allPassFilters[i].prepare(spec);
            allPassFilters[i].reset();
        }

        oldSampleRate = sampleRate;
    }

private:
    SmoothParam allPassFrequency;
    SmoothParam allPassQ;
    SmoothParam allPassAmount;

    float oldAllPassFreq = 0.0;
    float oldAllPassQ = 0.0;
    float oldAllPassAmount = 0.0;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> allPassFilters[50];

    double oldSampleRate;
    double oldSamplesPerBlock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllPassChain)
};