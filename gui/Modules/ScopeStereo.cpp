#include "Scope.h"

// the middle of a span of samples and how far either side of it they reach
struct SignalRange
{
    float mid;
    float amplitude;
};

static SignalRange midAndAmplitude(float lo, float hi)
{
    return { (hi + lo) * 0.5f, (hi - lo) * 0.5f };
}

// the stereo waveform, under the plugin's own in, mix and out
void Scope::drawLRView(juce::Graphics &g, juce::Rectangle<int> area)
{
    const auto scopeRect = area.toFloat();

    drawLRScope(g, scopeRect.withTrimmedTop(headerHeight(scopeRect)));
    drawParamHeader(g, scopeRect, { formatDecibels(getValue(ParamIDs::inputGain)),
                                    formatPercent(getValue(ParamIDs::mix) * 0.01f),
                                    formatDecibels(getValue(ParamIDs::outputGain)) });
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

    const auto raw = midAndAmplitude(rawLo, rawHi);
    const auto lowPassed = midAndAmplitude(lowLo, lowHi);

    // ignore detection (and preserve current trigger offset)
    if (juce::jmax(raw.amplitude, lowPassed.amplitude) < 0.0005f)
        return;

    // all the content sits above the cutoff, so the filtered copy has nothing left to lock onto
    if (lowPassed.amplitude < raw.amplitude * 0.25f)
    {
        // somehow the lowpassed audio is so quiet / we don't have a lot of lowpassed info. it barely makes a dent compared to the raw waveform.
        // take the first clean rising edge
        auto armLevel = raw.mid - raw.amplitude * 0.2f;
        auto fireLevel = raw.mid + raw.amplitude * 0.2f;
        
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

    auto armLevel = lowPassed.mid - lowPassed.amplitude * 0.2f;
    auto fireLevel = lowPassed.mid + lowPassed.amplitude * 0.2f;
    
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

    g.setColour(juce::Colours::grey);
    plotStraightLine(originLineData.data(), 2, g, scopeRect, 0.5f, h / 2);
    plotStraightLine(originLineData.data(), 2, g, scopeRect, -0.5f, h / 2);

    // trigger offset is position where trigger was detected
    g.setColour(juce::Colours::yellow);
    plotStraightLine(sampleDataL.data() + triggerOffset, hop, g, scopeRect, 0.5f, h / 2);
    g.setColour(juce::Colours::lime);
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
