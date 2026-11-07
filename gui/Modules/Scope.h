#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <map>
#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "AudioBufferQueue.h"
#include "ScopeDataCollector.h"
#include "ScopeConstants.h"
#include "SpectrumAnalyser.h"

#include "../LookAndFeel/HamburgerLAF.h"
#include "../LookAndFeel/Theme.h"
#include "../../utils/Params.h"

#include "../../dsp/NoiseDistortions.h"

enum ScopeContextType {
    LR_SCOPE, // by default
    IN_OUT, // input against output
    WAVESHAPE, // input against output, over the curve the waveshape distortion is set to
    // SPECTRUM, // once button press happens?
    SPECTRUM_EMPHASIS, // draw curves for emphasis eq
    SPECTRUM_TILT, // the emphasis tilt, before and after the distortion
    CLIPPER, // clipping curve + waveform
    COMPRESSION, // encapsulates all of the compressors. only display the active compressor at any given time
    NOISE, // get a sine wave and apply the noise distortions onto them so we can see what they look like
    MODULATOR, // a mod source's output over the last couple of seconds
    WATERMARK, // nothing but the watermark, for theming it
};

// as the scope's right click menu lists them
inline const char* const scopeContextNames[] { "Stereo", "In / Out", "Waveshape", "Emphasis", "Tilt", "Clipper", "Compressor", "Noise", "Modulation", "Watermark" };
static_assert(std::size(scopeContextNames) == ScopeContextType::WATERMARK + 1);

class ModMatrix;

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

    // the mod source the modulator view draws, or with a connection, that source as the connection shapes it
    void setModulatorSource(int source, int connection = -1)
    {
        modulatorSource.store(source);
        modulatorConnection.store(connection);
    }

    int getModulatorSource() const { return modulatorSource.load(); }
    int getModulatorConnection() const { return modulatorConnection.load(); }

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

    bool isLocked() const { return locked; }

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

    std::atomic<int> focus { (int) ModuleId::main << 8 };
    std::atomic<int> modulatorSource { 0 }, modulatorConnection { -1 };

    ScopeContextType type = ScopeContextType::LR_SCOPE; // the current type
    static constexpr ScopeContextType defaultType = ScopeContextType::LR_SCOPE; // the default to set after time has elapsed
};

// the waveshape curve the scope draws, kept between frames so its table is only rebuilt when the curve changes
struct WaveshapeCurveCache;

class Scope : public juce::Component,
              private juce::Timer
{
public:
    using Queue = AudioBufferQueue<float>;
    Scope(juce::AudioProcessorValueTreeState& valueTree, ScopeDataCollector<float>& scopeDataCollector, ScopeContext &newScopeContext, ModMatrix& modMatrix);

    void setFramesPerSecond(int framesPerSecond);
    void paint(juce::Graphics &g) override;

    // single source of truth for the x axis, everything drawn over the spectrum has to go through this
    float freqToX(double freq, float w) const;

    float getBandResponse(double freq, double centerFreq, double gainDb);
    void drawResponseCurve(juce::Graphics &g, const float w, const float centerY, const float maxHeight);
    
    void resized() override;
    void lookAndFeelChanged() override;


private:
    juce::AudioParameterFloat* lowFreqParam;
    juce::AudioParameterFloat* highFreqParam;
    MacroParam* lowGainParam;
    MacroParam* highGainParam;
    MacroParam* tiltParam;
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

    SpectrumAnalyser spectrum;

    std::array<float, 2> originLineData = {1.0f, 1.0f};

    juce::Image inOutFB;
    juce::Colour trailBackground;


    // for analysing what the result of noise distortion is, we store a copy of the DSP so we can operate on it!
    NoiseDistortions noiseDist;
    juce::AudioBuffer<float> noiseDistBuf;


    float inOutFade = 0.13f; // how much background is mixed in per frame, higher = shorter trails
    float inOutRenderScale = 0.98f;

    size_t hopSize = (size_t) scope_constants::defaultHopSize;
    double preparedSampleRate = 0.0;

    void updateHopSize();

    // one per view, each in its own file
    void drawLRView(juce::Graphics &g, juce::Rectangle<int> area);
    void drawDistortionView(juce::Graphics &g, juce::Rectangle<int> area);
    void drawEmphasisView(juce::Graphics &g, juce::Rectangle<int> area);
    void drawTiltView(juce::Graphics &g, juce::Rectangle<int> area);
    void drawClipperView(juce::Graphics &g, juce::Rectangle<int> area);
    void drawCompressorView(juce::Graphics &g, juce::Rectangle<int> area);
    void drawNoiseView(juce::Graphics &g, juce::Rectangle<int> area);
    void drawModulatorView(juce::Graphics &g, juce::Rectangle<int> area);

    // every mod source's last couple of seconds, sampled once a frame whichever view is up, so switching to one has it all
    // ponytail: once a frame, an LFO faster than about 30Hz aliases into a slower wave here. take each chunk's value from the audio thread if that matters
    void recordModulators();
    ModMatrix& matrix;
    std::array<std::array<float, 120>, 8> modulatorHistory {};
    std::array<float, 120> connectionHistory {}; // only the connection being shown, started over when it changes
    int recordedConnection = -1;
    size_t modulatorHistoryNext = 0;

    void updateTriggerOffset();
    void drawLRScope(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawInOut(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void renderInOutFrame(bool stampNewTrace);
    void drawSpectrumEmphasis(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    void drawSpectrumTilt(juce::Graphics &g, juce::Rectangle<float> scopeRect);
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
    void drawOptoComp(juce::Graphics &g, juce::Rectangle<float> scopeRect, bool multiband);
    void drawTransient(juce::Graphics &g, juce::Rectangle<float> scopeRect, const juce::StringArray &bandNames);
    void drawTiledContextLabel(juce::Graphics &g, juce::Rectangle<int> area, const juce::String &text);
    void drawParamHeader(juce::Graphics &g, juce::Rectangle<float> scopeRect, const juce::StringArray &labels);

    // strip every view keeps clear at the top for its readouts
    float headerHeight(juce::Rectangle<float> scopeRect) const;

    float paramValue(const ParamIDs::ParameterInfo &paramInfo) const;
    
    mutable std::map<const ParamIDs::ParameterInfo *, juce::AudioParameterFloat *> paramCache;

    // the transient shaper's gain per band as drawn, easing back once it's been furthest
    std::array<float, 3> shownTransientDb {};


    juce::AudioParameterChoice* choiceParam(const ParamIDs::ParameterInfo &id) const;
    juce::AudioParameterChoice* choiceParamForSlot(SlotId slot) const;
    // black tab sized to its text, grown past the top or bottom edge so only the inner corners round
    void drawTabbedLabel(juce::Graphics &g, juce::Rectangle<float> cell, const juce::String &text,
                         juce::Justification justification, bool hangingFromTop);
    void drawDistortionAmount(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    
    bool focusIsWaveshape() const;
    void drawWaveshapeCurve(juce::Graphics &g, juce::Rectangle<float> scopeRect);
    std::shared_ptr<WaveshapeCurveCache> waveshapeCurve;

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