#include "Scope.h"
#include "../../dsp/Filtering/EmphasisFilter.h"

void Scope::drawEmphasisView(juce::Graphics &g, juce::Rectangle<int> area)
{
    drawTiledContextLabel(g, area, "EMPHASIS");
    drawSpectrumEmphasis(g, area.toFloat());
}

void Scope::drawTiltView(juce::Graphics &g, juce::Rectangle<int> area)
{
    drawTiledContextLabel(g, area, "TILT");
    drawSpectrumTilt(g, area.toFloat());
}

void Scope::drawSpectrumEmphasis(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto w = scopeRect.getWidth();
    const auto h = scopeRect.getHeight();
    const auto maxHeight = h - headerHeight(scopeRect);

    spectrum.paint(g, juce::Rectangle<float>(0.0f, (float) (h - maxHeight), (float) w, (float) maxHeight),
                   dataCollector.getSampleRate(), 2.f, theme().spectrumLine, theme().spectrumFill);

    drawResponseCurve(g, w, h, maxHeight);

    drawParamHeader(g, scopeRect, { formatFrequency(lowFreqParam->get()), formatFrequency(highFreqParam->get()) });
}


/*  The tilt twice over the spectrum: grey going into the distortion, white coming out of it. On the same scale as the
    emphasis bands, 18 dB to the top or bottom of the plot. */

void Scope::drawSpectrumTilt(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto w = (float) scopeRect.getWidth();
    const auto h = (float) scopeRect.getHeight();
    const auto maxHeight = h - (float) headerHeight(scopeRect);
    const auto tilt = (double) tiltParam->getScaled();

    spectrum.paint(g, juce::Rectangle<float>(0.0f, h - maxHeight, w, maxHeight), dataCollector.getSampleRate(), 2.f,
                   theme().spectrumLine, theme().spectrumFill);

    auto curveFor = [&](double direction)
    {
        juce::Path path;
        constexpr int numPoints = 220;
        const auto logMin = std::log10(scope_constants::minDrawFreq);
        const auto logMax = std::log10(scope_constants::maxDrawFreq);

        for (int i = 0; i <= numPoints; ++i)
        {
            const auto freq = std::pow(10.0, logMin + (logMax - logMin) * i / numPoints);
            const auto db = EmphasisFilter::tiltResponseDb(freq, tilt * direction);
            const auto y = h - (float) juce::jlimit(0.0, 1.0, 0.5 + db / 36.0) * maxHeight;
            const auto x = (float) freqToX(freq, (float) w);

            if (i == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }

        return path;
    };

    const auto pre = curveFor(-1.0), post = curveFor(1.0);

    // pre first, so white draws over it where they cross at the pivot
    g.setColour(theme().scopeCurve);
    g.strokePath(pre, juce::PathStrokeType(2.0f));
    g.setColour(theme().scopeCurvePost);
    g.strokePath(post, juce::PathStrokeType(2.0f));

    // named at the right hand end, the higher curve's name above it and the other's below, so they never overlap
    g.setFont(getLookAndFeel().getPopupMenuFont().withHeight(12.0f));

    const auto postHigher = post.getCurrentPosition().y <= pre.getCurrentPosition().y;

    auto name = [&](const juce::Path& path, const juce::String& text, juce::Colour colour, bool above)
    {
        const auto end = path.getCurrentPosition();
        g.setColour(colour);
        g.drawText(text, juce::Rectangle<float>(end.x - 44.0f, above ? end.y - 16.0f : end.y + 2.0f, 40.0f, 14.0f),
                   juce::Justification::centredRight, false);
    };

    name(pre, "PRE", theme().scopeCurve, !postHigher);
    name(post, "POST", theme().scopeCurvePost, postHigher);

    drawParamHeader(g, scopeRect, { formatDecibels(tiltParam->getScaled()) });
}

float Scope::freqToX(double freq, float w) const
{
    return static_cast<float>(SpectrumAnalyser::freqToX(freq, static_cast<float>(w)));
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
    const auto lowGainDb = lowGainParam->getScaled();
    const auto highGainDb = highGainParam->getScaled();

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
    g.setColour(theme().scopeCurve);
    g.strokePath(eqInversePath, juce::PathStrokeType(2.0f));

    g.setColour(theme().scopeCurvePost);
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

    // named in the right hand corners like the tilt view's, the curve ending higher up getting the top one.
    // the mirrored curve is the one before the distortion
    const auto postHigher = eqPath.getCurrentPosition().y <= eqInversePath.getCurrentPosition().y;
    const auto top = juce::Rectangle<float>((float) w - 44.0f, (float) (centerY - maxHeight) + 2.0f, 40.0f, 14.0f);
    const auto bottom = top.withY((float) centerY - 16.0f);

    g.setFont(getLookAndFeel().getPopupMenuFont().withHeight(12.0f));

    g.setColour(theme().scopeCurve);
    g.drawText("PRE", postHigher ? bottom : top, juce::Justification::centredRight, false);

    g.setColour(theme().scopeCurvePost);
    g.drawText("POST", postHigher ? top : bottom, juce::Justification::centredRight, false);
}
