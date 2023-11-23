#pragma once
#include <Juceheader.h>
#include "../SmoothParam.h"
#include <cmath>

// the following code is taken and significantly modified from chowutils_dsp
// want to squeeze as much perf out as physically possible.

/**
 * A State Variable Filter, as derived by Andy Simper (Cytomic).
 *
 * Reference: https://cytomic.com/files/dsp/SvfLinearTrapAllOutputs.pdf
 */
template <typename SampleType>
class StateVariableFilter
{
public:
    template <typename T, bool = std::is_floating_point_v<T> || std::is_same_v<T, std::complex<float>> || std::is_same_v<T, std::complex<double>>>
    struct TypeTraits
    {
        using ElementType = T;
        static constexpr int Size = 1;
    };

    using NumericType = typename TypeTraits<SampleType>::ElementType;

    /** Constructor. */
    StateVariableFilter() {};

    /** Initialises the filter. */
    void prepare(const juce::dsp::ProcessSpec &spec);

    /** Resets the internal state variables of the filter. */
    void reset();

    /**
     * Ensure that the state variables are rounded to zero if the state
     * variables are denormals. This is only needed if you are doing
     * sample by sample processing.
     */
    void snapToZero() noexcept;

    SampleType cutoffFrequency, resonance, gain; // parameters
    SampleType g0, k0, sqrtA;                    // parameter intermediate values
    SampleType a1, a2, a3, ak;                   // coefficients
    std::vector<SampleType> ic1eq, ic2eq;        // state variables

    inline auto processCore(SampleType x, SampleType &s1, SampleType &s2) noexcept
    {
        const auto v3 = x - s2;
        const auto v0 = a1 * v3 - ak * s1;
        const auto v1 = a2 * v3 + a1 * s1;
        const auto v2 = a3 * v3 + a2 * s1 + s2;

        // update state
        s1 = two * v1 - s1;
        s2 = two * v2 - s2;

        return v2 + v0 - k0 * v1;
    }

private:
    NumericType two = (NumericType)2;

    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateVariableFilter)
};

template <typename SampleType>
void StateVariableFilter<SampleType>::prepare(const dsp::ProcessSpec &spec)
{
    jassert(spec.sampleRate > 0);
    jassert(spec.numChannels > 0);

    sampleRate = spec.sampleRate;

    ic1eq.resize(spec.numChannels);
    ic2eq.resize(spec.numChannels);

    reset();
}

template <typename SampleType>
void StateVariableFilter<SampleType>::reset()
{
    auto zero = static_cast<SampleType>(0);
    for (auto v : {&ic1eq, &ic2eq})
        std::fill(v->begin(), v->end(), zero);
}

template <typename SampleType>
void StateVariableFilter<SampleType>::snapToZero() noexcept // NOSONAR (cannot be const)
{
#if JUCE_SNAP_TO_ZERO
    for (auto v : {&ic1eq, &ic2eq})
        for (auto &element : *v)
            juce::dsp::util::snapToZero(element);
#endif
}

// ok onwards to my code

template <typename T>
static T *toBasePointer(dsp::SIMDRegister<T> *r) noexcept { return reinterpret_cast<T *>(r); }
constexpr auto registerSize = dsp::SIMDRegister<float>::size();
using Format = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

class SVFAllPassChain
{
public:
    SVFAllPassChain(juce::AudioProcessorValueTreeState &treeState) : allPassFrequency(treeState, "allPassFreq"),
                                                                     allPassQ(treeState, "allPassQ"),
                                                                     allPassAmount(treeState, "allPassAmount") {}

    template <typename SampleType>
    auto prepareChannelPointers(const dsp::AudioBlock<SampleType> &block)
    {
        // pads the input channels with zero buffers if the number of channels is less than the register size
        std::array<SampleType *, registerSize> result{};

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
                                         registerSize,
                                     },
                                     AudioData::InterleavedDest<Format>{toBasePointer(interleavedSubBlock.getChannelPointer(0)), registerSize}, numSamples);

        dsp::ProcessContextReplacing<dsp::SIMDRegister<float>> newCtx(interleavedSubBlock);

        updateAllCoefficients(sampleRate, allPassFrequency.getRaw(), allPassQ.getRaw());

        float allPassAmt = fmin(allPassAmount.getRaw(), 49.0f);

        TRACE_EVENT_BEGIN("dsp", "SVFAllPassChain loop");

        auto inputSamples = interleavedSubBlock.getChannelPointer(0);
        auto outputSamples = interleavedSubBlock.getChannelPointer(0);

        for (size_t i = 0; i < allPassAmt; i++)
        {
            auto &svf_ref = svf[i];

            auto s1 = svf_ref.ic1eq[0];
            auto s2 = svf_ref.ic2eq[0];

            for (size_t i = 0; i < numSamples; ++i)
                outputSamples[i] = svf_ref.processCore(inputSamples[i], s1, s2); // will rewrite s1 and s2

            svf_ref.ic1eq[0] = s1;
            svf_ref.ic2eq[0] = s2;
        }

        TRACE_EVENT_END("dsp");

        auto outChannels = prepareChannelPointers(context.getOutputBlock());

        AudioData::deinterleaveSamples(AudioData::InterleavedSource<Format>{toBasePointer(interleavedSubBlock.getChannelPointer(0)), registerSize},
                                       AudioData::NonInterleavedDest<Format>{outChannels.data(), registerSize},
                                       numSamples);
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        allPassFrequency.prepare(spec);
        allPassQ.prepare(spec);
        allPassAmount.prepare(spec);
        sampleRate = spec.sampleRate;
        // todo: initialise sample rate for filter, maybe change freq depending on sample rate? idk

        interleaved = dsp::AudioBlock<dsp::SIMDRegister<float>>(interleavedBlockData, 1, spec.maximumBlockSize);
        zero = dsp::AudioBlock<float>(zeroData, dsp::SIMDRegister<float>::size(), spec.maximumBlockSize);

        zero.clear();

        auto monoSpec = spec;
        monoSpec.numChannels = 1;

        for (size_t i = 0; i < 50; i++)
        {
            svf[i].reset();
            svf[i].prepare(monoSpec);
        }
    }

    void updateAllCoefficients(float sampleRate, float cutoff, float resonance)
    {
        if (oldFreq == cutoff && oldQ == resonance)
            return; // no unnecessary updates will increase performance

        const float w = juce::MathConstants<float>::pi * (cutoff / sampleRate);

        const float g0 = dsp::FastMathApproximations::tan(w);
        const SIMDReg g0cast = (SIMDReg)g0;

        const float k0 = (1.0f / resonance);
        const SIMDReg k0cast = (SIMDReg)k0;

        const float gk = g0 + k0;
        const SIMDReg gkCast = (SIMDReg)gk;

        const SIMDReg a1 = (SIMDReg)(1.0f / (g0 * gk + 1.0f));
        const SIMDReg a2 = g0cast * a1;
        const SIMDReg a3 = g0cast * a2;
        const SIMDReg ak = (gkCast * a1);

        for (int i = 0; i < 50; i++)
        {
            auto &svf_ref = this->svf[i];
            svf_ref.g0 = g0cast;
            svf_ref.k0 = k0cast;
            svf_ref.a1 = a1;
            svf_ref.a2 = a2;
            svf_ref.a3 = a3;
            svf_ref.ak = ak;
        }

        oldFreq = cutoff;
        oldQ = resonance;
    }

private:
    float oldFreq = 0.0f;
    float oldQ = 0.0f;

    float sampleRate = 44100.0f;

    using SIMDReg = dsp::SIMDRegister<float>;

    StateVariableFilter<SIMDReg> svf[50];

    SmoothParam allPassFrequency;
    SmoothParam allPassQ;
    SmoothParam allPassAmount;

    dsp::AudioBlock<SIMDReg> interleaved;
    dsp::AudioBlock<float> zero;

    HeapBlock<char> interleavedBlockData, zeroData;
};
