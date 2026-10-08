#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "AudioBufferQueue.h"
#include "ScopeDataCollector.h"
#include "ScopeConstants.h"

#include "../LookAndFeel/HamburgerLAF.h"
#include "../../utils/Params.h"

#include "../../dsp/NoiseDistortions.h"

enum ScopeContextType {
    LR_SCOPE, // by default
    IN_OUT, // input against output
    // SPECTRUM, // once button press happens?
    SPECTRUM_EMPHASIS, // draw curves for emphasis eq
    CLIPPER, // clipping curve + waveform
    COMPRESSION, // encapsulates all of the compressors. only display the active compressor at any given time
    NOISE, // get a sine wave and apply the noise distortions onto them so we can see what they look like
};

// cause maybe we might need to sync across threads later?
// also could store data used for scope to draw things like compression curves, eq curves etc
class ScopeContext {
public:
    ScopeContext() {
        lastTime = juce::Time::getMillisecondCounterHiRes();
    }
    ~ScopeContext() {}

    ScopeContextType getType() { return type; }

    void setFocus(SlotId slot) { focus.store(((int) slot.module << 8) | (slot.sub & 0xff)); }

    SlotId getFocus() const
    {
        const auto packed = focus.load();
        return { (ModuleId) (packed >> 8), packed & 0xff };
    }

    // locks the currently displayed scope and blocks decay
    void setLocked(bool shouldLock)
    {
        locked = shouldLock;

        if (locked)
        {
            decaying = false;
            timeTillReset = 0.0;
        }
        else
        {
            startDecaying();
        }
    }

    void toggleLocked()
    {
        setLocked(!locked);
    }

    // startDecaying dictates if the scope context should start the timer for the visual to change
    // also will immediately stop decaying if the type has been set
    void setType(ScopeContextType newType, bool force = false) {
        if (locked && !force)
            return;
        type = newType;
        decaying = false;
    }

    void startDecaying() {
        if (locked)
            return;
        decaying = true;
        timeTillReset = waitTime;
    }

    // call in frame rendering, it will keep track of it's own time
    void updateFrame() {
        if (!decaying || locked)
            return;
        if (timeTillReset > 0.0f) {
            auto now = juce::Time::getMillisecondCounterHiRes();
            auto elapsed = lastTime > 0.0 ? juce::jlimit(0.0, 1.0, (now - lastTime) * 0.001) : 0.0;
    
            timeTillReset -= elapsed;
            lastTime = now;
        } else {
            decaying = false;
            timeTillReset = 0.0f;
    
            type = defaultType;
        }
    }

private:
    double lastTime = 0.0; // last measured time from millisecond counter
    bool decaying = false;
    bool locked = false;

    std::atomic<int> focus { (int) ModuleId::main << 8 };

    double timeTillReset = 0.0; // when the user presses a knob it stays decayed for some amt of time
    static constexpr double waitTime = 2.0; // two seconds until the default view is returned

    ScopeContextType type = ScopeContextType::LR_SCOPE; // the current type
    static constexpr ScopeContextType defaultType = ScopeContextType::LR_SCOPE; // the default to set after time has elapsed
};

class Scope : public juce::Component,
              private juce::Timer
{
public:
    using Queue = AudioBufferQueue<float>;
    Scope(juce::AudioProcessorValueTreeState& valueTree, ScopeDataCollector<float>& scopeDataCollector, ScopeContext &newScopeContext);

    void mouseDown(const juce::MouseEvent &event) override;
    void setFramesPerSecond(int framesPerSecond);
    void paint(juce::Graphics &g) override;

    // single source of truth for the x axis, everything drawn over the spectrum has to go through this
    float freqToX(double freq, float w) const;

    float getBandResponse(double freq, double centerFreq, double gainDb);
    void drawResponseCurve(juce::Graphics &g, const float w, const float centerY, const float maxHeight);
    
    void resized() override;

    bool viewSpectrum = false;

private:
    juce::AudioParameterFloat* lowFreqParam;
    juce::AudioParameterFloat* highFreqParam;
    juce::AudioParameterFloat* lowGainParam;
    juce::AudioParameterFloat* highGainParam;
    juce::AudioParameterFloat* postClipKneeParam;
    juce::AudioParameterChoice* compressionType;

    juce::AudioProcessorValueTreeState& apvts;

    ScopeContext& scopeContext;
    ScopeDataCollector<float>& dataCollector;

    // two hops each: the newest hop plus the one before it, so the trigger has somewhere to search
    std::vector<float> sampleDataL;
    std::vector<float> sampleDataR;
    std::vector<float> triggerSignal;    // low passed copy of L, only ever used to find the trigger
    std::vector<float> triggerDecimated; // the same copy at a fraction of the rate, to search
    std::vector<float> triggerReference; // the shape locked onto last frame, to match against
    bool hasTriggerReference = false;
    size_t triggerOffset = 0;
    std::vector<float> sampleDataPreDistortion;
    std::vector<float> sampleDataPostDistortion;

    AudioBufferQueue<float> bufferedFFTInput;
    std::vector<float> fftHistory;
    std::array<float, scope_constants::fftBins> averagedSpectrum{};
    int fftHistoryWritePosition = 0;

    std::array<float, 2> originLineData = {1.0f, 1.0f};

    juce::dsp::FFT fft{static_cast<int>(std::log2(scope_constants::fftSize))};
    using WindowFun = juce::dsp::WindowingFunction<float>;
    WindowFun windowFun{scope_constants::fftSize, WindowFun::hann};

    std::array<float, scope_constants::fftInputSize> spectrumData{};
    std::array<float, scope_constants::fftBins> spectrumTransformed{};

    juce::Image inOutFB;
    juce::Colour inOutBackground {juce::Colours::black};


    // for analysing what the result of noise distortion is, we store a copy of the DSP so we can operate on it!
    NoiseDistortions noiseDist;
    juce::AudioBuffer<float> noiseDistBuf;


    float inOutFade = 0.13f; // how much background is mixed in per frame, higher = shorter trails
    float inOutRenderScale = 0.98f;

    size_t hopSize = (size_t) scope_constants::defaultHopSize;
    double preparedSampleRate = 0.0;

    void updateHopSize();
    void updateTriggerOffset();
    void drawLRScope(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawInOut(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawInOutAxes(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void renderInOutFrame(bool stampNewTrace);
    void drawSpectrumEmphasis(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawClipper(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    // one cell per band of whichever compressor is on screen
    struct CompBand
    {
        float thresholdDb;
        float level;      // linear, straight off the band meter
        const char* name;
        bool stacked = false; // shares the previous band's cell instead of taking one of its own
        float gainOffsetDb = 0.0f; // what this band's makeup is trimmed by relative to the others
    };

    void drawCompBands(juce::Graphics &g, juce::Rectangle<float> scopeRect,
                       const CompBand *bands, int numBands, float ratio);
    void drawMBComp(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawMSComp(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawStereoComp(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawTypeAComp(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawTiledContextLabel(juce::Graphics &g, juce::Rectangle<int> area, const juce::String &text);
    void drawParamHeader(juce::Graphics &g, juce::Rectangle<float> scopeRect, const juce::StringArray &labels);

    // strip every view keeps clear at the top for its readouts
    float headerHeight(juce::Rectangle<float> scopeRect) const;

    // a slot's copy of the parameter where it has one, otherwise the fixed parameter by its own id
    float paramValue(const ParamIDs::ParameterInfo &paramInfo, SlotId slot) const;
    float paramValue(const ParamIDs::ParameterInfo &paramInfo) const;


    juce::AudioParameterChoice* choiceParam(const ParamIDs::ParameterInfo &id) const;
    juce::AudioParameterChoice* choiceParamForSlot(SlotId slot) const;

    
    // black tab sized to its text, grown past the top or bottom edge so only the inner corners round
    void drawTabbedLabel(juce::Graphics &g, juce::Rectangle<float> cell, const juce::String &text,
                         juce::Justification justification, bool hangingFromTop);
    void drawDistortionAmount(juce::Graphics &g, juce::Rectangle<float> scopeRect);

    juce::StringArray getDistortionHeaderLabels() const;
    juce::String getDistortionAmountLabel() const;
    juce::StringArray getNoiseHeaderLabels() const;
    juce::StringArray getCompHeaderLabels(const ParamIDs::ParameterInfo &thresholdParam, bool withTilt) const;

    // shared by all three compressors, clamped so it can never invert the ratio ladder
    float compRatio() const;
    float bandLevel(LevelMeter &meter) const;

    void timerCallback() override;

    static void plotStraightLine(const float *data,
                     size_t numSamples,
                     juce::Graphics &g,
                     juce::Rectangle<float> rect,
                     float scaler = 1.0f,
                     float offset = 0.0f);
};