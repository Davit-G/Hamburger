#include "Scope.h"

void Scope::drawLRView(juce::Graphics &g, juce::Rectangle<int> area)
{
    const auto scopeRect = area.toFloat();

    drawLRScope(g, scopeRect.withTrimmedTop(headerHeight(scopeRect)));
    drawParamHeader(g, scopeRect, { formatDecibels(paramValue(ParamIDs::inputGain)),
                                    formatPercent(paramValue(ParamIDs::mix) * 0.01f),
                                    formatDecibels(paramValue(ParamIDs::outputGain)) });
}

// picks where the drawn window starts, so audio cycles land in the same place on the scope
// uses an implementtion of schmitt triggering
// - it fires on the way up, arms below the midpoint
// - levels are derived from min max values of all the samples in our window rather than from a fixed value
// - it looks at a low passed copy which reduces noise / jitter when scanning
// - once it detects a rising edge, it picks the trigger offset that has the least sum of squared difference in our samples to our previous capture
void Scope::updateTriggerOffset()
{
    auto searchEnd = sampleDataL.size() - hopSize;

    constexpr auto cutoffHz = 800.0;
    
    auto sampleRate = juce::jmax(8000.0, dataCollector.getSampleRate());
    auto coeff = (float) (1.0 - std::exp(-2.0 * juce::MathConstants<double>::pi * cutoffHz / sampleRate));

    auto smoothed = sampleDataL[0];
    auto rawLo = sampleDataL[0];
    auto rawHi = sampleDataL[0];

    auto lowLo = sampleDataL[0];
    auto lowHi = sampleDataL[0];
    auto blockSum = 0.0f;
    size_t decimatedIndex = 0;

    // filter and decimate audio
    for (size_t i = 0; i < sampleDataL.size(); ++i)
    {
        const auto sample = sampleDataL[i];

        smoothed += coeff * (sample - smoothed);
        triggerSignal[i] = smoothed;
        blockSum += smoothed;

        rawLo = std::min(rawLo, sample);
        rawHi = std::max(rawHi, sample);

        if ((i + 1) % scope_constants::triggerDecimation != 0 || decimatedIndex >= triggerDecimated.size())
            continue;

        const auto blockMean = blockSum / (float) scope_constants::triggerDecimation;

        triggerDecimated[decimatedIndex++] = blockMean;
        blockSum = 0.0f;

        lowLo = std::min(lowLo, blockMean);
        lowHi = std::max(lowHi, blockMean);
    }

    // midpoint and half range, so trigger levels sit relative to the signal rather than to zero
    const auto rawMid      = (rawHi + rawLo) * 0.5f;
    const auto rawAmp      = (rawHi - rawLo) * 0.5f;
    const auto lowMid      = (lowHi + lowLo) * 0.5f;
    const auto lowAmp      = (lowHi - lowLo) * 0.5f;

    // ignore detection (and preserve current trigger offset)
    if (juce::jmax(rawAmp, lowAmp) < float(0.0005))
        return;

    // all the content sits above the cutoff, so the filtered copy has nothing left to lock onto
    if (lowAmp < rawAmp * float(0.25))
    {
        // somehow the lowpassed audio is so quiet / we don't have a lot of lowpassed info. it barely makes a dent compared to the raw waveform.
        // take the first clean rising edge
        auto armLevel = rawMid - rawAmp * float(0.2);
        auto fireLevel = rawMid + rawAmp * float(0.2);
        
        // arming in this case means that 
        bool armed = false;

        for (size_t i = 1; i < searchEnd; ++i)
        {
            if (sampleDataL[i] <= armLevel)
                armed = true;
            else if (armed && sampleDataL[i - 1] < fireLevel && sampleDataL[i] >= fireLevel)
            {
                triggerOffset = i;
                return;
            }
        }

        return;
    }

    auto armLevel = lowMid - lowAmp * float(0.2);
    auto fireLevel = lowMid + lowAmp * float(0.2);
    
    auto searchEndDecimated = searchEnd / scope_constants::triggerDecimation;

    size_t best = 0;
    float bestError = 0;
    bool found = false;
    bool armed = false;

    for (size_t i = 1; i < searchEndDecimated; ++i)
    {
        if (triggerDecimated[i] <= armLevel)
        {
            armed = true;
            continue;
        }

        // ignore trigger
        if (!armed || triggerDecimated[i - 1] >= fireLevel || triggerDecimated[i] < fireLevel)
            continue;

        armed = false; // one candidate per cycle, the next needs to dip and come back up

        // if we havent made a detection before let's just pick this one since we need a reference
        if (!hasTriggerReference)
        {
            best = i;
            found = true;
            break;
        }

        float error = 0;

        // sum of squared differences
        for (size_t k = 0; k < scope_constants::triggerMatchLength; ++k)
        {
            const auto difference = triggerDecimated[i + k] - triggerReference[k];
            error += difference * difference;
        }

        if (!found || error < bestError)
        {
            bestError = error;
            best = i;
            found = true;
        }
    }

    if (!found)
        return;

    auto lockedShape = triggerDecimated.begin() + (std::ptrdiff_t) best;
    std::copy(lockedShape, lockedShape + (std::ptrdiff_t) scope_constants::triggerMatchLength, triggerReference.begin());
    hasTriggerReference = true;

    // we have to check the exact spot in our real data to continue from
    // because we only did detection on our decimated dataset
    auto coarse = best * scope_constants::triggerDecimation;
    auto from = coarse > scope_constants::triggerDecimation ? coarse - scope_constants::triggerDecimation : size_t(1);
    auto to = std::min(coarse + scope_constants::triggerDecimation, searchEnd);

    triggerOffset = coarse;

    for (size_t i = from; i < to; ++i)
    {
        if (triggerSignal[i - 1] < fireLevel && triggerSignal[i] >= fireLevel)
        {
            triggerOffset = i;
            break;
        }
    }
}

void Scope::drawLRScope(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto h = scopeRect.getHeight();
    const auto hop = hopSize;

    g.setColour(theme().scopeGrid);
    plotStraightLine(originLineData.data(), 2, g, scopeRect, 0.5f, h / 2);
    plotStraightLine(originLineData.data(), 2, g, scopeRect, float(-0.5), h / 2);

    // trigger offset is position where trigger was detected
    g.setColour(theme().scopeLeft);
    plotStraightLine(sampleDataL.data() + triggerOffset, hop, g, scopeRect, 0.5f, h / 2);
    g.setColour(theme().scopeRight);
    plotStraightLine(sampleDataR.data() + triggerOffset, hop, g, scopeRect, 0.5f, h / 2);
}

void Scope::plotStraightLine(const float *data,
                 size_t numSamples,
                 juce::Graphics &g,
                 juce::Rectangle<float> rect,
                 float scaler,
                 float offset)
{
    auto w = rect.getWidth();
    auto h = rect.getHeight();
    auto right = rect.getRight();

    auto center = rect.getBottom() - offset;
    auto gain = h * scaler;

    for (size_t i = 1; i < numSamples; ++i)
        g.drawLine({juce::jmap(float(i - 1), 0.0f, float(numSamples - 1), float(right - w), float(right)),
                    center - gain * data[i - 1],
                    juce::jmap(float(i), 0.0f, float(numSamples - 1), float(right - w), float(right)),
                    center - gain * data[i]});
}
