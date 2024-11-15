#pragma once
 
#include "../SmoothParam.h"
#include <cmath>

// broken code do not touch

using Float = dsp::SIMDRegister<float>;

// given up on this

class SIMDIIR
{
public:
    SIMDIIR() {};

    void calcCoefficients(float sampleRate, float frequency, float Q)
    {
        auto n = 1 / tanf(MathConstants<float>::pi * frequency / static_cast<float>(sampleRate));
        auto nSquared = n * n;
        auto invQ = 1.0f / Q;
        auto c1 = 1.0f / (1.0f + invQ * n + nSquared);
        auto b0 = c1 * (1.0f - n * invQ + nSquared);
        auto b1 = c1 * 2.0f * (1.0f - nSquared);
        auto b2 = 1.0f;
        auto a1 = 1.0f;
        auto a2 = b1;

        float coeffs[4][8] =
            {
                {0.0f, 0.0f, 0.0f, b0, b1, b2, a1, a2},
                {0.0f, 0.0f, b0, b1, b2, 0.0f, a2, 0.0f},
                {0.0f, b0, b1, b2, 0.0f, 0.0f, 0.0f, 0.0f},
                {b0, b1, b2, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            };

        for (int ii = 0; ii < 8; ii++)
        {
            // add a1*row[0] to row[1]
            coeffs[1][ii] += a1 * coeffs[0][ii];
            // add a1*row[1] + a2*row[0] to row 2
            coeffs[2][ii] += a1 * coeffs[1][ii] + a2 * coeffs[0][ii];
            // add a1*row[2] + a2*row[1] to row 3
            coeffs[3][ii] += a1 * coeffs[2][ii] + a2 * coeffs[1][ii];
        }

        for (int i = 0; i < 4; i++) {
            coeff_xp3[i] = coeffs[i][0];
            coeff_xp2[i] = coeffs[i][1];
            coeff_xp1[i] = coeffs[i][2];
            coeff_x0[i] = coeffs[i][3];
            coeff_xm1[i] = coeffs[i][4];
            coeff_xm2[i] = coeffs[i][5];
            coeff_ym1[i] = coeffs[i][6];
            coeff_ym2[i] = coeffs[i][7];
        }
    }

    void processBlock(dsp::AudioBlock<float> block, int channel)
    {
        // TRACE_EVENT("dsp", "SIMDIIR::processBlock");
        auto numSamples = block.getNumSamples();

        // copy block data to float array using juce::FloatVectorOperations
        float inputArr[block.getNumSamples()];
        FloatVectorOperations::copy(inputArr, block.getChannelPointer(channel), numSamples);
        FloatVectorOperations::multiply(inputArr, 2.0f, numSamples);

        // convert from scalar state block
        float value_xm2 = x2;
        float value_xm1 = x1;
        float value_ym2 = y2;
        float value_ym1 = y1;

        // vectorised loop
        for (int ii = 0; ii < numSamples; ii += 4)
        {
            // vector float x0123 = *ptr++; // load four floats
            float value_x0 = inputArr[ii];
            float value_xp1 = inputArr[ii+1];
            float value_xp2 = inputArr[ii+2];
            float value_xp3 = inputArr[ii+3];

            // calculate output samples
            float y0123[4];
            FloatVectorOperations::addWithMultiply(y0123, coeff_xp3, value_xp3, 4);
            FloatVectorOperations::addWithMultiply(y0123, coeff_xp2, value_xp2, 4);
            FloatVectorOperations::addWithMultiply(y0123, coeff_xp1, value_xp1, 4);
            FloatVectorOperations::addWithMultiply(y0123, coeff_x0, value_x0, 4);
            FloatVectorOperations::addWithMultiply(y0123, coeff_xm1, value_xm1, 4);
            FloatVectorOperations::addWithMultiply(y0123, coeff_xm2, value_xm2, 4);
            FloatVectorOperations::addWithMultiply(y0123, coeff_ym1, value_ym1, 4);
            FloatVectorOperations::addWithMultiply(y0123, coeff_ym2, value_ym2, 4);

            // write output samples
            for (int jj = 0; jj < 4; jj++)
            {
                block.setSample(channel, ii + jj, y0123[jj]);
            }

            // update recurrence
            value_xm2 = value_xp2;
            value_xm1 = value_xp3;
            value_ym2 = y0123[2];
            value_ym1 = y0123[3];
        }

        x2 = value_xm2;
        x1 = value_xm1;
        y2 = value_ym2;
        y1 = value_ym1;
    }

private:
    float state[2] = {0.0f, 0.0f};
    float sampleRate = 44100.0f;

    float x2 = 0.0f;
    float x1 = 0.0f;
    float y2 = 0.0f;
    float y1 = 0.0f;

    float coeff_xp3[4];
    float coeff_xp2[4];
    float coeff_xp1[4];
    float coeff_x0[4];
    float coeff_xm1[4];
    float coeff_xm2[4];
    float coeff_ym1[4];
    float coeff_ym2[4];
};

class SIMDAllPassChain
{
public:
    SIMDAllPassChain(juce::AudioProcessorValueTreeState &treeState) : allPassFrequency(treeState, "allPassFreq"),
                                                                      allPassQ(treeState, "allPassQ"),
                                                                      allPassAmount(treeState, "allPassAmount") {}
    ~SIMDAllPassChain()
    {
    }

    void processBlock(dsp::AudioBlock<float> &block)
    {
        // TRACE_EVENT("dsp", "SIMDAllPAss::processBlock");
        allPassAmount.update();
        allPassFrequency.update();
        allPassQ.update();

        float amount = allPassAmount.getRaw();
        float freq = allPassFrequency.getRaw();
        float q = allPassQ.getRaw();

        // continue here

        if (!((oldAllPassFreq == freq) && (oldAllPassQ == q) && (oldAllPassAmount == amount))) {
            for (int i = 0; i < amount; i++) {
                iirL[i].calcCoefficients(sampleRate, freq, q);
                iirR[i].calcCoefficients(sampleRate, freq, q);
            }
            oldAllPassFreq = freq;
            oldAllPassQ = q;
            oldAllPassAmount = amount;
        }

        for (int i = 0; i < amount; i++)
        {
            iirL[i].processBlock(block, 0);
            iirR[i].processBlock(block, 1);
        }
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        for (int i = 0; i < 50; i++)
        {
            iirL[i].calcCoefficients(spec.sampleRate, allPassFrequency.getRaw(), allPassQ.getRaw());
            iirR[i].calcCoefficients(spec.sampleRate, allPassFrequency.getRaw(), allPassQ.getRaw());
        }
        sampleRate = spec.sampleRate;
    }

private:
    SmoothParam allPassFrequency;
    SmoothParam allPassQ;
    SmoothParam allPassAmount;

    float oldAllPassFreq = 0.0;
    float oldAllPassQ = 0.0;
    float oldAllPassAmount = 0.0;

    float oldSampleRate;
    float sampleRate;

    dsp::IIR::Coefficients<float>::Ptr iirCoefficients;
    SIMDIIR iirL[50];
    SIMDIIR iirR[50];

    dsp::AudioBlock<dsp::SIMDRegister<float>> interleaved;
    dsp::AudioBlock<float> zero;

    HeapBlock<char> interleavedBlockData, zeroData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SIMDAllPassChain)
};