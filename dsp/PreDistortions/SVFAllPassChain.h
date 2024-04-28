#pragma once

#include "../SmoothParam.h"
#include <cmath>

// ok onwards to my code

template <typename T>
static T *toBasePointer(dsp::SIMDRegister<T> *r) noexcept { return reinterpret_cast<T *>(r); }
constexpr auto simdRegSize = dsp::SIMDRegister<float>::size();
using Format = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

class SVFAllPassChain
{
public:
    SVFAllPassChain(juce::AudioProcessorValueTreeState &treeState) : allPassFrequency(treeState, "allPassFreq"),
                                                                     allPassQ(treeState, "allPassQ"),
                                                                     allPassAmount(treeState, "allPassAmount")
    {
        for (size_t i = 0; i < 50; ++i)
        {
            ic1eq[i] = SIMDReg(0.0f);
            ic2eq[i] = SIMDReg(0.0f);
        }
    }

    template <typename SampleType>
    auto prepareChannelPointers(const dsp::AudioBlock<SampleType> &block)
    {
        // pads the input channels with zero buffers if the number of channels is less than the register size
        std::array<SampleType *, simdRegSize> result{};

        for (size_t ch = 0; ch < result.size(); ++ch)
            result[ch] = (ch < block.getNumChannels() ? block.getChannelPointer(ch) : zero.getChannelPointer(ch));

        return result;
    }

    void processBlock(dsp::AudioBlock<float> block)
    {
        allPassAmount.update();
        allPassFrequency.update();
        allPassQ.update();

        auto interleavedSubBlock = interleaved.getSubBlock(0, block.getNumSamples());
        auto context = dsp::ProcessContextReplacing<float>(block);

        const auto &input = context.getInputBlock();
        auto numSamples = block.getNumSamples();

        auto inChannels = prepareChannelPointers(input);

        AudioData::interleaveSamples(AudioData::NonInterleavedSource<Format>{
                                         inChannels.data(),
                                         simdRegSize,
                                     },
                                     AudioData::InterleavedDest<Format>{toBasePointer(interleavedSubBlock.getChannelPointer(0)), simdRegSize}, numSamples);

        dsp::ProcessContextReplacing<dsp::SIMDRegister<float>> newCtx(interleavedSubBlock);

        float allPassAmt = fmin(allPassAmount.getRaw(), 50.0f);

        TRACE_EVENT_BEGIN("dsp", "SVFAllPassChain loop");

        auto samples = interleavedSubBlock.getChannelPointer(0);

        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            updateAllCoefficients(sampleRate, allPassFrequency.getNextValue(), allPassQ.getNextValue());

            auto sampleVal = samples[sample];

            for (size_t i = 0; i < allPassAmt; i++)
            {
                processCore(sampleVal, ic1eq[i], ic2eq[i]);
            }

            samples[sample] = sampleVal;
        }

        TRACE_EVENT_END("dsp");

        auto outChannels = prepareChannelPointers(context.getOutputBlock());

        AudioData::deinterleaveSamples(AudioData::InterleavedSource<Format>{toBasePointer(interleavedSubBlock.getChannelPointer(0)), simdRegSize},
                                       AudioData::NonInterleavedDest<Format>{outChannels.data(), simdRegSize},
                                       numSamples);
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        allPassFrequency.prepare(spec);
        allPassQ.prepare(spec);
        allPassAmount.prepare(spec);
        sampleRate = spec.sampleRate;

        interleaved = dsp::AudioBlock<dsp::SIMDRegister<float>>(interleavedBlockData, 1, spec.maximumBlockSize);
        zero = dsp::AudioBlock<float>(zeroData, dsp::SIMDRegister<float>::size(), spec.maximumBlockSize);
        interleaved.clear();
        zero.clear();

        auto monoSpec = spec;
        monoSpec.numChannels = 1;

        for (size_t i = 0; i < 50; ++i)
        {
            ic1eq[i] = SIMDReg(0.0f);
            ic2eq[i] = SIMDReg(0.0f);
        }
    }

    void updateAllCoefficients(float sampleRate, float cutoff, float resonance)
    {
        if (oldFreq == cutoff && oldQ == resonance)
            return; // no unnecessary updates will increase performance

        const float w = juce::MathConstants<float>::pi * (cutoff / sampleRate);

        g0 = dsp::FastMathApproximations::tan(w);

        k0 = (1.0f / resonance);
        const auto gk = g0 + k0;

        a1 = 1.0f / (g0 * gk + 1.0f);
        a2 = g0 * a1;
        a3 = g0 * a2;
        ak = gk * a1;

        oldFreq = cutoff;
        oldQ = resonance;
    }

private:
    using SIMDReg = dsp::SIMDRegister<float>;

    inline void processCore(SIMDReg &x, SIMDReg &s1, SIMDReg &s2) noexcept
    {
        // compute allpass
        const auto v3 = x - s2;
        const auto v0 = v3 * a1 - s1 * ak;
        const auto v1 = v3 * a2 + s1 * a1;
        const auto v2 = v3 * a3 + s1 * a2 + s2;

        // update state
        s1 = two * v1 - s1;
        s2 = two * v2 - s2;

        x = v2 + v0 - v1 * k0;
    }

    float cutoffFrequency, resonance, gain; // parameters
    float g0, k0, sqrtA = 0.0f;             // parameter intermediate values, shared across all
    float a1, a2, a3, ak = 0.0f;            // coefficients, can be shared across all

    SIMDReg two = 2.0f;

    float oldFreq = 0.0f;
    float oldQ = 0.0f;

    float sampleRate = 44100.0f;

    std::array<SIMDReg, 50> ic1eq, ic2eq;

    dsp::AudioBlock<SIMDReg> interleaved;
    dsp::AudioBlock<float> zero;

    HeapBlock<char> interleavedBlockData, zeroData;

    SmoothParam allPassFrequency;
    SmoothParam allPassQ;
    SmoothParam allPassAmount;
};
