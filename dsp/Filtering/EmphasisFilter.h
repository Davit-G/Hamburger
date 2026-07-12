#pragma once

#include "juce_core/juce_core.h"
#include "ACoffs.h"
#include "chowdsp_filters/chowdsp_filters.h"

class EmphasisFilter
{
public:
    explicit EmphasisFilter(juce::AudioProcessorValueTreeState& treeState)
        : emphasisHighFreqSmooth(treeState, ParamIDs::emphasisHighFreq),
          emphasisLowFreqSmooth(treeState, ParamIDs::emphasisLowFreq),
          emphasisHighSmooth(treeState, ParamIDs::emphasisHighGain),
          emphasisLowSmooth(treeState, ParamIDs::emphasisLowGain)
    {
        enableEmphasis = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::emphasisOn.getParamID()));
        jassert(enableEmphasis);
    }

    void prepare(juce::dsp::ProcessSpec& spec)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            for (int i = 0; i < 2; ++i)
            {
                peakFilterBeforeSVF[i].prepare(spec);
                peakFilterBeforeSVF[1].setQValue(0.5f);
                peakFilterAfterSVF[i].prepare(spec);
                peakFilterAfterSVF[1].setQValue(0.5f);
            }
        }

        emphasisLowSmooth.prepare(spec);
        emphasisHighSmooth.prepare(spec);
        emphasisLowFreqSmooth.prepare(spec);
        emphasisHighFreqSmooth.prepare(spec);

        sampleRate = spec.sampleRate;

        const auto blockSize = static_cast<size_t>(spec.maximumBlockSize);
        emphasisLowBuffer.resize(blockSize, 0.0f);
        emphasisHighBuffer.resize(blockSize, 0.0f);
        emphasisLowFreqBuffer.resize(blockSize, 0.0f);
        emphasisHighFreqBuffer.resize(blockSize, 0.0f);
    }

    void processBefore(juce::dsp::AudioBlock<float>& block)
    {
        if (enableEmphasis != nullptr)
            emphasisOn = enableEmphasis->get();

        emphasisLowSmooth.update();
        emphasisHighSmooth.update();
        emphasisLowFreqSmooth.update();
        emphasisHighFreqSmooth.update();

        parametersNeedUpdates = emphasisLowSmooth.isSmoothing(0) || emphasisHighSmooth.isSmoothing(0) || emphasisLowFreqSmooth.isSmoothing(0) || emphasisHighFreqSmooth.isSmoothing(0);

        if (!emphasisOn)
            return;

        const auto numChannels = block.getNumChannels();
        const auto numSamples = block.getNumSamples();

        if (parametersNeedUpdates) {
            for (size_t sample = 0; sample < numSamples; ++sample)
            {
                emphasisLowBuffer[sample] = emphasisLowSmooth.getNextValue(0);
                emphasisHighBuffer[sample] = emphasisHighSmooth.getNextValue(0);
                emphasisLowFreqBuffer[sample] = emphasisLowFreqSmooth.getNextValue(0);
                emphasisHighFreqBuffer[sample] = emphasisHighFreqSmooth.getNextValue(0);
            }
        }

        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            if (parametersNeedUpdates && (sample % samplesToSkip == 0)) {
                const auto nextEmphasisLow = -emphasisLowBuffer[sample];
                const auto nextEmphasisHigh = -emphasisHighBuffer[sample];
                const auto nextEmphasisLowFreq = emphasisLowFreqBuffer[sample];
                const auto nextEmphasisHighFreq = emphasisHighFreqBuffer[sample];

                peakFilterBeforeSVF[0].setCutoffFrequency(nextEmphasisLowFreq);
                peakFilterBeforeSVF[0].setGainDecibels(nextEmphasisLow);
                peakFilterBeforeSVF[1].setCutoffFrequency(nextEmphasisHighFreq);
                peakFilterBeforeSVF[1].setGainDecibels(nextEmphasisHigh);
            }

            for (size_t channel = 0; channel < numChannels; ++channel)
            {
                const auto input = block.getSample(channel, sample);
                const auto interm = peakFilterBeforeSVF[0].processSample(channel, input);
                block.setSample(channel, sample, peakFilterBeforeSVF[1].processSample(channel, interm));
            }
        }

        peakFilterBeforeSVF[0].snapToZero();
        peakFilterBeforeSVF[1].snapToZero();
    }

    void processAfter(juce::dsp::AudioBlock<float>& block)
    {
        if (!emphasisOn)
            return;

        const auto numChannels = block.getNumChannels();
        const auto numSamples = block.getNumSamples();
        const auto eqCompensation = juce::Decibels::decibelsToGain(-(emphasisLowSmooth.getRaw(0) + emphasisHighSmooth.getRaw(0)) * 0.133f);

        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            if (parametersNeedUpdates && (sample % samplesToSkip == 0)) {
                const auto nextEmphasisLow = emphasisLowBuffer[sample];
                const auto nextEmphasisHigh = emphasisHighBuffer[sample];
                const auto nextEmphasisLowFreq = emphasisLowFreqBuffer[sample];
                const auto nextEmphasisHighFreq = emphasisHighFreqBuffer[sample];

                peakFilterAfterSVF[0].setCutoffFrequency(nextEmphasisLowFreq);
                peakFilterAfterSVF[0].setGainDecibels(nextEmphasisLow);
                peakFilterAfterSVF[1].setCutoffFrequency(nextEmphasisHighFreq);
                peakFilterAfterSVF[1].setGainDecibels(nextEmphasisHigh);
            }

            for (size_t channel = 0; channel < numChannels; ++channel)
            {
                const auto input = block.getSample(channel, sample);
                const auto interm = peakFilterAfterSVF[0].processSample(channel, input);
                block.setSample(channel, sample, peakFilterAfterSVF[1].processSample(channel, interm) * eqCompensation);
            }
        }

        peakFilterAfterSVF[0].snapToZero();
        peakFilterAfterSVF[1].snapToZero();
    }

private:
    double sampleRate = 44100.0;
    bool emphasisOn = true;

    bool parametersNeedUpdates = true;

    SmoothParam emphasisLowSmooth;
    SmoothParam emphasisHighSmooth;
    SmoothParam emphasisLowFreqSmooth;
    SmoothParam emphasisHighFreqSmooth;

    std::vector<float> emphasisLowBuffer;
    std::vector<float> emphasisHighBuffer;
    std::vector<float> emphasisLowFreqBuffer;
    std::vector<float> emphasisHighFreqBuffer;

    juce::AudioParameterBool* enableEmphasis = nullptr;

    int samplesToSkip = 8;

    chowdsp::SVFBell<float> peakFilterBeforeSVF[2];
    chowdsp::SVFBell<float> peakFilterAfterSVF[2];
};