#pragma once

#include <JuceHeader.h>


//==============================================================================
/*
 */
class AllPassChain
{
public:
    AllPassChain(juce::AudioParameterFloat *param, juce::AudioParameterFloat *param2, juce::AudioParameterFloat *param3)
    {
        allPassFrequency = param;
        jassert(allPassFrequency);

        allPassQ = param2;
        jassert(allPassQ);

        allPassAmount = param3;
        jassert(allPassAmount);
    }
    ~AllPassChain(){

    };

    void processBlock(dsp::AudioBlock<float> &block)
    {
        if ((oldAllPassFreq != allPassFrequency->get()) || (oldAllPassQ != allPassQ->get()) || (oldAllPassAmount != allPassAmount->get()))
        {
            for (const auto &allPassFilter : allPassFilters)
            {
                *allPassFilter.state = *dsp::IIR::Coefficients<float>::makeAllPass(oldSampleRate, allPassFrequency->get(), allPassQ->get());
            }
        }

        for (int i = 0; i < allPassAmount->get(); i++)
        {
            allPassFilters[i].process(dsp::ProcessContextReplacing<float>(block));
        }
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {

        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;

        for (const auto &allPassFilter : allPassFilters)
        {
            *allPassFilter.state = *dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, allPassFrequency->get(), allPassQ->get());
        }

        for (int i = 0; i < 50; i++) {
            allPassFilters[i].prepare(spec);
            allPassFilters[i].reset();
        }

        oldSampleRate = sampleRate;
    }

private:
    juce::AudioParameterFloat *allPassFrequency = nullptr;
    juce::AudioParameterFloat *allPassQ = nullptr;
    juce::AudioParameterFloat *allPassAmount = nullptr;

    float oldAllPassFreq = 0.0;
    float oldAllPassQ = 0.0;
    float oldAllPassAmount = 0.0;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> allPassFilters[50];

    double oldSampleRate;
    double oldSamplesPerBlock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllPassChain)
};