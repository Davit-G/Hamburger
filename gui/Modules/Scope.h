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

    double timeTillReset = 0.0; // when the user presses a knob it stays decayed for some amt of time
    static constexpr double waitTime = 2.0; // two seconds until the default view is returned

    ScopeContextType type = ScopeContextType::LR_SCOPE; // the current type
    static constexpr ScopeContextType defaultType = ScopeContextType::LR_SCOPE; // the default to set after time has elapsed
};

template <typename SampleType>
class Scope : public juce::Component,
              private juce::Timer
{
public:
    using Queue = AudioBufferQueue<SampleType>;
    Scope(juce::AudioProcessorValueTreeState& valueTree, ScopeDataCollector<SampleType>& scopeDataCollector, ScopeContext &newScopeContext);

    void mouseDown(const juce::MouseEvent &event) override;
    void setFramesPerSecond(int framesPerSecond);
    void paint(juce::Graphics &g) override;

    // single source of truth for the x axis, everything drawn over the spectrum has to go through this
    SampleType freqToX(double freq, SampleType w) const;

    float getBandResponse(double freq, double centerFreq, double gainDb);
    void drawResponseCurve(juce::Graphics &g, const SampleType w, const SampleType centerY, const SampleType maxHeight);
    
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
    ScopeDataCollector<SampleType>& dataCollector;

    // two hops each: the newest hop plus the one before it, so the trigger has somewhere to search
    std::vector<SampleType> sampleDataL;
    std::vector<SampleType> sampleDataR;
    std::vector<SampleType> triggerSignal;    // low passed copy of L, only ever used to find the trigger
    std::vector<SampleType> triggerDecimated; // the same copy at a fraction of the rate, to search
    std::vector<SampleType> triggerReference; // the shape locked onto last frame, to match against
    bool hasTriggerReference = false;
    size_t triggerOffset = 0;
    std::vector<SampleType> sampleDataPreDistortion;
    std::vector<SampleType> sampleDataPostDistortion;

    AudioBufferQueue<SampleType> bufferedFFTInput;
    std::vector<SampleType> fftHistory;
    std::array<SampleType, scope_constants::fftBins> averagedSpectrum{};
    int fftHistoryWritePosition = 0;

    std::array<SampleType, 2> originLineData = {SampleType(1), SampleType(1)};

    juce::dsp::FFT fft{static_cast<int>(std::log2(scope_constants::fftSize))};
    using WindowFun = juce::dsp::WindowingFunction<SampleType>;
    WindowFun windowFun{scope_constants::fftSize, WindowFun::hann};

    std::array<SampleType, scope_constants::fftInputSize> spectrumData{};
    std::array<SampleType, scope_constants::fftBins> spectrumTransformed{};

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
    void drawLRScope(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawInOut(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawInOutAxes(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void renderInOutFrame(bool stampNewTrace);
    void drawSpectrumEmphasis(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawClipper(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    // one cell per band of whichever compressor is on screen
    struct CompBand
    {
        float thresholdDb;
        float level;      // linear, straight off the band meter
        const char* name;
        bool stacked = false; // shares the previous band's cell instead of taking one of its own
        float gainOffsetDb = 0.0f; // what this band's makeup is trimmed by relative to the others
    };

    void drawCompBands(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect,
                       const CompBand *bands, int numBands, float ratio);
    void drawMBComp(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawMSComp(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawStereoComp(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawTypeAComp(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawTiledContextLabel(juce::Graphics &g, juce::Rectangle<int> area, const juce::String &text);
    void drawParamHeader(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect, const juce::StringArray &labels);

    // strip every view keeps clear at the top for its readouts
    SampleType headerHeight(juce::Rectangle<SampleType> scopeRect) const;

    float paramValue(const ParamIDs::ParameterInfo &paramInfo) const;


    juce::AudioParameterChoice* choiceParam(const ParamIDs::ParameterInfo &id) const;
    // black tab sized to its text, grown past the top or bottom edge so only the inner corners round
    void drawTabbedLabel(juce::Graphics &g, juce::Rectangle<float> cell, const juce::String &text,
                         juce::Justification justification, bool hangingFromTop);
    void drawDistortionAmount(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);

    juce::StringArray getDistortionHeaderLabels() const;
    juce::String getDistortionAmountLabel() const;
    juce::StringArray getNoiseHeaderLabels() const;
    juce::StringArray getCompHeaderLabels(const ParamIDs::ParameterInfo &thresholdParam, bool withTilt) const;

    // shared by all three compressors, clamped so it can never invert the ratio ladder
    float compRatio() const;
    float bandLevel(LevelMeter &meter) const;

    void timerCallback() override;

    static void plotStraightLine(const SampleType *data,
                     size_t numSamples,
                     juce::Graphics &g,
                     juce::Rectangle<SampleType> rect,
                     SampleType scaler = SampleType(1),
                     SampleType offset = SampleType(0));
};