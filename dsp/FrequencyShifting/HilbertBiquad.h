#pragma once

#include <cmath>
#include "juce_dsp/juce_dsp.h"
// Thank you https://www.katjaas.nl/hilbert/hilbert.html

// this is easy to understand but might be slow ish 

class CustomBiquadFilter {
public:
    CustomBiquadFilter(float a0, float a1, float a2, float b1, float b2)
        : a0_(a0), a1_(a1), a2_(a2), b1_(b1), b2_(b2), x1_(0), x2_(0), y1_(0), y2_(0) {}

    float processSample(float x) {
        float y = a0_ * x + a1_ * x1_ + a2_ * x2_ - b1_ * y1_ - b2_ * y2_;

        // Update delay lines
        x2_ = x1_;
        x1_ = x;
        y2_ = y1_;
        y1_ = y;

        return y;
    }

private:
    float a0_, a1_, a2_, b1_, b2_; // Filter coefficients
    float x1_, x2_, y1_, y2_; // Delay lines for x and y
};

class OneSampleDelay {
public:
    OneSampleDelay() : prevSample_(0) {}

    float processSample(float x) {
        float y = prevSample_;
        prevSample_ = x;
        return y;
    }

private:
    float prevSample_;
};

class HilbertBiquadShifter {
public:

    HilbertBiquadShifter() {

    }
    ~HilbertBiquadShifter() {}

    void prepare(juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;
    }

    float processSample(float x, float phaseIncrement) { return 2 * shift(x, phaseIncrement); }

    // at unity gain, up by phaseIncrement cycles a sample at 44.1kHz, down when it's negative
    float shift(float x, float phaseIncrement) {
        const auto [real, imag] = analytic(x);

        curPhase = fmodf(curPhase + phaseIncrement  * (44100.0 / sampleRate), 1.f); // with compensation for different sample rates, i know im supposed to change the biquads as well but eh
        float theta = 2 * juce::MathConstants<float>::pi * curPhase;
        return real * std::cos(theta) - imag * std::sin(theta);
    }

    float rotate(float x, float cosTheta, float sinTheta) {
        const auto [real, imag] = analytic(x);
        return real * cosTheta + imag * sinTheta;
    }

private:
    std::pair<float, float> analytic(float x) {
        float real = x, imag = delay.processSample(x);

        for (auto* stage : { &bql1, &bql2, &bql3, &bql4 })
            real = stage->processSample(real);

        for (auto* stage : { &bqr1, &bqr2, &bqr3, &bqr4 })
            imag = stage->processSample(imag);

        return { real, imag };
    }

    // todo for future: use polyphase designer for other sample rates

    CustomBiquadFilter bql1{0.161758, 0., -1, 0, -0.1617158};
    CustomBiquadFilter bql2{0.733029, 0, -1, 0, -0.733029};
    CustomBiquadFilter bql3{0.94535, 0., -1, 0, -0.94535};
    CustomBiquadFilter bql4{0.990598, 0., -1, 0, -0.990598};

    CustomBiquadFilter bqr1{0.479401, 0., -1, 0, -0.479401};
    CustomBiquadFilter bqr2{0.876218, 0., -1, 0, -0.876218};
    CustomBiquadFilter bqr3{0.976599, 0., -1, 0, -0.976599};
    CustomBiquadFilter bqr4{0.9975, 0., -1, 0, -0.9975};

    OneSampleDelay delay;

    float curPhase = 0.0f; // phase is incremented

    double sampleRate = 44100;
};

/*  A 6th order butterworth highpass or lowpass, as three biquads, each the RBJ filter at the cutoff with one pole pair's
    Q. Butterworth rather than anything steeper, which would ring on at the cutoff after every transient. In doubles: at
    the lowest cutoffs the poles sit so close to the unit circle that floats go unstable. */
class Butterworth
{
public:
    void setCutoff (double hz, double sampleRate, bool lowpass = false)
    {
        const auto w = juce::MathConstants<double>::twoPi * juce::jmin (hz, sampleRate * 0.49) / sampleRate;
        const auto cosW = std::cos (w);

        for (int k = 0; k < sections; ++k)
        {
            // each pole pair's Q is 1 / (2 sin theta), so alpha = sin w / 2Q comes to sin w sin theta
            const auto theta = juce::MathConstants<double>::pi * (2 * k + 1) / (4 * sections);
            const auto alpha = std::sin (w) * std::sin (theta);
            const auto a0 = 1.0 + alpha;

            const auto b1 = (lowpass ? 1.0 - cosW : -(1.0 + cosW)) / a0;
            coefficients[(size_t) k] = { std::abs (b1) * 0.5, b1, std::abs (b1) * 0.5, -2.0 * cosW / a0, (1.0 - alpha) / a0 };
        }
    }

    double processSample (double x)
    {
        for (int k = 0; k < sections; ++k)
        {
            const auto& [b0, b1, b2, a1, a2] = coefficients[(size_t) k];
            auto& [z1, z2] = state[(size_t) k];

            const auto y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            x = y;
        }

        return x;
    }

    void reset() { state = {}; }

private:
    static constexpr int sections = 3;

    struct Coefficients { double b0, b1, b2, a1, a2; };
    struct State { double z1, z2; };

    std::array<Coefficients, sections> coefficients {};
    std::array<State, sections> state {};
};
