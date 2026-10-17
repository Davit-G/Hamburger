#include "Scope.h"

Scope::Scope(juce::AudioProcessorValueTreeState& valueTree, ScopeDataCollector<float>& scopeDataCollector, ScopeContext &newScopeContext)
    : apvts(valueTree),
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

    bufferedFFTInput.resize(scope_constants::fftSize);
    fftHistory.resize(scope_constants::fftSize, 0.0f);

    lowFreqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisLowFreq.getParamID()));
    highFreqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisHighFreq.getParamID()));
    lowGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisLowGain.getParamID()));
    highGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::emphasisHighGain.getParamID()));
    postClipKneeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramIdFor(SlotId{ModuleId::postClip, 0}, ParamIDs::postClipKnee).getParamID()));

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
    imgG.fillAll(inOutBackground);
}

void Scope::mouseDown(const juce::MouseEvent &event)
{
    juce::ignoreUnused(event);
    this->viewSpectrum = !this->viewSpectrum;
}

void Scope::setFramesPerSecond(int framesPerSecond)
{
    jassert(framesPerSecond > 0 && framesPerSecond < 1000);
    startTimerHz(framesPerSecond);
}

// which view is on screen, each drawn in its own file, and the slot it reads its parameters from
void Scope::paint(juce::Graphics &g)
{
    const auto area = getLocalBounds();

    switch (scopeContext.getType()) {
        case ScopeContextType::LR_SCOPE:          drawLRView(g, area); break;
        case ScopeContextType::IN_OUT:            drawDistortionView(g, area, SlotId { ModuleId::main, 0 }); break;
        case ScopeContextType::SPECTRUM_EMPHASIS: drawEmphasisView(g, area); break;
        case ScopeContextType::CLIPPER:           drawClipperView(g, area, SlotId { ModuleId::postClip, 0 }); break;
        case ScopeContextType::COMPRESSION:       drawCompressorView(g, area, SlotId { ModuleId::dynamics, 0 }); break;
        case ScopeContextType::NOISE:             drawNoiseView(g, area, SlotId { ModuleId::module1, 0 }); break;
        default: break;
    }
}

void Scope::updateHopSize()
{
    const auto sampleRate = dataCollector.getSampleRate();

    // early exit if we don't need to make any changes!
    if (sampleRate <= 0.0 || juce::approximatelyEqual(sampleRate, preparedSampleRate)) return;
    
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

    updateHopSize(); // doesnt do anything in most cases, unless sample rate has changed

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

        hopsPopped++;

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

        if (spectrumView)
            updateSpectrum();
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

// one version of a method to get a parameter value, but this one is slot specific.
// in order to avoid doing an unnecessary iteration can you please feed the required slot to this function
float Scope::getValue(const ParamIDs::ParameterInfo &paramInfo, const SlotId slot) const
{
    auto *param = dynamic_cast<juce::AudioParameterFloat *>(apvts.getParameter(paramIdFor(slot, paramInfo).getParamID()));
    jassert(param != nullptr);

    return param != nullptr ? param->get() : 0.0f;
}

// another version of a method to get a parameter value
// in the case of no slot, we just use it's param id directly
// DONT USE IN PLACE OF PARAM THAT USES SLOTS
float Scope::getValue(const ParamIDs::ParameterInfo &paramInfo) const
{
    auto *param = dynamic_cast<juce::AudioParameterFloat *>(apvts.getParameter(paramInfo.getParamID()));
    jassert(param != nullptr);

    return param != nullptr ? param->get() : 0.0f;
}
