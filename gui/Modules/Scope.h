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
    COMP, // compression knee + level + ratio
    MB_COMP, // three bands with boxes for ratio, threshold etc
    MS_COMP, // two bands similar to mb
    TYPE_A, 
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

    std::vector<SampleType> sampleDataL;
    std::vector<SampleType> sampleDataR;
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
    float inOutRenderScale = 1.0f;

    void drawLRScope(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawInOut(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawInOutAxes(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void renderInOutFrame(bool stampNewTrace);
    void drawSpectrumEmphasis(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);
    void drawClipper(juce::Graphics &g, juce::Rectangle<SampleType> scopeRect);

    void timerCallback() override;

    HamburgerLAF hamburgerLAF;

    static void plotStraightLine(const SampleType *data,
                     size_t numSamples,
                     juce::Graphics &g,
                     juce::Rectangle<SampleType> rect,
                     SampleType scaler = SampleType(1),
                     SampleType offset = SampleType(0));
};
