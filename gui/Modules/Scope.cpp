#include "Scope.h"
#include "../../dsp/EffectInfos.h"

Scope::Scope(juce::AudioProcessorValueTreeState& valueTree, ScopeDataCollector<float>& scopeDataCollector, ScopeContext &newScopeContext, ModMatrix& modMatrix)
    : matrix(modMatrix),
      apvts(valueTree),
    dataCollector(scopeDataCollector),
      scopeContext(newScopeContext),
      noiseDist(valueTree)
{
    int fps = scope_constants::defaultFrameRate;
    sampleDataL.resize(scope_constants::defaultHopSize * 3, 0.0f);
    sampleDataR.resize(scope_constants::defaultHopSize * 3, 0.0f);
    triggerSignal.resize(sampleDataL.size(), 0.0f);
    triggerDecimated.resize(sampleDataL.size() / scope_constants::triggerDecimation, 0.0f);
    triggerReference.resize(scope_constants::triggerMatchLength, 0.0f);
    sampleDataPreDistortion.resize(scope_constants::defaultHopSize);
    sampleDataPostDistortion.resize(scope_constants::defaultHopSize);
    setFramesPerSecond(fps);

    lowFreqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisLowFreq.getParamID()));
    highFreqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisHighFreq.getParamID()));
    lowGainParam = &MacroParam::fetch(apvts, ParamIDs::emphasisLowGain.getParamID());
    highGainParam = &MacroParam::fetch(apvts, ParamIDs::emphasisHighGain.getParamID());
    tiltParam = &MacroParam::fetch(apvts, ParamIDs::emphasisTilt.getParamID());
    postClipKneeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(EffectInfos::paramIdForDescriptor(ParamIDs::postClipKnee)));

    compressionType = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(SlotId{ModuleId::dynamics, 0}.type().getParamID()));

    juce::dsp::ProcessSpec spec; // this is not realtime so whatever
    spec.maximumBlockSize = 128;
    spec.numChannels = 2;
    spec.sampleRate = 44100;

    noiseDist.prepare(spec);
    noiseDistBuf.setSize(2, 512, false, true, false);
}

void Scope::resized() {
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
    imgG.fillAll(theme().scopeBackground);
}

// the trails fade into the background colour, so a new one starts them over
void Scope::lookAndFeelChanged()
{
    if (trailBackground != theme().scopeBackground)
    {
        trailBackground = theme().scopeBackground;
        resized();
    }
}

void Scope::setFramesPerSecond(int framesPerSecond)
{
    jassert(framesPerSecond > 0 && framesPerSecond < 1000);
    startTimerHz(framesPerSecond);
}

// which view is on screen, each drawn in its own file
void Scope::paint(juce::Graphics &g)
{
    const auto area = getLocalBounds();

    switch (scopeContext.getType()) {
        case ScopeContextType::LR_SCOPE:          drawLRView(g, area); break;
        case ScopeContextType::IN_OUT:
        case ScopeContextType::WAVESHAPE:         drawDistortionView(g, area); break;
        case ScopeContextType::SPECTRUM_EMPHASIS: drawEmphasisView(g, area); break;
        case ScopeContextType::SPECTRUM_TILT:     drawTiltView(g, area); break;
        case ScopeContextType::CLIPPER:           drawClipperView(g, area); break;
        case ScopeContextType::COMPRESSION:       drawCompressorView(g, area); break;
        case ScopeContextType::NOISE:             drawNoiseView(g, area); break;
        case ScopeContextType::MODULATOR:         drawModulatorView(g, area); break;
        case ScopeContextType::WATERMARK:
            g.fillAll(theme().scopeBackground);
            drawTiledContextLabel(g, area, "HAMBURGER");
            break;
        default: break;
    }
}

void Scope::updateHopSize()
{
    const auto sampleRate = dataCollector.getSampleRate();
    
    if (sampleRate <= 0.0 || juce::approximatelyEqual(sampleRate, preparedSampleRate))
    return;
    
    preparedSampleRate = sampleRate;
    hopSize = (size_t) juce::jmax(64, juce::roundToInt(sampleRate / (double) scope_constants::defaultFrameRate));
    
    sampleDataL.assign(hopSize * 3, 0.0f);
    sampleDataR.assign(hopSize * 3, 0.0f);
    triggerSignal.assign(sampleDataL.size(), 0.0f);
    triggerDecimated.assign(sampleDataL.size() / scope_constants::triggerDecimation, 0.0f);
    sampleDataPreDistortion.assign(hopSize, 0.0f);
    sampleDataPostDistortion.assign(hopSize, 0.0f);
    
    triggerOffset = 0;
    hasTriggerReference = false;
}

void Scope::timerCallback()
{
    auto& queueL = dataCollector.audioBufferQueueL;
    auto& queueR = dataCollector.audioBufferQueueR;
    auto& preDist = dataCollector.audioBufferQueuePreDistortion;
    auto& postDist = dataCollector.audioBufferQueuePostDistortion;

    updateHopSize();
    recordModulators();

    scopeContext.updateFrame();

    const auto hop = hopSize;
    const auto newestHop = sampleDataL.size() - hop;
    const auto spectrumView = scopeContext.getType() == ScopeContextType::SPECTRUM_EMPHASIS
                              || scopeContext.getType() == ScopeContextType::SPECTRUM_TILT;

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
        if (spectrumView)
            spectrum.push(sampleDataL.data() + newestHop, hop);
    }

    if (hopsPopped > 0) {
        // only the lr scope draws from the trigger
        if (scopeContext.getType() == ScopeContextType::LR_SCOPE)
            updateTriggerOffset();

        if (spectrumView)
            spectrum.update();
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
    const auto inOutView = scopeContext.getType() == ScopeContextType::IN_OUT || scopeContext.getType() == ScopeContextType::WAVESHAPE;

    if (inOutView && inOutFB.isValid())
        renderInOutFrame(poppedInOut);

    repaint(getLocalBounds());
}

// caching parameter fetches here so we dont need to keep checking what the IDs are every time
float Scope::paramValue(const ParamIDs::ParameterInfo &paramInfo) const
{
    auto [entry, inserted] = paramCache.try_emplace(&paramInfo, nullptr);

    if (inserted)
    {
        // most descriptors are macros now; fall back to the descriptor's own id for fixed ones
        auto id = EffectInfos::paramIdForDescriptor(paramInfo);

        if (id.isEmpty())
            id = paramInfo.getParamID();

        entry->second = dynamic_cast<juce::AudioParameterFloat *>(apvts.getParameter(id));
    }

    // as the audio hears it, scaled by the start page where that applies
    if (auto *macro = dynamic_cast<MacroParam *>(entry->second))
        return macro->getScaled();

    return entry->second != nullptr ? entry->second->get() : 0.0f;
}

juce::AudioParameterChoice* Scope::choiceParam(const ParamIDs::ParameterInfo &id) const
{
    return dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter(id.getParamID()));
}

juce::AudioParameterChoice* Scope::choiceParamForSlot(SlotId slot) const
{
    return dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter(slot.type().getParamID()));
}
