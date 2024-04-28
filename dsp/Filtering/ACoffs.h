#pragma once

#include <cmath>
#include <array>

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

template <typename NumericType>
struct ACoffs
{
    /** Returns the coefficients for a first order low-pass filter. */
    static std::array<NumericType, 4> makeFirstOrderLowPass(double sampleRate, NumericType frequency);

    /** Returns the coefficients for a first order high-pass filter. */
    static std::array<NumericType, 4> makeFirstOrderHighPass(double sampleRate, NumericType frequency);

    /** Returns the coefficients for a first order all-pass filter. */
    static std::array<NumericType, 4> makeFirstOrderAllPass(double sampleRate, NumericType frequency);

    /** Returns the coefficients for a low-pass filter. */
    static std::array<NumericType, 6> makeLowPass(double sampleRate, NumericType frequency);

    /** Returns the coefficients for a low-pass filter with variable Q. */
    static std::array<NumericType, 6> makeLowPass(double sampleRate, NumericType frequency, NumericType Q);

    /** Returns the coefficients for a high-pass filter. */
    static std::array<NumericType, 6> makeHighPass(double sampleRate, NumericType frequency);

    /** Returns the coefficients for a high-pass filter with variable Q. */
    static std::array<NumericType, 6> makeHighPass(double sampleRate, NumericType frequency, NumericType Q);

    /** Returns the coefficients for a band-pass filter. */
    static std::array<NumericType, 6> makeBandPass(double sampleRate, NumericType frequency);

    /** Returns the coefficients for a band-pass filter with variable Q. */
    static std::array<NumericType, 6> makeBandPass(double sampleRate, NumericType frequency, NumericType Q);

    /** Returns the coefficients for a notch filter. */
    static std::array<NumericType, 6> makeNotch(double sampleRate, NumericType frequency);

    /** Returns the coefficients for a notch filter with variable Q. */
    static std::array<NumericType, 6> makeNotch(double sampleRate, NumericType frequency, NumericType Q);

    /** Returns the coefficients for an all-pass filter. */
    static std::array<NumericType, 6> makeAllPass(double sampleRate, NumericType frequency);

    /** Returns the coefficients for an all-pass filter with variable Q. */
    static std::array<NumericType, 6> makeAllPass(double sampleRate, NumericType frequency, NumericType Q);

    /** Returns the coefficients for a low-pass shelf filter with variable Q and gain.

        The gain is a scale factor that the low frequencies are multiplied by, so values
        greater than 1.0 will boost the low frequencies, values less than 1.0 will
        attenuate them.
    */
    static std::array<NumericType, 6> makeLowShelf(double sampleRate,
                                                   NumericType cutOffFrequency,
                                                   NumericType Q,
                                                   NumericType gainFactor);

    /** Returns the coefficients for a high-pass shelf filter with variable Q and gain.

        The gain is a scale factor that the high frequencies are multiplied by, so values
        greater than 1.0 will boost the high frequencies, values less than 1.0 will
        attenuate them.
    */
    static std::array<NumericType, 6> makeHighShelf(double sampleRate,
                                                    NumericType cutOffFrequency,
                                                    NumericType Q,
                                                    NumericType gainFactor);

    /** Returns the coefficients for a peak filter centred around a
        given frequency, with a variable Q and gain.

        The gain is a scale factor that the centre frequencies are multiplied by, so
        values greater than 1.0 will boost the centre frequencies, values less than
        1.0 will attenuate them.
    */
    static std::array<std::array<NumericType, 6>, 2> makeDualPeakFilter(double sampleRate,
                                                     NumericType centreFrequency,
                                                     NumericType Q,
                                                     NumericType gainFactor);

private:
    // Unfortunately, std::sqrt is not marked as constexpr just yet in all compilers
    static constexpr NumericType inverseRootTwo = static_cast<NumericType>(0.70710678118654752440L);
};
