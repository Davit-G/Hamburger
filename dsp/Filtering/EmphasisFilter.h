#pragma once

#include "juce_core/juce_core.h"
#include "ACoffs.h"

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

    void prepare(dsp::ProcessSpec &spec)
    {
        // Initialize the filter

        auto defaultCoeffs = ACoffs<double>::makeDualPeakFilter(spec.sampleRate, 500.0f, 0.5f, 1.0f);
        auto lowCoeffs = defaultCoeffs[0];
        auto highCoeffs = defaultCoeffs[1];

        for (int channel = 0; channel < 2; channel++)
        {
            *peakFilterBefore[0][channel].coefficients = lowCoeffs;
            *peakFilterBefore[1][channel].coefficients = highCoeffs;
            *peakFilterAfter[0][channel].coefficients = lowCoeffs;
            *peakFilterAfter[1][channel].coefficients = highCoeffs;
        }

        for (int channel = 0; channel < 2; channel++)
        {
            for (int i = 0; i < 2; i++)
            {
                peakFilterBefore[i][channel].prepare(spec);
                peakFilterAfter[i][channel].prepare(spec);
            }
        }

        emphasisLowSmooth.prepare(spec);
        emphasisHighSmooth.prepare(spec);
        emphasisLowFreqSmooth.prepare(spec);
        emphasisHighFreqSmooth.prepare(spec);

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

        sampleRate = spec.sampleRate;
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

        TRACE_EVENT("dsp", "emphasis EQ before");

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

            auto highCoeffs = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(sampleRate, nextEmphasisHighFreq, 0.5f, Decibels::decibelsToGain(-nextEmphasisHigh));
            auto lowCoeffs = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(sampleRate, nextEmphasisLowFreq, 0.5f, Decibels::decibelsToGain(-nextEmphasisLow));

            for (int channel = 0; channel < block.getNumChannels(); channel++)
            {
                *peakFilterBefore[1][channel].coefficients = highCoeffs;
                *peakFilterBefore[0][channel].coefficients = lowCoeffs;

                auto interm = peakFilterBefore[0][channel].processSample(block.getSample(channel, sample));
                block.setSample(channel, sample, peakFilterBefore[1][channel].processSample(interm));
            }
        }

        peakFilterBefore[0][0].snapToZero();
        peakFilterBefore[1][0].snapToZero();
        peakFilterBefore[0][1].snapToZero();
        peakFilterBefore[1][1].snapToZero();
        peakFilterAfter[0][0].snapToZero();
        peakFilterAfter[1][0].snapToZero();
        peakFilterAfter[0][1].snapToZero();
        peakFilterAfter[1][1].snapToZero();
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

            auto highCoeffs = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(sampleRate, nextEmphasisHighFreq, 0.5f, Decibels::decibelsToGain(nextEmphasisHigh));
            auto lowCoeffs = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(sampleRate, nextEmphasisLowFreq, 0.5f, Decibels::decibelsToGain(nextEmphasisLow));
            
            for (int channel = 0; channel < block.getNumChannels(); channel++)
            {

                *peakFilterAfter[1][channel].coefficients = highCoeffs;
                *peakFilterAfter[0][channel].coefficients = lowCoeffs;

                float eqCompensation = (emphasisLowSmooth.getRaw() + emphasisHighSmooth.getRaw()) * 0.133f;

                auto interm = peakFilterAfter[0][channel].processSample(block.getSample(channel, sample));
                block.setSample(channel, sample, peakFilterAfter[1][channel].processSample(interm) * juce::Decibels::decibelsToGain(-eqCompensation));
            }
        }
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

    dsp::IIR::Filter<double> peakFilterBefore[2][2];
    dsp::IIR::Filter<double> peakFilterAfter[2][2];
};