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
    MB_COMP, // three bands with boxes for ratio, threshold etc
    MS_COMP, // two bands similar to mb, mid and side
    STEREO_COMP, // two bands similar to mb, left and right
    TYPE_A, // four bands, the top one stacked on the one below it
    NOISE, // get a sine wave and apply the noise distortions onto them so we can see what they look like
};

// cause maybe we might need to sync across threads later?
// also could store data used for scope to draw things like compression curves, eq curves etc
class ScopeContext {
public:
    ScopeContext() {}
    ~ScopeContext() {}

    ScopeContextType getType();

    void setType(ScopeContextType newType);

private:
    ScopeContextType type = ScopeContextType::LR_SCOPE;
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

    float paramValue(const juce::ParameterID &id) const;
    juce::AudioParameterChoice* choiceParam(const juce::ParameterID &id) const;
    // black tab sized to its text, grown past the top or bottom edge so only the inner corners round
    void drawTabbedLabel(juce::Graphics &g, juce::Rectangle<float> cell, const juce::String &text,
                         juce::Justification justification, bool hangingFromTop);
    void drawDistortionAmount(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);

    juce::StringArray getDistortionHeaderLabels() const;
    juce::String getDistortionAmountLabel() const;
    juce::StringArray getNoiseHeaderLabels() const;
    juce::StringArray getCompHeaderLabels(const juce::ParameterID &thresholdID, bool withTilt) const;

    // shared by all three compressors, clamped so it can never invert the ratio ladder
    float compRatio() const;
    float bandLevel(LevelMeter &meter) const;

    void timerCallback() override;

    HamburgerLAF hamburgerLAF;

    static void plotStraightLine(const SampleType *data,
                     size_t numSamples,
                     juce::Graphics &g,
                     juce::Rectangle<SampleType> rect,
                     SampleType scaler = SampleType(1),
                     SampleType offset = SampleType(0));
};
