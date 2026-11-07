// clang++ -std=c++17 -O2 dsptests/chebyshev_highpass.cpp -o /tmp/cheb && /tmp/cheb
// the phase distortion shifter's foldback highpass: flat above its cutoff, sharp below, and stable down to 5 Hz oversampled
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <utility>

namespace juce
{
    template <typename T> struct MathConstants { static constexpr T pi = (T) 3.141592653589793238, twoPi = (T) 6.283185307179586477; };
    template <typename T> T jmin (T a, T b) { return std::min (a, b); }
    namespace dsp { struct ProcessSpec { double sampleRate; }; }
}

#include "../dsp/FrequencyShifting/HilbertBiquad.h"

static double gainDb (double hz, double cutoff, double sampleRate)
{
    ChebyshevHighpass filter;
    filter.setCutoff (cutoff, sampleRate);

    auto peak = 0.0;
    const auto length = (int) (sampleRate * 4.0);

    for (int i = 0; i < length; ++i)
    {
        const auto y = filter.processSample (std::sin (juce::MathConstants<double>::twoPi * hz * i / sampleRate));

        if (i > length / 2)
            peak = std::max (peak, std::abs (y));
    }

    return 20.0 * std::log10 (peak);
}

int main()
{
    for (const auto cutoff : { 5.0, 100.0, 2000.0 })
    {
        const auto below = gainDb (cutoff * 0.5, cutoff, 176400.0), at = gainDb (cutoff, cutoff, 176400.0), above = gainDb (cutoff * 4.0, cutoff, 176400.0);
        std::printf ("%g Hz: octave below %.1f dB, at cutoff %.1f dB, two octaves above %.1f dB\n", cutoff, below, at, above);

        assert (below < -45.0);
        assert (at > -1.5 && at < 0.1);
        assert (above > -1.1 && above < 0.1);
    }
}
