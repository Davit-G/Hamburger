#include "Scope.h"
#include "../../dsp/Dynamics/Compressor.h"
#include "../../dsp/Dynamics/TypeA.h"

// whichever compressor type is running. no watermark, the cells are opaque and cover everything under the header
void Scope::drawCompressorView(juce::Graphics &g, juce::Rectangle<int> area, SlotId slot)
{
    const auto scopeRect = area.toFloat();

    switch (compressionType->getIndex()) {
        case 0:
            drawStereoComp(g, scopeRect, slot);
            drawParamHeader(g, scopeRect, getCompHeaderLabels(ParamIDs::stereoCompThreshold, false, slot));
            break;
        case 1:
            drawMBComp(g, scopeRect, slot);
            drawParamHeader(g, scopeRect, getCompHeaderLabels(ParamIDs::MBCompThreshold, true, slot));
            break;
        case 2:
            drawMSComp(g, scopeRect, slot);
            drawParamHeader(g, scopeRect, getCompHeaderLabels(ParamIDs::MSCompThreshold, true, slot));
            break;
        case 3:
            drawTypeAComp(g, scopeRect, slot);
            // the ratio is fixed in the dsp, and tilt only moves the makeup gains
            drawParamHeader(g, scopeRect, { formatDecibels(getValue(ParamIDs::TypeAThreshold, slot)),
                                            juce::String(TypeAProcessor::baseRatio, 1) + ":1",
                                            formatDecibels(getValue(ParamIDs::TypeATilt, slot)) });
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
            
            g.setColour(juce::Colours::black);
            g.fillRect(cell);

            g.setColour(juce::Colour::fromRGB(22, 22, 22));
            g.fillRect(cell.withBottom(thresholdY));
        }

        const auto half = cell.getWidth() * 0.5f;
        const auto region = stacked ? cell.withTrimmedLeft(half)
                                    : (hasStacked ? cell.withWidth(half) : cell);

        if (stacked)
        {
            g.setColour(juce::Colour::fromRGB(30, 30, 30));
            g.fillRect(region.withBottom(thresholdY));

            g.setColour(juce::Colour::fromRGB(48, 48, 48));
            g.drawLine(region.getX(), region.getY(), region.getX(), region.getBottom(), 1.0f);
        }

        constexpr auto minKneeHeight = 3.0f;

        const auto kneeTop = toY(thresholdDb + Compressor::standardKneeDb * 0.5f);
        const auto kneeBottom = toY(thresholdDb - Compressor::standardKneeDb * 0.5f);

        g.setColour(juce::Colour::fromRGB(38, 38, 38));
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

            g.setColour(juce::Colours::grey.withAlpha(0.5f - 0.09f * (float) rung));
            g.drawLine(region.getX() + 1.0f, rungY, region.getRight() - 1.0f, rungY, 1.0f);
        }

        const auto levelDb = juce::Decibels::gainToDecibels(bands[band].level, mindB);

        if (levelDb > mindB)
        {
            g.setColour(levelDb > thresholdDb ? juce::Colours::orange : juce::Colours::yellow);
            g.fillRect(region.withTop(toY(levelDb)).reduced(region.getWidth() * 0.28f, 0.0f));
        }

        g.setColour(juce::Colours::grey);
        g.drawLine(region.getX(), thresholdY, region.getRight(), thresholdY, 1.5f);

        constexpr auto readoutHeight = 12.0f;
        const auto readoutAbove = thresholdY - cell.getY() >= readoutHeight + 2.0f;
        const auto readout = juce::Rectangle<float>(region.getX(),
                                                    readoutAbove ? cell.getY() + 1.0f : thresholdY + 1.0f,
                                                    region.getWidth(), readoutHeight);

        g.setColour(juce::Colours::darkgrey);

        if (!stacked || ! juce::approximatelyEqual(thresholdDb, bands[band - 1].thresholdDb))
            g.drawText(juce::String(juce::roundToInt(thresholdDb)) + " dB", readout, juce::Justification::centred, false);

        g.drawText(bands[band].name,
                   juce::Rectangle<float>(region.getX(), cell.getBottom() - 13.0f, region.getWidth(), 12.0f),
                   juce::Justification::centred, false);

        if (! juce::approximatelyEqual(bands[band].gainOffsetDb, 0.0f))
        {
            const auto offset = bands[band].gainOffsetDb;

            g.setColour(juce::Colour::fromRGB(90, 90, 90));
            g.drawText(juce::String(offset > 0.0f ? "+" : "") + juce::String(offset, 1),
                       juce::Rectangle<float>(region.getX(), cell.getBottom() - 25.0f, region.getWidth(), 12.0f),
                       juce::Justification::centred, false);
            g.setColour(juce::Colours::darkgrey);
        }
    }

    g.setColour(juce::Colour::fromRGB(64, 64, 64));

    for (int edge = 1; edge < cellCount; ++edge)
    {
        const auto x = cellArea.getX() + (float) edge * cellWidth;
        g.drawLine(x, cellArea.getY(), x, cellArea.getBottom(), 1.0f);
    }
}

void Scope::drawStereoComp(juce::Graphics &g, juce::Rectangle<float> scopeRect, SlotId slot)
{
    const auto thr = getValue(ParamIDs::stereoCompThreshold, slot);

    const std::array<CompBand, 2> bands {
        CompBand { thr, bandLevel(dataCollector.band1), "LEFT" },
        CompBand { thr, bandLevel(dataCollector.band2), "RIGHT" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), compRatio(slot));
}

void Scope::drawMBComp(juce::Graphics &g, juce::Rectangle<float> scopeRect, SlotId slot)
{
    const auto thr = getValue(ParamIDs::MBCompThreshold, slot);
    const auto tilt = getValue(ParamIDs::compBandTilt, slot);

    const std::array<CompBand, 3> bands {
        CompBand { thr - tilt, bandLevel(dataCollector.band1), "LOW" },
        CompBand { thr,        bandLevel(dataCollector.band2), "MID" },
        CompBand { thr + tilt, bandLevel(dataCollector.band3), "HIGH" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), compRatio(slot));
}

void Scope::drawMSComp(juce::Graphics &g, juce::Rectangle<float> scopeRect, SlotId slot)
{
    const auto thr = getValue(ParamIDs::MSCompThreshold, slot);
    const auto tilt = getValue(ParamIDs::compBandTilt, slot);

    const std::array<CompBand, 2> bands {
        CompBand { thr - tilt, bandLevel(dataCollector.band1), "MID" },
        CompBand { thr + tilt, bandLevel(dataCollector.band2), "SIDE" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), compRatio(slot));
}

void Scope::drawTypeAComp(juce::Graphics &g, juce::Rectangle<float> scopeRect, SlotId slot)
{
    const auto thr = getValue(ParamIDs::TypeAThreshold, slot);
    const auto tilt = getValue(ParamIDs::TypeATilt, slot);

    const std::array<CompBand, 4> bands {
        CompBand { thr, bandLevel(dataCollector.band1), "LOW",  false, -tilt },
        CompBand { thr, bandLevel(dataCollector.band2), "MID",  false, 0.0f },
        CompBand { thr, bandLevel(dataCollector.band3), "HIGH", false, tilt * 0.5f },
        CompBand { thr, bandLevel(dataCollector.band4), "X-HI", true,  tilt * 0.5f }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), TypeAProcessor::baseRatio);
}

float Scope::compRatio(SlotId slot) const
{
    return juce::jmax(1.0f, getValue(ParamIDs::compRatio, slot));
}

float Scope::bandLevel(LevelMeter &meter) const
{
    return meter.getNext();
}

// only the threshold differs between the three compressor views, and the stereo one has no tilt
juce::StringArray Scope::getCompHeaderLabels(const ParamIDs::ParameterInfo &thresholdParam, bool withTilt, SlotId slot) const
{
    juce::StringArray labels { formatDecibels(getValue(thresholdParam, slot)),
                               juce::String(compRatio(slot), 1) + ":1" };

    if (withTilt)
        labels.add(formatDecibels(getValue(ParamIDs::compBandTilt, slot)));

    return labels;
}
