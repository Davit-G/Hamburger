#pragma once

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"

template <typename SampleType>
class SVTPTFilter
{
public:

    SVTPTFilter();

    /** Sets the cutoff frequency of the filter.

        @param newFrequencyHz the new cutoff frequency in Hz.
    */
    void setCutoffFrequency (SampleType newFrequencyHz);

    /** Sets the resonance of the filter.

        Note: The bandwidth of the resonance increases with the value of the
        parameter. To have a standard 12 dB / octave filter, the value must be set
        at 1 / sqrt (2).
    */
    void setResonance (SampleType newResonance);

    /** Returns the cutoff frequency of the filter. */
    SampleType getCutoffFrequency() const noexcept     { return cutoffFrequency; }

    /** Returns the resonance of the filter. */
    SampleType getResonance() const noexcept           { return resonance; }

    //==============================================================================
    /** Initialises the filter. */
    void prepare (const juce::dsp::ProcessSpec& spec);

    /** Resets the internal state variables of the filter. */
    void reset();

    /** Resets the internal state variables of the filter to a given value. */
    void reset (SampleType newValue);

    /** Ensure that the state variables are rounded to zero if the state
        variables are denormals. This is only needed if you are doing
        sample by sample processing.
    */
    void snapToZero() noexcept;

    //==============================================================================
    /** Processes the input and output samples supplied in the processing context. */
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() <= s1.size());
        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples()  == numSamples);

        if (context.isBypassed)
        {
            outputBlock.copyFrom (inputBlock);
            return;
        }

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* inputSamples  = inputBlock .getChannelPointer (channel);
            auto* outputSamples = outputBlock.getChannelPointer (channel);

            for (size_t i = 0; i < numSamples; ++i)
                outputSamples[i] = processSample ((int) channel, inputSamples[i]);
        }

       #if JUCE_DSP_ENABLE_SNAP_TO_ZERO
        snapToZero();
       #endif
    }

    //==============================================================================
    /** Processes one sample at a time on a given channel. */
    inline std::array<SampleType, 3> processSample (int channel, SampleType inputValue);

private:
    //==============================================================================
    void update();

    //==============================================================================
    SampleType g, h, R2;
    std::vector<SampleType> s1 { 2 }, s2 { 2 };

    double sampleRate = 44100.0;
    SampleType cutoffFrequency = static_cast<SampleType> (1000.0),
               resonance       = static_cast<SampleType> (1.0 / std::sqrt (2.0));
};