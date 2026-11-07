#include "Scope.h"
#include "../../dsp/Dynamics/Compressor.h"
#include "../../dsp/Dynamics/TypeA.h"

// whichever compressor is running. no watermark here, the cells are opaque and cover everything under the header
void Scope::drawCompressorView(juce::Graphics &g, juce::Rectangle<int> area)
{
    const auto scopeRect = area.toFloat();
    const auto compressionTypeValue = compressionType->getIndex();

    switch (compressionTypeValue) {
        case 0:
            drawStereoComp(g, scopeRect);
            drawParamHeader(g, scopeRect, getCompHeaderLabels(ParamIDs::stereoCompThreshold, false));
            break;
        case 1:
            drawMBComp(g, scopeRect);
            drawParamHeader(g, scopeRect, getCompHeaderLabels(ParamIDs::MBCompThreshold, true));
            break;
        case 2:
            drawMSComp(g, scopeRect);
            drawParamHeader(g, scopeRect, getCompHeaderLabels(ParamIDs::MSCompThreshold, true));
            break;
        case 3:
            drawTypeAComp(g, scopeRect);
            // tilt only moves the makeup gains
            drawParamHeader(g, scopeRect, { formatDecibels(paramValue(ParamIDs::TypeAThreshold)),
                                            juce::String(paramValue(ParamIDs::TypeARatio), 1) + ":1",
                                            formatDecibels(paramValue(ParamIDs::TypeATilt)) });
            break;
        case 4:
            drawTransient(g, scopeRect, { "LEFT", "RIGHT" });
            drawParamHeader(g, scopeRect, { formatDecibels(paramValue(ParamIDs::transientAttack)),
                                            formatDecibels(paramValue(ParamIDs::transientSustain)),
                                            formatPercent(paramValue(ParamIDs::transientLink) * 0.01f) });
            break;
        case 5:
            drawTransient(g, scopeRect, { "LOW", "MID", "HIGH" });
            drawParamHeader(g, scopeRect, { formatDecibels(paramValue(ParamIDs::transientAttack)),
                                            formatDecibels(paramValue(ParamIDs::transientSustain)),
                                            formatPercent(paramValue(ParamIDs::transientTilt) * 0.01f) });
            break;
        case 6:
        case 7:
            drawOptoComp(g, scopeRect, compressionTypeValue == 7);
            break;
        default:
            break;
    }
}

// one cell per band
void Scope::drawCompBands(juce::Graphics &g, juce::Rectangle<float> scopeRect,
                                      const CompBand *bands, int numBands, float ratio)
{
    if (bands == nullptr || numBands <= 0)
        return;

    const auto w = (float) scopeRect.getWidth();
    const auto h = (float) scopeRect.getHeight();

    constexpr auto mindB = -60.0f;
    constexpr auto maxdB = 6.0f;

    // the cells fill everything under the header
    const auto cellArea = juce::Rectangle<float>(0.0f, (float) headerHeight(scopeRect),
                                                 w, h - (float) headerHeight(scopeRect));

    if (cellArea.getWidth() <= 0.0f || cellArea.getHeight() <= 0.0f)
        return;

    // a stacked band shares the cell of the band before it
    int cellCount = 0;

    for (int band = 0; band < numBands; ++band)
        if (!bands[band].stacked || band == 0)
            ++cellCount;

    const auto cellWidth = cellArea.getWidth() / (float) cellCount;

    auto toY = [&](float db) {
        return juce::jmap(juce::jlimit(mindB, maxdB, db), mindB, maxdB, cellArea.getBottom(), cellArea.getY());
    };

    g.setFont(getLookAndFeel().getPopupMenuFont());
    g.setFont(scope_constants::readoutFontHeight);

    auto cell = juce::Rectangle<float>(cellArea.getX(), cellArea.getY(), cellWidth, cellArea.getHeight());
    int cellIndex = -1;

    for (int band = 0; band < numBands; ++band)
    {
        const auto thresholdDb = bands[band].thresholdDb;
        const auto thresholdY = toY(thresholdDb);
        const auto stacked = bands[band].stacked && band > 0;
        const auto hasStacked = band + 1 < numBands && bands[band + 1].stacked;

        if (!stacked)
        {
            ++cellIndex;
            cell = juce::Rectangle<float>(cellArea.getX() + (float) cellIndex * cellWidth, cellArea.getY(),
                                          cellWidth, cellArea.getHeight());
            
            g.setColour(theme().compCell);
            g.fillRect(cell);

            g.setColour(theme().compOverThreshold);
            g.fillRect(cell.withBottom(thresholdY));
        }

        const auto half = cell.getWidth() * 0.5f;
        const auto region = stacked ? cell.withTrimmedLeft(half)
                                    : (hasStacked ? cell.withWidth(half) : cell);

        if (stacked)
        {
            g.setColour(theme().compStackedOverThreshold);
            g.fillRect(region.withBottom(thresholdY));

            g.setColour(theme().scopeGrid);
            g.drawLine(region.getX(), region.getY(), region.getX(), region.getBottom(), 1.0f);
        }

        constexpr auto minKneeHeight = 3.0f;

        const auto kneeTop = toY(thresholdDb + Compressor::standardKneeDb * 0.5f);
        const auto kneeBottom = toY(thresholdDb - Compressor::standardKneeDb * 0.5f);

        g.setColour(theme().compKnee);
        g.fillRect(juce::Rectangle<float>(region.getX(), kneeTop, region.getWidth(), kneeBottom - kneeTop)
                       .withSizeKeepingCentre(region.getWidth(), juce::jmax(minKneeHeight, kneeBottom - kneeTop)));

        // a fixed array of lines above the threshold representing the knee. these are in DB before ratio is applied
        constexpr std::array<float, 4> overshoots { 6.0f, 12.0f, 18.0f, 24.0f };

        for (size_t rung = 0; rung < overshoots.size(); ++rung)
        {
            const auto outDb = thresholdDb + overshoots[rung] / ratio;

            // rungs that the ratio has not pulled into view yet just stay off the top
            if (outDb > maxdB)
                continue;

            const auto rungY = toY(outDb);

            g.setColour(theme().scopeGrid.withMultipliedAlpha(1.0f - 0.18f * (float) rung));
            g.drawLine(region.getX() + 1.0f, rungY, region.getRight() - 1.0f, rungY, 1.0f);
        }

        const auto levelDb = juce::Decibels::gainToDecibels(bands[band].level, mindB);

        if (levelDb > mindB)
        {
            g.setColour(levelDb > thresholdDb ? theme().compLevelOver : theme().compLevel);
            g.fillRect(region.withTop(toY(levelDb)).reduced(region.getWidth() * 0.28f, 0.0f));
        }

        constexpr auto lineInset = 6.0f;

        g.setColour(theme().compThreshold);
        g.drawLine(region.getX() + lineInset, thresholdY, region.getRight() - lineInset, thresholdY, 1.5f);

        constexpr auto readoutHeight = 12.0f;
        const auto readoutAbove = thresholdY - cell.getY() >= readoutHeight + 2.0f;
        const auto readout = juce::Rectangle<float>(region.getX(),
                                                    readoutAbove ? cell.getY() + 1.0f : thresholdY + 1.0f,
                                                    region.getWidth(), readoutHeight);

        g.setColour(theme().textDim);

        if (!stacked || ! juce::approximatelyEqual(thresholdDb, bands[band - 1].thresholdDb))
            g.drawText(juce::String(juce::roundToInt(thresholdDb)) + " dB", readout, juce::Justification::centred, false);

        g.drawText(bands[band].name,
                   juce::Rectangle<float>(region.getX(), cell.getBottom() - 13.0f, region.getWidth(), 12.0f),
                   juce::Justification::centred, false);

        if (! juce::approximatelyEqual(bands[band].gainOffsetDb, 0.0f))
        {
            const auto offset = bands[band].gainOffsetDb;

            g.setColour(theme().textDim);
            g.drawText(juce::String(offset > 0.0f ? "+" : "") + juce::String(offset, 1),
                       juce::Rectangle<float>(region.getX(), cell.getBottom() - 25.0f, region.getWidth(), 12.0f),
                       juce::Justification::centred, false);
            g.setColour(theme().textDim);
        }
    }

    g.setColour(theme().scopeGrid);

    for (int edge = 1; edge < cellCount; ++edge)
    {
        const auto x = cellArea.getX() + (float) edge * cellWidth;
        g.drawLine(x, cellArea.getY(), x, cellArea.getBottom(), 1.0f);
    }
}

void Scope::drawMBComp(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto thr = paramValue(ParamIDs::MBCompThreshold);
    const auto tilt = paramValue(ParamIDs::compBandTilt);

    const std::array<CompBand, 3> bands {
        CompBand { thr - tilt, bandLevel(dataCollector.band1), "LOW" },
        CompBand { thr,        bandLevel(dataCollector.band2), "MID" },
        CompBand { thr + tilt, bandLevel(dataCollector.band3), "HIGH" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), compRatio());
}

void Scope::drawMSComp(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto thr = paramValue(ParamIDs::MSCompThreshold);
    const auto tilt = paramValue(ParamIDs::compBandTilt);

    const std::array<CompBand, 2> bands {
        CompBand { thr - tilt, bandLevel(dataCollector.band1), "MID" },
        CompBand { thr + tilt, bandLevel(dataCollector.band2), "SIDE" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), compRatio());
}

void Scope::drawTypeAComp(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto thr = paramValue(ParamIDs::TypeAThreshold);
    const auto tilt = paramValue(ParamIDs::TypeATilt);

    const std::array<CompBand, 4> bands {
        CompBand { thr, bandLevel(dataCollector.band1), "LOW",  false, -tilt },
        CompBand { thr, bandLevel(dataCollector.band2), "MID",  false, 0.0f },
        CompBand { thr, bandLevel(dataCollector.band3), "HIGH", false, tilt * 0.5f },
        CompBand { thr, bandLevel(dataCollector.band4), "X-HI", true,  tilt * 0.5f }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), paramValue(ParamIDs::TypeARatio));
}

// one cell per band, with a bar up from the middle for a boost or down for a cut
void Scope::drawTransient(juce::Graphics &g, juce::Rectangle<float> scopeRect, const juce::StringArray &bandNames)
{
    constexpr auto rangeDb = 24.0f;
    constexpr auto easing = 0.85f;
    const auto numBands = bandNames.size();

    const auto cellArea = juce::Rectangle<float>(0.0f, (float) headerHeight(scopeRect),
                                                 (float) scopeRect.getWidth(), (float) scopeRect.getHeight() - (float) headerHeight(scopeRect));

    if (cellArea.isEmpty())
        return;

    const auto cellWidth = cellArea.getWidth() / (float) numBands;

    g.setFont(getLookAndFeel().getPopupMenuFont());
    g.setFont(scope_constants::readoutFontHeight);

    for (int band = 0; band < numBands; ++band)
    {
        const auto db = dataCollector.transientGainDb[(size_t) band].exchange(0.0f, std::memory_order_relaxed);
        auto& shown = shownTransientDb[(size_t) band];
        shown = std::abs(db) > std::abs(shown) ? db : shown * easing;

        const auto cell = cellArea.withX(cellArea.getX() + cellWidth * (float) band).withWidth(cellWidth);
        const auto middle = cell.getCentreY();
        const auto y = juce::jmap(juce::jlimit(-rangeDb, rangeDb, shown), -rangeDb, rangeDb, cell.getBottom(), cell.getY());

        g.setColour(theme().compCell);
        g.fillRect(cell);

        g.setColour(shown > 0.0f ? theme().compLevelOver : theme().compLevel);
        g.fillRect(juce::Rectangle<float>(cell.getX(), juce::jmin(y, middle), cell.getWidth(), std::abs(y - middle))
                       .reduced(cell.getWidth() * 0.28f, 0.0f));

        constexpr auto lineInset = 6.0f;

        g.setColour(theme().compThreshold);
        g.drawLine(cell.getX() + lineInset, middle, cell.getRight() - lineInset, middle, 1.5f);

        g.setColour(theme().textDim);
        g.drawText(juce::String(shown, 1) + " dB", cell.withHeight(12.0f).translated(0.0f, 1.0f), juce::Justification::centred, false);

        g.drawText(bandNames[band], cell.withTop(cell.getBottom() - 13.0f).withHeight(12.0f), juce::Justification::centred, false);

        if (band > 0)
        {
            g.setColour(theme().scopeGrid);
            g.drawLine(cell.getX(), cell.getY(), cell.getX(), cell.getBottom(), 1.0f);
        }
    }
}

// each channel's cell, or each band's at its tilted threshold, the way the other compressors show theirs
void Scope::drawOptoComp(juce::Graphics &g, juce::Rectangle<float> scopeRect, bool multiband)
{
    const auto thr = paramValue(ParamIDs::optoThreshold);
    const auto ratio = paramValue(ParamIDs::optoRatio);

    if (multiband)
    {
        const auto tilt = paramValue(ParamIDs::compBandTilt);
        const std::array<CompBand, 3> bands {
            CompBand { thr - tilt, bandLevel(dataCollector.band1), "LOW" },
            CompBand { thr,        bandLevel(dataCollector.band2), "MID" },
            CompBand { thr + tilt, bandLevel(dataCollector.band3), "HIGH" }
        };

        drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), ratio);
        drawParamHeader(g, scopeRect, { formatDecibels(thr), juce::String(ratio, 1) + ":1", formatDecibels(tilt) });
        return;
    }

    const std::array<CompBand, 2> bands {
        CompBand { thr, bandLevel(dataCollector.band1), "LEFT" },
        CompBand { thr, bandLevel(dataCollector.band2), "RIGHT" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), ratio);
    drawParamHeader(g, scopeRect, { formatDecibels(thr), juce::String(ratio, 1) + ":1",
                                    formatPercent(paramValue(ParamIDs::compStereoLink) * 0.01f) });
}

void Scope::drawStereoComp(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto thr = paramValue(ParamIDs::stereoCompThreshold);

    const std::array<CompBand, 2> bands {
        CompBand { thr, bandLevel(dataCollector.band1), "LEFT" },
        CompBand { thr, bandLevel(dataCollector.band2), "RIGHT" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), compRatio());
}

float Scope::compRatio() const
{
    return juce::jmax(1.0f, paramValue(ParamIDs::compRatio));
}

float Scope::bandLevel(LevelMeter &meter) const
{
    return meter.getNext();
}

// only the threshold differs between the three compressor views, and the stereo one has no tilt
juce::StringArray Scope::getCompHeaderLabels(const ParamIDs::ParameterInfo &thresholdParam, bool withTilt) const
{
    juce::StringArray labels { formatDecibels(paramValue(thresholdParam)),
                               juce::String(compRatio(), 1) + ":1" };

    if (withTilt)
        labels.add(formatDecibels(paramValue(ParamIDs::compBandTilt)));

    return labels;
}
