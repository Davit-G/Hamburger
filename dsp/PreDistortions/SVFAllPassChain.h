#pragma once
#include <Juceheader.h>
#include "../SmoothParam.h"
#include <cmath>

// the following code is taken and significantly modified from chowutils_dsp

/**
 * A State Variable Filter, as derived by Andy Simper (Cytomic).
 *
 * Reference: https://cytomic.com/files/dsp/SvfLinearTrapAllOutputs.pdf
 */
template <typename SampleType>
class StateVariableFilter
{
public:
    static constexpr int Order = 2;
    using NumericType = dsp::SampleTypeHelpers::ElementType<SampleType>::Type;

    /** Constructor. */
    StateVariableFilter();

    template <bool shouldUpdate = true>
    void setCutoffFrequency(SampleType newFrequencyHz);

    template <bool shouldUpdate = true>
    void setQValue(SampleType newResonance);

    /**
     * Updates the filter coefficients.
     *
     * Don't touch this unless you know what you're doing!
     */
    void update();

    /** Returns the cutoff frequency of the filter. */
    [[nodiscard]] SampleType getCutoffFrequency() const noexcept { return cutoffFrequency; }

    /** Returns the resonance of the filter. */
    [[nodiscard]] SampleType getQValue() const noexcept { return resonance; }

    /** Returns the gain of the filter. */
    [[nodiscard]] SampleType getGain() const noexcept { return gain; }

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

    /** Processes the input and output samples supplied in the processing context. */
    template <typename ProcessContext>
    void process(const ProcessContext &context) noexcept
    {
        const auto &inputBlock = context.getInputBlock();
        auto &outputBlock = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples = outputBlock.getNumSamples();

        jassert(inputBlock.getNumChannels() <= ic1eq.size());
        jassert(inputBlock.getNumChannels() == numChannels);
        jassert(inputBlock.getNumSamples() == numSamples);

        if (context.isBypassed)
        {
            outputBlock.copyFrom(inputBlock);
            return;
        }

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto *inputSamples = inputBlock.getChannelPointer(channel);
            auto *outputSamples = outputBlock.getChannelPointer(channel);

            auto s1 = ic1eq[channel];
            auto s2 = ic2eq[channel];

            for (size_t i = 0; i < numSamples; ++i)
                outputSamples[i] = processCore(inputSamples[i], s1.get(), s2.get());
        }

// #if JUCE_SNAP_TO_ZERO
        snapToZero();
// #endif
    }

    inline auto processSample(int channel, SampleType inputValue) noexcept
    {
        return processCore(inputValue, ic1eq[(size_t)channel], ic2eq[(size_t)channel]);
    }


    SampleType cutoffFrequency, resonance, gain; // parameters
    SampleType g0, k0, sqrtA;                 // parameter intermediate values
    SampleType a1, a2, a3, ak, k0A;         // coefficients
    std::vector<SampleType> ic1eq, ic2eq;        // state variables
    SampleType A = 1.0f;

private:
    inline auto processCore(SampleType x, SampleType &s1, SampleType &s2) noexcept
    {
        const auto v3 = x - s2;
        const auto v0 = a1 * v3 - ak * s1;
        const auto v1 = a2 * v3 + a1 * s1;
        const auto v2 = a3 * v3 + a2 * s1 + s2;

        // update state
        s1 = (NumericType)2 * v1 - s1;
        s2 = (NumericType)2 * v2 - s2;

        return v2 + v0 - k0 * v1;
    }

    

    NumericType lowpassMult{0}, bandpassMult{0}, highpassMult{0};

    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateVariableFilter)
};

template <typename SampleType>
StateVariableFilter<SampleType>::StateVariableFilter()
{
    setCutoffFrequency(static_cast<NumericType>(1000.0));
    setQValue(static_cast<NumericType>(1.0 / juce::MathConstants<double>::sqrt2));
}

template <typename SampleType>
template <bool shouldUpdate>
void StateVariableFilter<SampleType>::setCutoffFrequency(SampleType newCutoffFrequencyHz)
{
    cutoffFrequency = newCutoffFrequencyHz;
    const auto w = juce::MathConstants<NumericType>::pi * cutoffFrequency / (NumericType)sampleRate;

    g0 = dsp::FastMathApproximations::tan(w);

    if constexpr (shouldUpdate)
        update();
}

template <typename SampleType>
template <bool shouldUpdate>
void StateVariableFilter<SampleType>::setQValue(SampleType newResonance)
{
    // jassert (SIMDUtils::all (newResonance > static_cast<NumericType> (0)));

    resonance = newResonance;
    k0 = (NumericType)1.0 / resonance;
    k0A = k0 * A;

    if constexpr (shouldUpdate)
        update();
}

template <typename SampleType>
void StateVariableFilter<SampleType>::prepare(const dsp::ProcessSpec &spec)
{
    jassert(spec.sampleRate > 0);
    jassert(spec.numChannels > 0);

    sampleRate = spec.sampleRate;

    ic1eq.resize(spec.numChannels);
    ic2eq.resize(spec.numChannels);

    reset();

    setCutoffFrequency(cutoffFrequency);
}

template <typename SampleType>
void StateVariableFilter<SampleType>::reset()
{
    for (auto v : {&ic1eq, &ic2eq})
        std::fill(v->begin(), v->end(), static_cast<SampleType>(0));
}

template <typename SampleType>
void StateVariableFilter<SampleType>::snapToZero() noexcept // NOSONAR (cannot be const)
{
// #if JUCE_SNAP_TO_ZERO
    for (auto v : {&ic1eq, &ic2eq})
        for (auto &element : *v)
            juce::dsp::util::snapToZero(element);
// #endif
}

template <typename SampleType>
void StateVariableFilter<SampleType>::update()
{
    SampleType g, k;
    g = g0;
    k = k0;

    const auto gk = g + k;
    a1 = (NumericType)1.0 / ((NumericType)1.0 + g * gk);
    a2 = g * a1;
    a3 = g * a2;
    ak = gk * a1;
}

// ok onwards to my code

class SVFAllPassChain
{
public:
    SVFAllPassChain(juce::AudioProcessorValueTreeState &treeState) : allPassFrequency(treeState, "allPassFreq"),
                                                                     allPassQ(treeState, "allPassQ"),
                                                                     allPassAmount(treeState, "allPassAmount") {}

    void processBlock(dsp::AudioBlock<float> block)
    {
        allPassAmount.updateFloored();
        allPassFrequency.update();
        allPassQ.update();

        for (int j = 0; j < block.getNumSamples(); j++)
        {
            updateAllCoefficients(sampleRate, allPassFrequency.get(), allPassQ.get());

            float allPassAmt = allPassAmount.get();
            float l = block.getSample(0, j);
            float r = block.getSample(1, j);

            float between = fmod(allPassAmt, 1.0f);

            int i;
            for (i = 0; i < fmax(fmin(allPassAmt, 49.0f), 0.0f); i++)
            {
                l = svf[i].processSample(0, l);
                r = svf[i].processSample(1, r);

                block.setSample(0, j, l);
                block.setSample(1, j, r);
            }

            float nextL = svf[i].processSample(0, l);
            float nextR = svf[i].processSample(1, r);

            svf[(int)fmin(i + 1, 49)].processSample(0, nextL); // prepare the next filter ahead of time, so that the next sample is ready
            svf[(int)fmin(i + 1, 49)].processSample(1, nextR);

            l = l * (1.0f - between) + nextL * between;
            r = r * (1.0f - between) + nextR * between;

            block.setSample(0, j, l);
            block.setSample(1, j, r);


        }
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        allPassFrequency.prepare(spec);
        allPassQ.prepare(spec);
        allPassAmount.prepare(spec);
        sampleRate = spec.sampleRate;
        // todo: initialise sample rate for filter, maybe change freq depending on sample rate? idk

        for (int i = 0; i < 50; i++)
        {
            svf[i].reset();
            svf[i].prepare(spec);
        }
    }

    void updateAllCoefficients(float sampleRate, float cutoff, float resonance) {
        const auto w = juce::MathConstants<float>::pi * cutoff / sampleRate;

        auto g0 = dsp::FastMathApproximations::tan(w);

        auto k0 = 1.0f / resonance;
        auto k0A = k0;

        const auto gk = g0 + k0;
        auto a1 = 1.0f / (1.0f + g0 * gk);
        auto a2 = g0 * a1;
        auto a3 = g0 * a2;
        auto ak = gk * a1;

        for (int i = 0; i < 50; i++)
        {
            svf[i].g0 = g0;
            svf[i].k0 = k0;
            svf[i].k0A = k0A;
            svf[i].a1 = a1;
            svf[i].a2 = a2;
            svf[i].a3 = a3;
            svf[i].ak = ak;
        }
    }

private:
    float sampleRate = 44100.0f;

    StateVariableFilter<float> svf[50];

    SmoothParam allPassFrequency;
    SmoothParam allPassQ;
    SmoothParam allPassAmount;
};
