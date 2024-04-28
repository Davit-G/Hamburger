#include "ACoffs.h"

constexpr auto minimumDecibels = -300.0;

template <typename NumericType>
std::array<NumericType, 4> ACoffs<NumericType>::makeFirstOrderLowPass(double sampleRate,
                                                                                 NumericType frequency)
{
    jassert(sampleRate > 0.0);
    jassert(frequency > 0 && frequency <= static_cast<float>(sampleRate * 0.5));

    const auto n = std::tan(juce::MathConstants<NumericType>::pi * frequency / static_cast<NumericType>(sampleRate));

    return {{n, n, n + 1, n - 1}};
}

template <typename NumericType>
std::array<NumericType, 4> ACoffs<NumericType>::makeFirstOrderHighPass(double sampleRate,
                                                                                  NumericType frequency)
{
    jassert(sampleRate > 0.0);
    jassert(frequency > 0 && frequency <= static_cast<float>(sampleRate * 0.5));

    const auto n = std::tan(juce::MathConstants<NumericType>::pi * frequency / static_cast<NumericType>(sampleRate));

    return {{1, -1, n + 1, n - 1}};
}

template <typename NumericType>
std::array<NumericType, 4> ACoffs<NumericType>::makeFirstOrderAllPass(double sampleRate,
                                                                                 NumericType frequency)
{
    jassert(sampleRate > 0.0);
    jassert(frequency > 0 && frequency <= static_cast<float>(sampleRate * 0.5));

    const auto n = std::tan(juce::MathConstants<NumericType>::pi * frequency / static_cast<NumericType>(sampleRate));

    return {{n - 1, n + 1, n + 1, n - 1}};
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeLowPass(double sampleRate,
                                                                       NumericType frequency)
{
    return makeLowPass(sampleRate, frequency, inverseRootTwo);
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeLowPass(double sampleRate,
                                                                       NumericType frequency,
                                                                       NumericType Q)
{
    jassert(sampleRate > 0.0);
    jassert(frequency > 0 && frequency <= static_cast<float>(sampleRate * 0.5));
    jassert(Q > 0.0);

    const auto n = 1 / std::tan(juce::MathConstants<NumericType>::pi * frequency / static_cast<NumericType>(sampleRate));
    const auto nSquared = n * n;
    const auto invQ = 1 / Q;
    const auto c1 = 1 / (1 + invQ * n + nSquared);

    return {{c1, c1 * 2, c1,
             1, c1 * 2 * (1 - nSquared),
             c1 * (1 - invQ * n + nSquared)}};
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeHighPass(double sampleRate,
                                                                        NumericType frequency)
{
    return makeHighPass(sampleRate, frequency, inverseRootTwo);
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeHighPass(double sampleRate,
                                                                        NumericType frequency,
                                                                        NumericType Q)
{
    jassert(sampleRate > 0.0);
    jassert(frequency > 0 && frequency <= static_cast<float>(sampleRate * 0.5));
    jassert(Q > 0.0);

    const auto n = std::tan(juce::MathConstants<NumericType>::pi * frequency / static_cast<NumericType>(sampleRate));
    const auto nSquared = n * n;
    const auto invQ = 1 / Q;
    const auto c1 = 1 / (1 + invQ * n + nSquared);

    return {{c1, c1 * -2, c1,
             1, c1 * 2 * (nSquared - 1),
             c1 * (1 - invQ * n + nSquared)}};
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeBandPass(double sampleRate,
                                                                        NumericType frequency)
{
    return makeBandPass(sampleRate, frequency, inverseRootTwo);
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeBandPass(double sampleRate,
                                                                        NumericType frequency,
                                                                        NumericType Q)
{
    jassert(sampleRate > 0.0);
    jassert(frequency > 0 && frequency <= static_cast<float>(sampleRate * 0.5));
    jassert(Q > 0.0);

    const auto n = 1 / std::tan(juce::MathConstants<NumericType>::pi * frequency / static_cast<NumericType>(sampleRate));
    const auto nSquared = n * n;
    const auto invQ = 1 / Q;
    const auto c1 = 1 / (1 + invQ * n + nSquared);

    return {{c1 * n * invQ, 0,
             -c1 * n * invQ, 1,
             c1 * 2 * (1 - nSquared),
             c1 * (1 - invQ * n + nSquared)}};
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeNotch(double sampleRate,
                                                                     NumericType frequency)
{
    return makeNotch(sampleRate, frequency, inverseRootTwo);
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeNotch(double sampleRate,
                                                                     NumericType frequency,
                                                                     NumericType Q)
{
    jassert(sampleRate > 0.0);
    jassert(frequency > 0 && frequency <= static_cast<float>(sampleRate * 0.5));
    jassert(Q > 0.0);

    const auto n = 1 / std::tan(juce::MathConstants<NumericType>::pi * frequency / static_cast<NumericType>(sampleRate));
    const auto nSquared = n * n;
    const auto invQ = 1 / Q;
    const auto c1 = 1 / (1 + n * invQ + nSquared);
    const auto b0 = c1 * (1 + nSquared);
    const auto b1 = 2 * c1 * (1 - nSquared);

    return {{b0, b1, b0, 1, b1, c1 * (1 - n * invQ + nSquared)}};
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeAllPass(double sampleRate,
                                                                       NumericType frequency)
{
    return makeAllPass(sampleRate, frequency, inverseRootTwo);
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeAllPass(double sampleRate,
                                                                       NumericType frequency,
                                                                       NumericType Q)
{
    jassert(sampleRate > 0);
    jassert(frequency > 0 && frequency <= sampleRate * 0.5);
    jassert(Q > 0);

    const auto n = 1 / std::tan(juce::MathConstants<NumericType>::pi * frequency / static_cast<NumericType>(sampleRate));
    const auto nSquared = n * n;
    const auto invQ = 1 / Q;
    const auto c1 = 1 / (1 + invQ * n + nSquared);
    const auto b0 = c1 * (1 - n * invQ + nSquared);
    const auto b1 = c1 * 2 * (1 - nSquared);

    return {{b0, b1, 1, 1, b1, b0}};
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeLowShelf(double sampleRate,
                                                                        NumericType cutOffFrequency,
                                                                        NumericType Q,
                                                                        NumericType gainFactor)
{
    jassert(sampleRate > 0.0);
    jassert(cutOffFrequency > 0.0 && cutOffFrequency <= sampleRate * 0.5);
    jassert(Q > 0.0);

    const auto A = std::sqrt(juce::Decibels::gainWithLowerBound(gainFactor, (NumericType)minimumDecibels));
    const auto aminus1 = A - 1;
    const auto aplus1 = A + 1;
    const auto omega = (2 * juce::MathConstants<NumericType>::pi * juce::jmax(cutOffFrequency, static_cast<NumericType>(2.0))) / static_cast<NumericType>(sampleRate);
    const auto coso = std::cos(omega);
    const auto beta = std::sin(omega) * std::sqrt(A) / Q;
    const auto aminus1TimesCoso = aminus1 * coso;

    return {{A * (aplus1 - aminus1TimesCoso + beta),
             A * 2 * (aminus1 - aplus1 * coso),
             A * (aplus1 - aminus1TimesCoso - beta),
             aplus1 + aminus1TimesCoso + beta,
             -2 * (aminus1 + aplus1 * coso),
             aplus1 + aminus1TimesCoso - beta}};
}

template <typename NumericType>
std::array<NumericType, 6> ACoffs<NumericType>::makeHighShelf(double sampleRate,
                                                                         NumericType cutOffFrequency,
                                                                         NumericType Q,
                                                                         NumericType gainFactor)
{
    jassert(sampleRate > 0);
    jassert(cutOffFrequency > 0 && cutOffFrequency <= static_cast<NumericType>(sampleRate * 0.5));
    jassert(Q > 0);

    const auto A = std::sqrt(juce::Decibels::gainWithLowerBound(gainFactor, (NumericType)minimumDecibels));
    const auto aminus1 = A - 1;
    const auto aplus1 = A + 1;
    const auto omega = (2 * juce::MathConstants<NumericType>::pi * juce::jmax(cutOffFrequency, static_cast<NumericType>(2.0))) / static_cast<NumericType>(sampleRate);
    const auto coso = std::cos(omega);
    const auto beta = std::sin(omega) * std::sqrt(A) / Q;
    const auto aminus1TimesCoso = aminus1 * coso;

    return {{A * (aplus1 + aminus1TimesCoso + beta),
             A * -2 * (aminus1 + aplus1 * coso),
             A * (aplus1 + aminus1TimesCoso - beta),
             aplus1 - aminus1TimesCoso + beta,
             2 * (aminus1 - aplus1 * coso),
             aplus1 - aminus1TimesCoso - beta}};
}

template <typename NumericType>
std::array<std::array<NumericType, 6>, 2> ACoffs<NumericType>::makeDualPeakFilter(double sampleRate,
                                                                          NumericType frequency,
                                                                          NumericType Q,
                                                                          NumericType gainFactor)
{
    const auto A = std::sqrt(juce::Decibels::gainWithLowerBound(gainFactor, (NumericType)minimumDecibels));
    const auto omega = juce::MathConstants<NumericType>::twoPi * frequency / static_cast<NumericType>(sampleRate);
    const auto alpha = std::sin(omega) / (Q * 2);
    const auto c2 = -2 * std::cos(omega);
    const auto alphaTimesA = alpha * A;
    const auto alphaOverA = alpha / A;

    auto arr1 = std::array<NumericType, 6>{1 + alphaTimesA, c2, 1 - alphaTimesA, 1 + alphaOverA, c2, 1 - alphaOverA};
    auto arr2 = std::array<NumericType, 6>{1 + alphaOverA, c2, 1 - alphaOverA, 1 + alphaTimesA, c2, 1 - alphaTimesA};

    return {arr1, arr2};
}

template struct ACoffs<float>;
template struct ACoffs<double>;