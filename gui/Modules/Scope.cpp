#include "Scope.h"

#include "../../dsp/WaveShapers.h"
#include "../../dsp/Dynamics/Compressor.h"
#include "../../dsp/Dynamics/TypeA.h"


static constexpr size_t triggerDecimation = 8; // we do detection on a decimated audio stream so its way cheaper
static constexpr size_t triggerMatchLength = 32; // how much of the waveform is matched against the previous frame
static constexpr juce::uint8 contextLabelAlpha = 20; // brightness of background watermark on scope
static constexpr float readoutFontHeight = 11.0f; // font height for info / stats on scope

// midpoint and half range of a block, so trigger levels sit relative to the signal rather than to zero
template <typename SampleType>
struct SignalRange
{
    SampleType mid;
    SampleType amplitude;
};

template <typename SampleType>
static SignalRange<SampleType> midAndAmplitude(SampleType lo, SampleType hi)
{
    return { (hi + lo) * SampleType(0.5), (hi - lo) * SampleType(0.5) };
}

static juce::String formatFrequency(float freq)
{
    if (freq >= 1000.0f)
        return juce::String(freq / 1000.0f, 1) + " kHz";

    return juce::String(juce::roundToInt(freq)) + " Hz";
}

static juce::String formatDecibels(float db)
{
    return juce::String(db > 0.0f ? "+" : "") + juce::String(db, 1) + " dB";
}

static juce::String formatPercent(float normalised)
{
    return juce::String(juce::roundToInt(normalised * 100.0f)) + "%";
}


template <typename SampleType>
Scope<SampleType>::Scope(juce::AudioProcessorValueTreeState& valueTree, ScopeDataCollector<SampleType>& scopeDataCollector, ScopeContext &newScopeContext)
    : apvts(valueTree),
    dataCollector(scopeDataCollector),
      scopeContext(newScopeContext),
      noiseDist(valueTree)
{
    int fps = scope_constants::defaultFrameRate;
    sampleDataL.resize(scope_constants::defaultHopSize * 3, SampleType(0));
    sampleDataR.resize(scope_constants::defaultHopSize * 3, SampleType(0));
    triggerSignal.resize(sampleDataL.size(), SampleType(0));
    triggerDecimated.resize(sampleDataL.size() / triggerDecimation, SampleType(0));
    triggerReference.resize(triggerMatchLength, SampleType(0));
    sampleDataPreDistortion.resize(scope_constants::defaultHopSize);
    sampleDataPostDistortion.resize(scope_constants::defaultHopSize);
    setFramesPerSecond(fps);

    bufferedFFTInput.resize(scope_constants::fftSize);
    fftHistory.resize(scope_constants::fftSize, SampleType(0));

    lowFreqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisLowFreq.getParamID()));
    highFreqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisHighFreq.getParamID()));
    lowGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisLowGain.getParamID()));
    highGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisHighGain.getParamID()));
    postClipKneeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::postClipKnee.getParamID()));

    compressionType = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(ParamIDs::compressionType.getParamID()));

    juce::dsp::ProcessSpec spec; // this is not realtime so whatever
    spec.maximumBlockSize = 128;
    spec.numChannels = 2;
    spec.sampleRate = 44100;

    noiseDist.prepare(spec);
    noiseDistBuf.setSize(2, 512, false, true, false);
}

template <typename SampleType>
void Scope<SampleType>::resized() {
    if (getWidth() <= 0 || getHeight() <= 0) {
        inOutFB = juce::Image();
        return;
    }

    const auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds());
    inOutRenderScale = juce::jlimit(1.0f, 2.0f, display != nullptr ? (float) display->scale : 1.0f);

    inOutFB = juce::Image(juce::Image::RGB,
                          juce::roundToInt(getWidth() * inOutRenderScale),
                          juce::roundToInt(getHeight() * inOutRenderScale),
                          false);

    juce::Graphics imgG(inOutFB);
    imgG.fillAll(inOutBackground);
}

template <typename SampleType>
void Scope<SampleType>::mouseDown(const juce::MouseEvent &event)
{
    juce::ignoreUnused(event);
    this->viewSpectrum = !this->viewSpectrum;
}


template <typename SampleType>
void Scope<SampleType>::setFramesPerSecond(int framesPerSecond)
{
    jassert(framesPerSecond > 0 && framesPerSecond < 1000);
    startTimerHz(framesPerSecond);
}


template <typename SampleType>
void Scope<SampleType>::paint(juce::Graphics &g)
{
    auto area = getLocalBounds();
    auto h = (SampleType)area.getHeight();
    auto w = (SampleType)area.getWidth();

    auto scopeRect = juce::Rectangle<SampleType>{SampleType(0), SampleType(0), w, h};

    auto currentType = scopeContext.getType();

    auto compressionTypeValue = compressionType->getIndex();
    
    switch (currentType) {
        case ScopeContextType::LR_SCOPE:
            drawLRScope(g, scopeRect.withTrimmedTop(headerHeight(scopeRect)));
            drawParamHeader(g, scopeRect, { formatDecibels(paramValue(ParamIDs::inputGain)),
                                            formatPercent(paramValue(ParamIDs::mix) * 0.01f),
                                            formatDecibels(paramValue(ParamIDs::outputGain)) });
        break;
        case ScopeContextType::IN_OUT:
            if (inOutFB.isValid())
                g.drawImage(inOutFB, area.toFloat());

            drawTiledContextLabel(g, area, "DISTORTION");
            drawParamHeader(g, scopeRect, getDistortionHeaderLabels());
            drawDistortionAmount(g, scopeRect);
        
            // drawInOutAxes(g, scopeRect);
            break;
        case ScopeContextType::SPECTRUM_EMPHASIS:
            drawTiledContextLabel(g, area, "EMPHASIS");
            drawSpectrumEmphasis(g, scopeRect);
            break;
        case ScopeContextType::CLIPPER:
            drawTiledContextLabel(g, area, "CLIPPER");
            drawClipper(g, scopeRect);
            drawParamHeader(g, scopeRect, { formatDecibels(paramValue(ParamIDs::postClipGain)),
                                            juce::String(paramValue(ParamIDs::postClipKnee), 1) + " dB" });
            break;
        // no watermark on these three - the cells are opaque and cover everything under the header
        case ScopeContextType::COMPRESSION: {
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
                    // the ratio is fixed in the dsp, and tilt only moves the makeup gains
                    drawParamHeader(g, scopeRect, { formatDecibels(paramValue(ParamIDs::TypeAThreshold)),
                                                    juce::String(TypeAProcessor::baseRatio, 1) + ":1",
                                                    formatDecibels(paramValue(ParamIDs::TypeATilt)) });
                    break;
                default:
                    break;
            }
            break;
        }
                
        case ScopeContextType::NOISE: {
            drawTiledContextLabel(g, area, "NOISE");

            for (int i = 0; i < noiseDistBuf.getNumSamples(); i++) {
                noiseDistBuf.setSample(0, i, 0.7f * sin(6.28f * (i - 32.0f) * 100.f / 44100.f));
            }
            auto block = juce::dsp::AudioBlock<float>(noiseDistBuf);
            noiseDist.processBlock(block);

            const auto noiseRect = scopeRect.withTrimmedTop(headerHeight(scopeRect));

            // draw line halfway
            g.setColour(juce::Colours::darkgrey);
            g.drawLine(SampleType(0), noiseRect.getCentreY(), w, noiseRect.getCentreY());

            // draw wave
            g.setColour(juce::Colours::yellow);
            plotStraightLine(noiseDistBuf.getReadPointer(0) + 32, noiseDistBuf.getNumSamples() - 32, g, noiseRect, SampleType(0.5), noiseRect.getHeight() / 2);

            drawParamHeader(g, scopeRect, getNoiseHeaderLabels());
            break;
        }
        default:
            break;
    }
}

template <typename SampleType>
void Scope<SampleType>::updateHopSize()
{
    const auto sampleRate = dataCollector.getSampleRate();
    
    if (sampleRate <= 0.0 || juce::approximatelyEqual(sampleRate, preparedSampleRate))
    return;
    
    preparedSampleRate = sampleRate;
    hopSize = (size_t) juce::jmax(64, juce::roundToInt(sampleRate / (double) scope_constants::defaultFrameRate));
    
    sampleDataL.assign(hopSize * 3, SampleType(0));
    sampleDataR.assign(hopSize * 3, SampleType(0));
    triggerSignal.assign(sampleDataL.size(), SampleType(0));
    triggerDecimated.assign(sampleDataL.size() / triggerDecimation, SampleType(0));
    sampleDataPreDistortion.assign(hopSize, SampleType(0));
    sampleDataPostDistortion.assign(hopSize, SampleType(0));
    
    triggerOffset = 0;
    hasTriggerReference = false;
}

// picks where the drawn window starts, so audio cycles land in the same place on the scope
// uses an implementtion of schmitt triggering
// - it fires on the way up, arms below the midpoint
// - levels are derived from min max values of all the samples in our window rather than from a fixed value
// - it looks at a low passed copy which reduces noise / jitter when scanning
// - once it detects a rising edge, it picks the trigger offset that has the least sum of squared difference in our samples to our previous capture
template <typename SampleType>
void Scope<SampleType>::updateTriggerOffset()
{
    auto searchEnd = sampleDataL.size() - hopSize;

    constexpr auto cutoffHz = 800.0;
    
    auto sampleRate = juce::jmax(8000.0, dataCollector.getSampleRate());
    auto coeff = (SampleType) (1.0 - std::exp(-2.0 * juce::MathConstants<double>::pi * cutoffHz / sampleRate));

    auto smoothed = sampleDataL[0];
    auto rawLo = sampleDataL[0];
    auto rawHi = sampleDataL[0];

    auto lowLo = sampleDataL[0];
    auto lowHi = sampleDataL[0];
    auto blockSum = SampleType(0);
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

        if ((i + 1) % triggerDecimation != 0 || decimatedIndex >= triggerDecimated.size())
            continue;

        const auto blockMean = blockSum / (SampleType) triggerDecimation;

        triggerDecimated[decimatedIndex++] = blockMean;
        blockSum = SampleType(0);

        lowLo = std::min(lowLo, blockMean);
        lowHi = std::max(lowHi, blockMean);
    }

    const auto raw = midAndAmplitude(rawLo, rawHi);
    const auto lowPassed = midAndAmplitude(lowLo, lowHi);

    // ignore detection (and preserve current trigger offset)
    if (juce::jmax(raw.amplitude, lowPassed.amplitude) < SampleType(0.0005))
        return;

    // all the content sits above the cutoff, so the filtered copy has nothing left to lock onto
    if (lowPassed.amplitude < raw.amplitude * SampleType(0.25))
    {
        // somehow the lowpassed audio is so quiet / we don't have a lot of lowpassed info. it barely makes a dent compared to the raw waveform.
        // take the first clean rising edge
        auto armLevel = raw.mid - raw.amplitude * SampleType(0.2);
        auto fireLevel = raw.mid + raw.amplitude * SampleType(0.2);
        
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

    auto armLevel = lowPassed.mid - lowPassed.amplitude * SampleType(0.2);
    auto fireLevel = lowPassed.mid + lowPassed.amplitude * SampleType(0.2);
    
    auto searchEndDecimated = searchEnd / triggerDecimation;

    size_t best = 0;
    SampleType bestError = 0;
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

        SampleType error = 0;

        // sum of squared differences
        for (size_t k = 0; k < triggerMatchLength; ++k)
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
    std::copy(lockedShape, lockedShape + (std::ptrdiff_t) triggerMatchLength, triggerReference.begin());
    hasTriggerReference = true;

    // we have to check the exact spot in our real data to continue from
    // because we only did detection on our decimated dataset
    auto coarse = best * triggerDecimation;
    auto from = coarse > triggerDecimation ? coarse - triggerDecimation : size_t(1);
    auto to = std::min(coarse + triggerDecimation, searchEnd);

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

template <typename SampleType>
void Scope<SampleType>::drawLRScope(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto h = scopeRect.getHeight();
    const auto hop = hopSize;

    g.setColour(juce::Colours::grey);
    plotStraightLine(originLineData.data(), 2, g, scopeRect, SampleType(0.5), h / 2);
    plotStraightLine(originLineData.data(), 2, g, scopeRect, SampleType(-0.5), h / 2);

    // trigger offset is position where trigger was detected
    g.setColour(juce::Colours::yellow);
    plotStraightLine(sampleDataL.data() + triggerOffset, hop, g, scopeRect, SampleType(0.5), h / 2);
    g.setColour(juce::Colours::lime);
    plotStraightLine(sampleDataR.data() + triggerOffset, hop, g, scopeRect, SampleType(0.5), h / 2);
}


template <typename SampleType>
void Scope<SampleType>::drawInOut(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    if (sampleDataPreDistortion.size() > 1 && sampleDataPostDistortion.size() > 1)
    {
        // centred under the header, so the top of the curve cannot run up behind it
        const auto plot = scopeRect.withTrimmedTop(headerHeight(scopeRect));

        const auto centerX = plot.getCentreX();
        const auto centerY = plot.getCentreY();
        const auto halfW = plot.getHeight() * SampleType(0.95);
        const auto halfH = plot.getHeight() * SampleType(0.46);

        g.setColour(juce::Colours::yellow);
        const auto count = juce::jmin(sampleDataPreDistortion.size(), sampleDataPostDistortion.size());
        for (size_t i = 1; i < count; ++i)
        {
            const auto x0 = sampleDataPreDistortion[i - 1];
            const auto y0 = sampleDataPostDistortion[i - 1];
            const auto x1 = sampleDataPreDistortion[i];
            const auto y1 = sampleDataPostDistortion[i];

            const auto px0 = centerX + x0 * halfW;
            const auto py0 = centerY - y0 * halfH;
            const auto px1 = centerX + x1 * halfW;
            const auto py1 = centerY - y1 * halfH;

            g.drawLine(px0, py0, px1, py1);
        }
    }
}

// kept out of the framebuffer so the axes stay crisp instead of being stamped and faded every frame
template <typename SampleType>
void Scope<SampleType>::drawInOutAxes(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto w = scopeRect.getWidth();
    const auto h = scopeRect.getHeight();

    const auto centerX = w * SampleType(0.5);
    const auto centerY = h * SampleType(0.5);

    g.setColour(juce::Colours::grey);
    g.drawLine(centerX, SampleType(0), centerX, h);
    g.drawLine(SampleType(0), centerY, w, centerY);
}

// accumulates one frame of the trace into the framebuffer, called from the timer so trail
// density follows the frame rate rather than however often the os asks us to repaint
template <typename SampleType>
void Scope<SampleType>::renderInOutFrame(bool stampNewTrace)
{
    juce::Graphics imgG(inOutFB);

    // fading toward the background rather than clearing is what leaves the older frames behind
    imgG.setColour(inOutBackground.withAlpha(inOutFade));
    imgG.fillRect(inOutFB.getBounds());

    if (!stampNewTrace)
        return;

    imgG.addTransform(juce::AffineTransform::scale(inOutRenderScale));
    drawInOut(imgG, juce::Rectangle<SampleType>{SampleType(0), SampleType(0),
                                                (SampleType) getWidth(), (SampleType) getHeight()});
}


template <typename SampleType>
void Scope<SampleType>::drawSpectrumEmphasis(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
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
        constexpr auto tiltDbPerOctave = SampleType(4.5);
        constexpr auto mindB = SampleType(-70);
        constexpr auto maxdB = SampleType(18);

        juce::Path spectrumPath;
        bool startedSpectrumPath = false;

        for (size_t i = 0; i < spectrumTransformed.size(); ++i)
        {
            const auto freq = static_cast<double>(i) * binToHz;

            if (freq < scope_constants::minDrawFreq)
                continue;

            const auto x = freqToX(freq, w);

            const auto dbValue = juce::jmap(spectrumTransformed[i], SampleType(0), SampleType(1), mindB, maxdB);
            const auto octaveIndex = std::max(SampleType(0), std::log2(static_cast<SampleType>(nyquist / freq)));
            const auto tiltDb = -tiltDbPerOctave * octaveIndex;
            const auto adjustedDb = dbValue + tiltDb;
            const auto magnitude = juce::jlimit(SampleType(0), SampleType(1), juce::jmap(
                juce::jlimit(mindB, maxdB, adjustedDb),
                mindB,
                maxdB,
                SampleType(0),
                SampleType(1)));
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


template <typename SampleType>
void Scope<SampleType>::drawClipper(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto w = (float) scopeRect.getWidth();
    const auto h = (float) scopeRect.getHeight();

    const auto knee = postClipKneeParam != nullptr ? postClipKneeParam->get() * 0.5f : 0.0f;

    constexpr float threshold = 1.0f;
    constexpr float maxIn = 2.5f;
    constexpr float maxOut = 1.25f;

    // mapping between pixel space and cartesian plane type coords, with the header strip kept clear
    const auto plot = scopeRect.withTrimmedTop(headerHeight(scopeRect));

    auto toX = [w, maxIn](float in) { return juce::jmap(in, 0.0f, maxIn, 0.0f, w); };
    auto toY = [&plot, maxOut](float out) { return juce::jmap(out, 0.0f, maxOut, (float) plot.getBottom() - 1.0f, (float) plot.getY() + 1.0f); };

    // shaded knee region
    if (knee > 0.0f)
    {
        const auto kneeStart = toX(threshold - knee * 0.5f);
        const auto kneeEnd = toX(threshold + knee * 0.5f);

        // translucent rather than a solid fill, so the watermark underneath still reads through it -
        // alpha 22 over black lands on the same 22,22,22 the solid version used to paint
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 22));
        g.fillRect(juce::Rectangle<float>(kneeStart, 0.0f, kneeEnd - kneeStart, h));
    }

    // line at 1.0
    g.setColour(juce::Colours::darkgrey);
    g.drawLine(0.0f, toY(0.0f), w, toY(0.0f));

    // f(x) = x
    g.setColour(juce::Colour::fromRGB(64, 64, 64));
    g.drawLine(toX(0.0f), toY(0.0f), toX(maxOut), toY(maxOut));

    // where the flat top lands
    g.drawLine(0.0f, toY(threshold), w, toY(threshold));

    constexpr int numPoints = 64;

    juce::Path curve;
    for (int i = 0; i <= numPoints; ++i)
    {
        const float in = juce::jmap((float) i, 0.0f, (float) numPoints, 0.0f, maxIn);
        const float x = toX(in);
        const float y = toY(softClipperFunc(in, threshold, knee));

        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    // translucent, so the knee strip and the watermark still read through the fill
    auto closeToBaseline = [&](juce::Path path, float rightX) {
        path.lineTo(rightX, toY(0.0f));
        path.lineTo(toX(0.0f), toY(0.0f));
        path.closeSubPath();
        return path;
    };

    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.fillPath(closeToBaseline(curve, toX(maxIn)));

    g.setColour(juce::Colours::darkgrey);
    g.strokePath(curve, juce::PathStrokeType(2.0f));
    
    const auto level = juce::jlimit(0.0f, maxIn, dataCollector.levelMeter.getNext());

    auto levelColour = juce::Colours::yellow;
    if (level > threshold + knee * 0.5f)
        levelColour = juce::Colours::red;
    else if (isSoftClipperKnee(level, threshold, knee))
        levelColour = juce::Colours::orange;

    g.setColour(levelColour);
    
    if (level > 0.0f)
    {
        juce::Path active;
        for (int i = 0; i <= numPoints; ++i)
        {
            const auto in = juce::jmap((float) i, 0.0f, (float) numPoints, 0.0f, level);
            const auto x = toX(in);
            const auto y = toY(softClipperFunc(in, threshold, knee));

            if (i == 0)
                active.startNewSubPath(x, y);
            else
                active.lineTo(x, y);
        }

        g.setColour(levelColour.withAlpha(0.18f));
        g.fillPath(closeToBaseline(active, toX(level)));

        g.setColour(levelColour);
        g.strokePath(active, juce::PathStrokeType(2.5f));
    }

    constexpr auto dotRadius = 3.0f;
    const auto outAtLevel = softClipperFunc(level, threshold, knee);

    g.fillEllipse(toX(level) - dotRadius, toY(outAtLevel) - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

// watermark naming the current context, tiled over the whole view with odd rows staggered and
// every row started off screen so the pattern runs past the edges
template <typename SampleType>
void Scope<SampleType>::drawTiledContextLabel(juce::Graphics &g, juce::Rectangle<int> area, const juce::String &text)
{
    const auto w = (float) area.getWidth();
    const auto h = (float) area.getHeight();

    if (w <= 0.0f || h <= 0.0f)
        return;

    const auto font = getLookAndFeel().getPopupMenuFont().withHeight(juce::jlimit(12.0f, 34.0f, h * 0.22f));

    g.setFont(font);
    g.setColour(juce::Colour::fromRGBA(255, 255, 255, contextLabelAlpha));

    const auto rowHeight = font.getHeight();
    const auto tileWidth = juce::GlyphArrangement::getStringWidth(font, text);

    if (tileWidth <= 0.0f || rowHeight <= 0.0f)
        return;

    // one long pre-repeated string per row, so the font lays out the joins - drawing each repeat
    // separately would round every start x and leave a seam
    juce::String row;

    for (int repeat = (int) std::ceil((w + tileWidth * 2.0f) / tileWidth); repeat > 0; --repeat)
        row << text;

    int rowIndex = 0;

    for (auto y = -rowHeight * 0.3f; y < h; y += rowHeight, ++rowIndex)
    {
        // both the bleed and the stagger pull left, or a staggered row opens a gap at the left edge
        const auto startX = -tileWidth * 0.35f - (rowIndex % 2 == 1 ? tileWidth * 0.5f : 0.0f);

        g.drawSingleLineText(row, juce::roundToInt(startX), juce::roundToInt(y + font.getAscent()));
    }
}

// one cell per band
template <typename SampleType>
void Scope<SampleType>::drawCompBands(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect,
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
    g.setFont(readoutFontHeight);

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

template <typename SampleType>
void Scope<SampleType>::drawMBComp(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
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

template <typename SampleType>
void Scope<SampleType>::drawMSComp(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto thr = paramValue(ParamIDs::MSCompThreshold);
    const auto tilt = paramValue(ParamIDs::compBandTilt);

    const std::array<CompBand, 2> bands {
        CompBand { thr - tilt, bandLevel(dataCollector.band1), "MID" },
        CompBand { thr + tilt, bandLevel(dataCollector.band2), "SIDE" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), compRatio());
}

template <typename SampleType>
void Scope<SampleType>::drawTypeAComp(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto thr = paramValue(ParamIDs::TypeAThreshold);
    const auto tilt = paramValue(ParamIDs::TypeATilt);

    const std::array<CompBand, 4> bands {
        CompBand { thr, bandLevel(dataCollector.band1), "LOW",  false, -tilt },
        CompBand { thr, bandLevel(dataCollector.band2), "MID",  false, 0.0f },
        CompBand { thr, bandLevel(dataCollector.band3), "HIGH", false, tilt * 0.5f },
        CompBand { thr, bandLevel(dataCollector.band4), "X-HI", true,  tilt * 0.5f }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), TypeAProcessor::baseRatio);
}


template <typename SampleType>
void Scope<SampleType>::drawStereoComp(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto thr = paramValue(ParamIDs::stereoCompThreshold);

    const std::array<CompBand, 2> bands {
        CompBand { thr, bandLevel(dataCollector.band1), "LEFT" },
        CompBand { thr, bandLevel(dataCollector.band2), "RIGHT" }
    };

    drawCompBands(g, scopeRect, bands.data(), (int) bands.size(), compRatio());
}

template <typename SampleType>
SampleType Scope<SampleType>::headerHeight(juce::Rectangle<SampleType> scopeRect) const
{
    return (SampleType) juce::jmin(16.0f, (float) scopeRect.getHeight() * 0.2f);
}

// readouts across the top of a view, ends hugging the corners. three already fill the strip so they
// get one solid bar, fewer than that get a tab each and leave the gaps open
template <typename SampleType>
void Scope<SampleType>::drawParamHeader(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect, const juce::StringArray &labels)
{
    if (labels.isEmpty())
        return;

    const auto bar = juce::Rectangle<float>(0.0f, 0.0f, (float) scopeRect.getWidth(), (float) headerHeight(scopeRect));

    g.setFont(getLookAndFeel().getPopupMenuFont());
    g.setFont(readoutFontHeight);

    const auto solidBar = labels.size() > 2;

    if (solidBar)
    {
        g.setColour(juce::Colours::black);
        g.fillRect(bar);
    }

    const auto cells = bar.reduced(4.0f, 0.0f);
    const auto cellWidth = cells.getWidth() / (float) labels.size();

    for (int i = 0; i < labels.size(); ++i)
    {
        const auto isFirst = i == 0;
        const auto isLast = i == labels.size() - 1;
        const auto justification = isFirst ? juce::Justification::centredLeft
                                           : (isLast ? juce::Justification::centredRight
                                                     : juce::Justification::centred);

        const auto cell = cells.withX(cells.getX() + (float) i * cellWidth).withWidth(cellWidth);

        if (solidBar)
        {
            g.setColour(juce::Colours::grey);
            g.drawText(labels[i], cell, justification, false);
        }
        else
        {
            drawTabbedLabel(g, cell, labels[i], justification, true);
        }
    }
}

template <typename SampleType>
void Scope<SampleType>::drawTabbedLabel(juce::Graphics &g, juce::Rectangle<float> cell, const juce::String &text,
                                        juce::Justification justification, bool hangingFromTop)
{
    if (text.isEmpty())
        return;

    constexpr auto corner = 5.0f;

    auto tab = cell.withWidth(juce::jmin(cell.getWidth(),
                                         juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), text) + corner * 2.0f));

    const auto horizontal = justification.getOnlyHorizontalFlags();

    if (horizontal == juce::Justification::right)
        tab.setX(cell.getRight() - tab.getWidth());
    else if (horizontal == juce::Justification::horizontallyCentred)
        tab.setCentre(cell.getCentreX(), tab.getCentreY());

    // grown past the outer edge so only the inner corners round off
    const auto grown = hangingFromTop ? tab.withTop(tab.getY() - corner)
                                      : tab.withBottom(tab.getBottom() + corner);

    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(grown.expanded(corner * 0.5f, 0.0f), corner);

    g.setColour(juce::Colours::grey);
    g.drawText(text, cell, justification, false);
}

// bottom right corner rather than the header, since that corner of the transfer plot stays empty
template <typename SampleType>
void Scope<SampleType>::drawDistortionAmount(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto label = getDistortionAmountLabel();

    if (label.isEmpty())
        return;

    g.setFont(getLookAndFeel().getPopupMenuFont());
    g.setFont(readoutFontHeight);

    const auto rowHeight = (float) headerHeight(scopeRect);
    const auto row = juce::Rectangle<float>(0.0f, (float) scopeRect.getHeight() - rowHeight,
                                            (float) scopeRect.getWidth(), rowHeight).reduced(4.0f, 0.0f);

    drawTabbedLabel(g, row, label, juce::Justification::centredRight, false);
}

template <typename SampleType>
float Scope<SampleType>::paramValue(const ParamIDs::ParameterInfo &paramInfo) const
{
    if (auto *param = dynamic_cast<juce::AudioParameterFloat *>(apvts.getParameter(paramInfo.getParamID())))
        return param->get();

    return 0.0f;
}

template <typename SampleType>
float Scope<SampleType>::compRatio() const
{
    return juce::jmax(1.0f, paramValue(ParamIDs::compRatio));
}

template <typename SampleType>
float Scope<SampleType>::bandLevel(LevelMeter &meter) const
{
    return meter.getNext();
}

// only the threshold differs between the three compressor views, and the stereo one has no tilt
template <typename SampleType>
juce::StringArray Scope<SampleType>::getCompHeaderLabels(const ParamIDs::ParameterInfo &thresholdParam, bool withTilt) const
{
    juce::StringArray labels { formatDecibels(paramValue(thresholdParam)),
                               juce::String(compRatio(), 1) + ":1" };

    if (withTilt)
        labels.add(formatDecibels(paramValue(ParamIDs::compBandTilt)));

    return labels;
}

template <typename SampleType>
juce::AudioParameterChoice* Scope<SampleType>::choiceParam(const ParamIDs::ParameterInfo &id) const
{
    return dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter(id.getParamID()));
}

template <typename SampleType>
juce::StringArray Scope<SampleType>::getDistortionHeaderLabels() const
{
    auto *type = choiceParam(ParamIDs::primaryDistortionType);

    if (type == nullptr)
        return {};

    return { type->getCurrentChoiceName() };
}

// every type keeps its own amount parameter on its own range, and matrix has no single one
template <typename SampleType>
juce::String Scope<SampleType>::getDistortionAmountLabel() const
{
    auto *type = choiceParam(ParamIDs::primaryDistortionType);

    if (type == nullptr)
        return {};

    switch (type->getIndex())
    {
        case 0: return formatPercent(paramValue(ParamIDs::saturationAmount) * 0.01f); // GRILL
        case 1: return formatPercent(paramValue(ParamIDs::tubeAmount) * 0.01f);       // TUBE
        case 2: return formatPercent(paramValue(ParamIDs::phaseAmount) * 0.01f);      // PHASE
        case 3: return formatPercent(paramValue(ParamIDs::rubidiumAmount) * 0.01f);   // RUBIDIUM
        case 5: return formatPercent(paramValue(ParamIDs::tapeDrive));                // TAPE
        case 6: return formatPercent(paramValue(ParamIDs::alphaParam));               // SLEW
        default: return {};                                                            // MATRIX
    }
}

// only the sizzles and erosion have the full frequency / amount / q set
template <typename SampleType>
juce::StringArray Scope<SampleType>::getNoiseHeaderLabels() const
{
    auto *type = choiceParam(ParamIDs::noiseDistortionType);

    if (type == nullptr)
        return {};

    switch (type->getIndex())
    {
        case 1: // EROSION
            return { formatFrequency(paramValue(ParamIDs::erosionFrequency)),
                     formatPercent(paramValue(ParamIDs::erosionAmount) * 0.01f),
                     "Q " + juce::String(paramValue(ParamIDs::erosionQ), 2) };

        case 2: // BIT, no q - the bit depth is the interesting middle value instead
            return { formatFrequency(paramValue(ParamIDs::downsampleFreq)),
                     juce::String(juce::roundToInt(paramValue(ParamIDs::bitReduction))) + " bit",
                     formatPercent(paramValue(ParamIDs::downsampleMix)) };

        case 3: // JEFF, the gate only has amount and mix
            return { formatPercent(paramValue(ParamIDs::gateAmt)),
                     "MIX " + formatPercent(paramValue(ParamIDs::gateMix)) };

        default: // SIZZLE and SIZZLE_OG
            return { formatFrequency(paramValue(ParamIDs::sizzleFrequency)),
                     formatPercent(paramValue(ParamIDs::sizzleAmount) * 0.01f),
                     "Q " + juce::String(paramValue(ParamIDs::sizzleQ), 2) };
    }
}

// single source of truth for the x axis, everything drawn over the spectrum has to go through this
template <typename SampleType>
SampleType Scope<SampleType>::freqToX(double freq, SampleType w) const
{
    const auto clamped = juce::jlimit(scope_constants::minDrawFreq, scope_constants::maxDrawFreq, freq);

    return static_cast<SampleType>(juce::jmap(std::log10(clamped),
                                              std::log10(scope_constants::minDrawFreq),
                                              std::log10(scope_constants::maxDrawFreq),
                                              0.0,
                                              static_cast<double>(w)));
}

template <typename SampleType>
float Scope<SampleType>::getBandResponse(double freq, double centerFreq, double gainDb)
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

template <typename SampleType>
void Scope<SampleType>::drawResponseCurve(juce::Graphics &g, const SampleType w, const SampleType centerY, const SampleType maxHeight)
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
        const auto y = static_cast<float>(centerY - static_cast<SampleType>(response) * maxHeight);

        g.fillEllipse(x - dotRadius, y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }
}


template <typename SampleType>
void Scope<SampleType>::timerCallback()
{
    auto& queueL = dataCollector.audioBufferQueueL;
    auto& queueR = dataCollector.audioBufferQueueR;
    auto& preDist = dataCollector.audioBufferQueuePreDistortion;
    auto& postDist = dataCollector.audioBufferQueuePostDistortion;

    updateHopSize();

    scopeContext.updateFrame();

    const auto hop = hopSize;
    const auto newestHop = sampleDataL.size() - hop;
    const auto spectrumView = scopeContext.getType() == ScopeContextType::SPECTRUM_EMPHASIS;

    // drain whatever has piled up rather than exactly one hop. the timer does not fire on an exact
    // 60hz, so even a matched hop drifts, and once the queue is full push() starts dropping the
    // tail of every block, which splices the waveform. the queue holds a tenth of a second, so this
    // cap is always enough to empty it
    constexpr int maxHopsPerTick = 8;
    int hopsPopped = 0;

    while (hopsPopped < maxHopsPerTick
           && queueL.getReadableSpace() >= (int) hop
           && queueR.getReadableSpace() >= (int) hop) {
        // the older hops slide down to make room for the new one
        std::copy(sampleDataL.begin() + hop, sampleDataL.end(), sampleDataL.begin());
        std::copy(sampleDataR.begin() + hop, sampleDataR.end(), sampleDataR.begin());

        queueL.pop(sampleDataL.data() + newestHop, hop);
        queueR.pop(sampleDataR.data() + newestHop, hop);

        ++hopsPopped;

        // every hop has to reach the spectrum history, even the ones only drained to catch up
        if (spectrumView) {
            for (size_t i = 0; i < hop; ++i)
            {
                const auto writeIndex = (fftHistoryWritePosition + i) % fftHistory.size();
                fftHistory[writeIndex] = sampleDataL[newestHop + i];
            }

            fftHistoryWritePosition = (fftHistoryWritePosition + hop) % fftHistory.size();
        }
    }

    if (hopsPopped > 0) {
        // only the lr scope draws from the trigger
        if (scopeContext.getType() == ScopeContextType::LR_SCOPE)
            updateTriggerOffset();

        if (spectrumView) {
            const auto fftSize = fft.getSize();

            if (fftHistory.size() >= fftSize)
            {
                std::fill(spectrumData.begin(), spectrumData.end(), SampleType(0));
                std::fill(spectrumTransformed.begin(), spectrumTransformed.end(), SampleType(0));

                for (int i = 0; i < fftSize; ++i)
                {
                    const auto historyIndex = (fftHistoryWritePosition + i + fftHistory.size() - fftSize) % fftHistory.size();
                    spectrumData[i] = fftHistory[historyIndex];
                }

                windowFun.multiplyWithWindowingTable(spectrumData.data(), fftSize);
                fft.performFrequencyOnlyForwardTransform(spectrumData.data(), true);

                static constexpr auto mindB = SampleType(-70);
                static constexpr auto maxdB = SampleType(50);
                const auto binsToRender = juce::jmin<int>((int) spectrumTransformed.size(), fftSize / 2 + 1);

                for (int i = 0; i < binsToRender; ++i)
                {
                    const auto magnitude = juce::jlimit(SampleType(1.0e-6), SampleType(1.0e6), spectrumData[i]);
                    const auto db = juce::Decibels::gainToDecibels(magnitude, -70.0f);
                    const auto normalized = juce::jmap(
                        db,
                        mindB,
                        maxdB,
                        SampleType(0),
                        SampleType(1));

                    const auto smoothed = averagedSpectrum[i] * scope_constants::spectrumSmoothing + normalized * (SampleType(1) - scope_constants::spectrumSmoothing);
                    averagedSpectrum[i] = smoothed;
                    spectrumTransformed[i] = smoothed;
                }
            }
        }
    }

    bool poppedInOut = false;
    int inOutPopped = 0;

    // same drain, so the distortion trail cannot splice either
    while (inOutPopped < maxHopsPerTick
           && preDist.getReadableSpace() >= (int) sampleDataPreDistortion.size()
           && postDist.getReadableSpace() >= (int) sampleDataPostDistortion.size()) {
        preDist.pop(sampleDataPreDistortion.data(), sampleDataPreDistortion.size());
        postDist.pop(sampleDataPostDistortion.data(), sampleDataPostDistortion.size());

        ++inOutPopped;
        poppedInOut = true;
    }

    // fade every tick even without new data, so the trails decay away when the audio stops
    if (scopeContext.getType() == ScopeContextType::IN_OUT && inOutFB.isValid())
        renderInOutFrame(poppedInOut);

    repaint(getLocalBounds());
}


template <typename SampleType>
void Scope<SampleType>::plotStraightLine(const SampleType *data,
                 size_t numSamples,
                 juce::Graphics &g,
                 juce::Rectangle<SampleType> rect,
                 SampleType scaler,
                 SampleType offset)
{
    auto w = rect.getWidth();
    auto h = rect.getHeight();
    auto right = rect.getRight();

    auto center = rect.getBottom() - offset;
    auto gain = h * scaler;

    for (size_t i = 1; i < numSamples; ++i)
        g.drawLine({juce::jmap(SampleType(i - 1), SampleType(0), SampleType(numSamples - 1), SampleType(right - w), SampleType(right)),
                    center - gain * data[i - 1],
                    juce::jmap(SampleType(i), SampleType(0), SampleType(numSamples - 1), SampleType(right - w), SampleType(right)),
                    center - gain * data[i]});
}


template class Scope<float>;
