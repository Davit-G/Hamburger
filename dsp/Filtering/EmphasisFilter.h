#pragma once

#include "juce_core/juce_core.h"
#include "ACoffs.h"
#include "chowdsp_filters/chowdsp_filters.h"

class EmphasisFilter
{
public:
    EmphasisFilter(juce::AudioProcessorValueTreeState &treeState) : emphasisHighFreqSmooth(treeState, ParamIDs::emphasisHighFreq),
                                                                    emphasisLowFreqSmooth(treeState, ParamIDs::emphasisLowFreq),
                                                                    emphasisHighSmooth(treeState, ParamIDs::emphasisHighGain),
                                                                    emphasisLowSmooth(treeState, ParamIDs::emphasisLowGain)
    {
        enableEmphasis = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::emphasisOn.getParamID()));
        jassert(enableEmphasis);
    };

    void prepare(juce::dsp::ProcessSpec &spec)
    {
        for (int channel = 0; channel < 2; channel++)
        {
            for (int i = 0; i < 2; i++)
            {
                peakFilterBeforeSVF[i].prepare(spec);
                peakFilterAfterSVF[i].prepare(spec);
            }
        }

        emphasisLowSmooth.prepare(spec);
        emphasisHighSmooth.prepare(spec);
        emphasisLowFreqSmooth.prepare(spec);
        emphasisHighFreqSmooth.prepare(spec);

        sampleRate = spec.sampleRate;

        emphasisLowBuffer.reserve(spec.maximumBlockSize);
        emphasisHighBuffer.reserve(spec.maximumBlockSize);
        emphasisLowFreqBuffer.reserve(spec.maximumBlockSize);
        emphasisHighFreqBuffer.reserve(spec.maximumBlockSize);

        for (int i = 0; i < spec.maximumBlockSize; i++)
        {
            emphasisLowBuffer.push_back(0.0f);
            emphasisHighBuffer.push_back(0.0f);
            emphasisLowFreqBuffer.push_back(0.0f);
            emphasisHighFreqBuffer.push_back(0.0f);
        }
    }

    void processBefore(juce::dsp::AudioBlock<float> &block)
    {
        emphasisOn = enableEmphasis->get();

        emphasisLowSmooth.update();
        emphasisHighSmooth.update();
        emphasisLowFreqSmooth.update();
        emphasisHighFreqSmooth.update();

        if (!emphasisOn)
            return;

        // TRACE_EVENT("dsp", "emphasis EQ before");

        for (int i = 0; i < block.getNumSamples(); i++)
        {
            emphasisLowBuffer[i] = emphasisLowSmooth.get();
            emphasisHighBuffer[i] = emphasisHighSmooth.get();
            emphasisLowFreqBuffer[i] = emphasisLowFreqSmooth.get();
            emphasisHighFreqBuffer[i] = emphasisHighFreqSmooth.get();
        }

        // figure out if coefficients need updating

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float nextEmphasisLow = emphasisLowBuffer[sample];
            float nextEmphasisHigh = emphasisHighBuffer[sample];
            float nextEmphasisLowFreq = emphasisLowFreqBuffer[sample];
            float nextEmphasisHighFreq = emphasisHighFreqBuffer[sample];

            for (int channel = 0; channel < block.getNumChannels(); channel++)
            {
                peakFilterBeforeSVF[0].setCutoffFrequency(nextEmphasisLowFreq);
                peakFilterBeforeSVF[0].setQValue(0.5f);
                peakFilterBeforeSVF[0].setGainDecibels(-nextEmphasisLow);
                peakFilterBeforeSVF[1].setCutoffFrequency(nextEmphasisHighFreq);
                peakFilterBeforeSVF[1].setQValue(0.5f);
                peakFilterBeforeSVF[1].setGainDecibels(-nextEmphasisHigh);

                auto interm = peakFilterBeforeSVF[0].processSample(channel, block.getSample(channel, sample));
                block.setSample(channel, sample, peakFilterBeforeSVF[1].processSample(channel, interm));
            }
        }

        peakFilterBeforeSVF[0].snapToZero();
        peakFilterBeforeSVF[1].snapToZero();
    }

    void processAfter(juce::dsp::AudioBlock<float> &block)
    {
        if (!emphasisOn)
            return;

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float nextEmphasisLow = emphasisLowBuffer[sample];
            float nextEmphasisHigh = emphasisHighBuffer[sample];
            float nextEmphasisLowFreq = emphasisLowFreqBuffer[sample];
            float nextEmphasisHighFreq = emphasisHighFreqBuffer[sample];

            peakFilterAfterSVF[0].setCutoffFrequency(nextEmphasisLowFreq);
            peakFilterAfterSVF[0].setQValue(0.5f);
            peakFilterAfterSVF[0].setGainDecibels(nextEmphasisLow);
            peakFilterAfterSVF[1].setCutoffFrequency(nextEmphasisHighFreq);
            peakFilterAfterSVF[1].setQValue(0.5f);
            peakFilterAfterSVF[1].setGainDecibels(nextEmphasisHigh);
            
            for (int channel = 0; channel < block.getNumChannels(); channel++)
            {

                float eqCompensation = (emphasisLowSmooth.getRaw() + emphasisHighSmooth.getRaw()) * 0.133f;

                auto interm = peakFilterAfterSVF[0].processSample(channel, block.getSample(channel, sample));
                block.setSample(channel, sample, peakFilterAfterSVF[1].processSample(channel, interm) * juce::Decibels::decibelsToGain(-eqCompensation));
            }
        }

        peakFilterAfterSVF[0].snapToZero();
        peakFilterAfterSVF[1].snapToZero();
    }

private:
    std::array<float, 6>
        beforeCoeffs;
    std::array<float, 6> afterCoeffs;

    double sampleRate = 44100.0;
    bool emphasisOn = true;

    SmoothParam emphasisLowSmooth;
    SmoothParam emphasisHighSmooth;
    SmoothParam emphasisLowFreqSmooth;
    SmoothParam emphasisHighFreqSmooth;

    std::vector<float> emphasisLowBuffer;
    std::vector<float> emphasisHighBuffer;
    std::vector<float> emphasisLowFreqBuffer;
    std::vector<float> emphasisHighFreqBuffer;

    juce::AudioParameterBool *enableEmphasis = nullptr;

    chowdsp::SVFBell<double> peakFilterBeforeSVF[2];
    chowdsp::SVFBell<double> peakFilterAfterSVF[2];
};