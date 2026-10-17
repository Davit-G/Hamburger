#include "Scope.h"

void Scope::drawEmphasisView(juce::Graphics &g, juce::Rectangle<int> area)
{
    drawTiledContextLabel(g, area, "EMPHASIS");
    drawSpectrumEmphasis(g, area.toFloat());
}

// the newest fft's worth of the history, smoothed into the spectrum the emphasis view draws
void Scope::updateSpectrum()
{
    const auto fftSize = fft.getSize();

    if (fftHistory.size() >= fftSize)
    {
        std::fill(spectrumData.begin(), spectrumData.end(), 0.0f);
        std::fill(spectrumTransformed.begin(), spectrumTransformed.end(), 0.0f);

        for (int i = 0; i < fftSize; ++i)
        {
            const auto historyIndex = (fftHistoryWritePosition + i + fftHistory.size() - fftSize) % fftHistory.size();
            spectrumData[i] = fftHistory[historyIndex];
        }

        windowFun.multiplyWithWindowingTable(spectrumData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform(spectrumData.data(), true);

        static constexpr auto mindB = -70.0f;
        static constexpr auto maxdB = 50.0f;
        const auto binsToRender = juce::jmin<int>((int) spectrumTransformed.size(), fftSize / 2 + 1);

        for (int i = 0; i < binsToRender; ++i)
        {
            const auto magnitude = juce::jlimit(1.0e-6f, 1.0e6f, spectrumData[i]);
            const auto db = juce::Decibels::gainToDecibels(magnitude, -70.0f);
            const auto normalized = juce::jmap(
                db,
                mindB,
                maxdB,
                0.0f,
                1.0f);

            const auto smoothed = averagedSpectrum[i] * scope_constants::spectrumSmoothing + normalized * (1.0f - scope_constants::spectrumSmoothing);
            averagedSpectrum[i] = smoothed;
            spectrumTransformed[i] = smoothed;
        }
    }
}

void Scope::drawSpectrumEmphasis(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    if (spectrumTransformed.size() > 1)
    {
        const auto w = scopeRect.getWidth();
        const auto h = scopeRect.getHeight();

        g.setColour(juce::Colour::fromRGB(255 * 0.6, 255 * 0.6, 0));
        const auto centerY = h;
        const auto maxHeight = h - headerHeight(scopeRect);
        const auto binCount = static_cast<double>(spectrumTransformed.size());
        const auto binToHz = dataCollector.getSampleRate() / static_cast<double>(scope_constants::fftSize);
        const auto nyquist = binToHz * (binCount - 1.0);
        constexpr auto tiltDbPerOctave = 4.5f;
        constexpr auto mindB = -70.0f;
        constexpr auto maxdB = 18.0f;

        juce::Path spectrumPath;
        bool startedSpectrumPath = false;

        for (size_t i = 0; i < spectrumTransformed.size(); ++i)
        {
            const auto freq = static_cast<double>(i) * binToHz;

            if (freq < scope_constants::minDrawFreq)
                continue;

            const auto x = freqToX(freq, w);

            const auto dbValue = juce::jmap(spectrumTransformed[i], 0.0f, 1.0f, mindB, maxdB);
            const auto octaveIndex = std::max(0.0f, std::log2(static_cast<float>(nyquist / freq)));
            const auto tiltDb = -tiltDbPerOctave * octaveIndex;
            const auto adjustedDb = dbValue + tiltDb;
            const auto magnitude = juce::jlimit(0.0f, 1.0f, juce::jmap(
                juce::jlimit(mindB, maxdB, adjustedDb),
                mindB,
                maxdB,
                0.0f,
                1.0f));
            const auto y = centerY - magnitude * maxHeight;

            if (!startedSpectrumPath)
            {
                spectrumPath.startNewSubPath(x, y);
                startedSpectrumPath = true;
            }
            else
            {
                spectrumPath.lineTo(x, y);
            }

            // this bin already reached the right hand edge, anything past it is off screen
            if (freq >= scope_constants::maxDrawFreq)
                break;
        }

        if (startedSpectrumPath)
            g.strokePath(spectrumPath, juce::PathStrokeType(2.f));
        drawResponseCurve(g, w, centerY, maxHeight);
    }

    drawParamHeader(g, scopeRect, { formatFrequency(lowFreqParam->get()), formatFrequency(highFreqParam->get()) });
}

// single source of truth for the x axis, everything drawn over the spectrum has to go through this
float Scope::freqToX(double freq, float w) const
{
    const auto clamped = juce::jlimit(scope_constants::minDrawFreq, scope_constants::maxDrawFreq, freq);

    return static_cast<float>(juce::jmap(std::log10(clamped),
                                              std::log10(scope_constants::minDrawFreq),
                                              std::log10(scope_constants::maxDrawFreq),
                                              0.0,
                                              static_cast<double>(w)));
}

float Scope::getBandResponse(double freq, double centerFreq, double gainDb)
{
    if (!std::isfinite(freq) || freq <= 0.0 || !std::isfinite(centerFreq) || centerFreq <= 0.0 || !std::isfinite(gainDb))
        return 0.5;

    const auto bandwidth = 0.35;
    const auto amplitude = juce::jlimit(-1.0, 1.0, gainDb / 18.0);
    const auto logFreq = std::log10(std::max(freq, 1.0));
    const auto logCenter = std::log10(std::max(centerFreq, 1.0));
    const auto distance = (logFreq - logCenter) / bandwidth;
    const auto bell = std::exp(-0.5 * distance * distance);
    return juce::jlimit(0.0, 1.0, 0.5 + bell * amplitude * 0.5);
}

void Scope::drawResponseCurve(juce::Graphics &g, const float w, const float centerY, const float maxHeight)
{
    const auto lowFreq = lowFreqParam->get();
    const auto highFreq = highFreqParam->get();
    const auto lowGainDb = lowGainParam->get();
    const auto highGainDb = highGainParam->get();

    juce::Path eqPath, eqInversePath;
    bool startedPaths = false;
    constexpr int numPoints = 220;
    const auto logMin = std::log10(scope_constants::minDrawFreq);
    const auto logMax = std::log10(scope_constants::maxDrawFreq);

    for (int i = 0; i <= numPoints; ++i)
    {
        const auto logT = static_cast<double>(i) / static_cast<double>(numPoints);
        const auto logFreq = logMin + logT * (logMax - logMin);
        const auto freq = std::pow(10.0, logFreq);
        const auto lowResponse = getBandResponse(freq, lowFreq, lowGainDb);
        const auto highResponse = getBandResponse(freq, highFreq, highGainDb);
        const auto responseToDraw = juce::jlimit(0.0, 1.0, 0.5 + (lowResponse - 0.5) + (highResponse - 0.5));

        const auto x = freqToX(freq, w);
        const auto y = centerY - responseToDraw * maxHeight;
        const auto inverseY = centerY - (1.0 - responseToDraw) * maxHeight;

        if (!std::isfinite(x) || !std::isfinite(y))
            continue;

        if (!startedPaths) {
            eqPath.startNewSubPath(static_cast<float>(x), static_cast<float>(y));
            eqInversePath.startNewSubPath(static_cast<float>(x), static_cast<float>(inverseY));
            startedPaths = true;
        }
        else {
            eqPath.lineTo(static_cast<float>(x), static_cast<float>(y));
            eqInversePath.lineTo(static_cast<float>(x), static_cast<float>(inverseY));
        }
    }

    if (!startedPaths)
        return;

    // draw inverse path first so white can draw over it
    g.setColour(juce::Colours::grey);
    g.strokePath(eqInversePath, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colours::white);
    g.strokePath(eqPath, juce::PathStrokeType(2.0f));

    // placed with the same summed response the curve is built from, so they sit on it rather than
    // on their own band's bell
    constexpr auto dotRadius = 3.0f;

    for (const auto freq : { lowFreq, highFreq })
    {
        const auto response = juce::jlimit(0.0, 1.0, 0.5 + (getBandResponse(freq, lowFreq, lowGainDb) - 0.5)
                                                        + (getBandResponse(freq, highFreq, highGainDb) - 0.5));
        const auto x = static_cast<float>(freqToX(freq, w));
        const auto y = static_cast<float>(centerY - static_cast<float>(response) * maxHeight);

        g.fillEllipse(x - dotRadius, y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }
}
