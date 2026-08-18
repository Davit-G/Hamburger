#include "Scope.h"

#include "../../dsp/WaveShapers.h"



ScopeContextType ScopeContext::getType()
{
    return type;
}

void ScopeContext::setType(ScopeContextType newType)
{
    type = newType;
}


template <typename SampleType>
Scope<SampleType>::Scope(juce::AudioProcessorValueTreeState& valueTree, ScopeDataCollector<SampleType>& scopeDataCollector, ScopeContext &newScopeContext)
    : apvts(valueTree),
    dataCollector(scopeDataCollector),
      scopeContext(newScopeContext),
      noiseDist(valueTree)
{
    int fps = scope_constants::defaultFrameRate;
    sampleDataL.resize(scope_constants::defaultHopSize);
    sampleDataR.resize(scope_constants::defaultHopSize);
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

    
    switch (currentType) {
        case ScopeContextType::LR_SCOPE:
            drawLRScope(g, scopeRect);
        break;
        case ScopeContextType::IN_OUT:
            if (inOutFB.isValid())
                g.drawImage(inOutFB, area.toFloat());
        
            g.setColour(juce::Colour::fromRGBA(255, 255, 255, 10));
            g.setFont(hamburgerLAF.getPopupMenuFont());
            g.setFont(30);

            // .removeFromBottom(area.getHeight() / 3)
            g.drawText("DISTORTION", area.expanded(50), juce::Justification::centred, false);
        
            // drawInOutAxes(g, scopeRect);
            break;
        case ScopeContextType::SPECTRUM_EMPHASIS:
            g.drawText("EMPHASIS", area.expanded(50), juce::Justification::centred, false);

            drawSpectrumEmphasis(g, scopeRect);
            break;
        case ScopeContextType::CLIPPER:
            g.drawText("CLIPPER", area.expanded(50), juce::Justification::centred, false);
            drawClipper(g, scopeRect);
            break;
        case ScopeContextType::NOISE:
            // we draw based on actual result from our signal

            for (int i = 0; i < noiseDistBuf.getNumSamples(); i++) {
                noiseDistBuf.setSample(0, i, 0.7f * sin(6.28f * (i - 32.0f) * 100.f / 44100.f));
            }
            auto block = juce::dsp::AudioBlock<float>(noiseDistBuf);
            noiseDist.processBlock(block);

            // draw wave
            g.setColour(juce::Colours::yellow);
            plotStraightLine(noiseDistBuf.getReadPointer(0) + 32, noiseDistBuf.getNumSamples() - 32, g, scopeRect, SampleType(0.5), h / 2);
            break;
    }
}


template <typename SampleType>
void Scope<SampleType>::drawLRScope(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto h = scopeRect.getHeight();

    g.setColour(juce::Colours::grey);
    plotStraightLine(originLineData.data(), 2, g, scopeRect, SampleType(0.4), h / 2);
    plotStraightLine(originLineData.data(), 2, g, scopeRect, SampleType(-0.4), h / 2);


    g.setColour(juce::Colours::yellow);
    plotStraightLine(sampleDataL.data(), sampleDataL.size(), g, scopeRect, SampleType(0.4), h / 2);
    g.setColour(juce::Colours::lime);
    plotStraightLine(sampleDataR.data(), sampleDataR.size(), g, scopeRect, SampleType(0.4), h / 2);
}


template <typename SampleType>
void Scope<SampleType>::drawInOut(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    if (sampleDataPreDistortion.size() > 1 && sampleDataPostDistortion.size() > 1)
    {
        const auto w = scopeRect.getWidth();
        const auto h = scopeRect.getHeight();

        const auto centerX = w * SampleType(0.5);
        const auto centerY = h * SampleType(0.5);
        const auto halfW = h * 0.95;
        const auto halfH = h * SampleType(0.46);

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

        g.setColour(juce::Colours::yellow);
        const auto centerY = h;
        const auto maxHeight = h;
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
}


template <typename SampleType>
void Scope<SampleType>::drawClipper(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect)
{
    const auto w = (float) scopeRect.getWidth();
    const auto h = (float) scopeRect.getHeight();

    const auto knee = postClipKneeParam != nullptr ? postClipKneeParam->get() * 0.5f : 0.0f;

    constexpr auto threshold = 1.0f;
    constexpr auto maxIn = 2.5f;
    constexpr auto maxOut = 1.25f;

    // mapping between pixel space and cartesian plane type coords
    auto toX = [w](float in) { return juce::jmap(in, 0.0f, maxIn, 0.0f, w); };
    auto toY = [h](float out) { return juce::jmap(out, 0.0f, maxOut, h - 1.0f, 1.0f); };

    // shaded knee region
    if (knee > 0.0f)
    {
        const auto kneeStart = toX(threshold - knee * 0.5f);
        const auto kneeEnd = toX(threshold + knee * 0.5f);

        g.setColour(juce::Colour::fromRGB(22, 22, 22));
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
        const auto in = juce::jmap((float) i, 0.0f, (float) numPoints, 0.0f, maxIn);
        const auto x = toX(in);
        const auto y = toY(softClipperFunc(in, threshold, knee));

        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    g.setColour(juce::Colours::darkgrey);
    g.strokePath(curve, juce::PathStrokeType(2.0f));
    
    const auto level = juce::jlimit(0.0f, maxIn, dataCollector.levelMeter.getNext(scope_constants::defaultFrameRate));

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

        g.strokePath(active, juce::PathStrokeType(2.5f));
    }

    constexpr auto dotRadius = 3.0f;
    const auto outAtLevel = softClipperFunc(level, threshold, knee);

    g.fillEllipse(toX(level) - dotRadius, toY(outAtLevel) - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
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
}


template <typename SampleType>
void Scope<SampleType>::timerCallback()
{
    auto& queueL = dataCollector.audioBufferQueueL;
    auto& queueR = dataCollector.audioBufferQueueR;
    auto& preDist = dataCollector.audioBufferQueuePreDistortion;
    auto& postDist = dataCollector.audioBufferQueuePostDistortion;

    if (queueL.getReadableSpace() >= sampleDataL.size() && queueR.getReadableSpace() >= sampleDataR.size()) {
        queueL.pop(sampleDataL.data(), sampleDataL.size());
        queueR.pop(sampleDataR.data(), sampleDataR.size());

        if (scopeContext.getType() == ScopeContextType::SPECTRUM_EMPHASIS) {
            const auto fftSize = fft.getSize();

            for (size_t i = 0; i < sampleDataL.size(); ++i)
            {
                const auto writeIndex = (fftHistoryWritePosition + i) % fftHistory.size();
                fftHistory[writeIndex] = sampleDataL[i];
            }

            fftHistoryWritePosition = (fftHistoryWritePosition + sampleDataL.size()) % fftHistory.size();

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

    if (preDist.getReadableSpace() >= sampleDataPreDistortion.size() && postDist.getReadableSpace() >= sampleDataPostDistortion.size()) {
        preDist.pop(sampleDataPreDistortion.data(), sampleDataPreDistortion.size());
        postDist.pop(sampleDataPostDistortion.data(), sampleDataPostDistortion.size());

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
