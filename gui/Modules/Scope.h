#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "ScopeDataCollector.h"
#include "ScopeConstants.h"

#include "../../utils/Params.h"

enum ScopeContextType {
    LR_SCOPE, // by default
    IN_OUT, // input against output
    // SPECTRUM, // once button press happens?
    SPECTRUM_EMPHASIS, // draw curves for emphasis eq
    CLIPPER, // clipping curve + waveform
    COMP, // compression knee + level + ratio
    MB_COMP,
    MS_COMP,
};

// cause maybe we might need to sync across threads later?
// also could store data used for scope to draw things like compression curves, eq curves etc
class ScopeContext {
public:
    ScopeContext() {}
    ~ScopeContext() {}

    ScopeContextType getType() {
        return type;
    }

    void setType(ScopeContextType newType) {
        type = newType;
    }

private:
    ScopeContextType type = ScopeContextType::LR_SCOPE;



};

template <typename SampleType>
class Scope : public juce::Component,
              private juce::Timer
{
public:
    using Queue = AudioBufferQueue<SampleType>;

    //==============================================================================
    Scope(juce::AudioProcessorValueTreeState& valueTree, ScopeDataCollector<SampleType>& scopeDataCollector, ScopeContext &newScopeContext)
        : apvts(valueTree), 
        dataCollector(scopeDataCollector),
          scopeContext(newScopeContext)
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
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        juce::ignoreUnused(event);
        this->viewSpectrum = !this->viewSpectrum;
    }

    //==============================================================================
    void setFramesPerSecond(int framesPerSecond)
    {
        jassert(framesPerSecond > 0 && framesPerSecond < 1000);
        startTimerHz(framesPerSecond);
    }

    //==============================================================================
    void paint(juce::Graphics &g) override
    {
        auto area = getLocalBounds();
        auto h = (SampleType)area.getHeight();
        auto w = (SampleType)area.getWidth();

        auto scopeRect = juce::Rectangle<SampleType>{SampleType(0), SampleType(0), w, h};

        auto currentType = scopeContext.getType();

        switch (currentType) {
            case ScopeContextType::LR_SCOPE: {
                g.setColour(juce::Colours::grey);
                plotStraightLine(originLineData.data(), 2, g, scopeRect, SampleType(0.4), h / 2);
                plotStraightLine(originLineData.data(), 2, g, scopeRect, SampleType(-0.4), h / 2);
        
        
                g.setColour(juce::Colours::yellow);
                plotStraightLine(sampleDataL.data(), sampleDataL.size(), g, scopeRect, SampleType(0.4), h / 2);
                g.setColour(juce::Colours::lime);
                plotStraightLine(sampleDataR.data(), sampleDataR.size(), g, scopeRect, SampleType(0.4), h / 2);
                
                break;
            }
            case ScopeContextType::IN_OUT: {
                if (sampleDataPreDistortion.size() > 1 && sampleDataPostDistortion.size() > 1)
                {
                    const auto centerX = w * SampleType(0.5);
                    const auto centerY = h * SampleType(0.5);
                    const auto halfW = h * 0.95;
                    const auto halfH = h * SampleType(0.46);
                    
                    g.setColour(juce::Colours::grey);
                    g.drawLine(centerX, SampleType(0), centerX, h);
                    g.drawLine(SampleType(0), centerY, w, centerY);

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

                break;
            }
            case ScopeContextType::SPECTRUM_EMPHASIS: {
                if (spectrumTransformed.size() > 1)
                {
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

                break;
            }
            case ScopeContextType::CLIPPER: {
                

                break;
            }
        }
    }

    // single source of truth for the x axis, everything drawn over the spectrum has to go through this
    SampleType freqToX(double freq, SampleType w) const
    {
        const auto clamped = juce::jlimit(scope_constants::minDrawFreq, scope_constants::maxDrawFreq, freq);

        return static_cast<SampleType>(juce::jmap(std::log10(clamped),
                                                  std::log10(scope_constants::minDrawFreq),
                                                  std::log10(scope_constants::maxDrawFreq),
                                                  0.0,
                                                  static_cast<double>(w)));
    }

    float getBandResponse(double freq, double centerFreq, double gainDb)
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

    void drawResponseCurve(juce::Graphics &g, const SampleType w, const SampleType centerY, const SampleType maxHeight)
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

        g.setColour(juce::Colours::white);
        g.strokePath(eqPath, juce::PathStrokeType(2.0f));
            
        g.setColour(juce::Colours::grey);
        g.strokePath(eqInversePath, juce::PathStrokeType(2.0f));
    }

    //==============================================================================
    void resized() override {}

    bool viewSpectrum = false;

private:
    juce::AudioParameterFloat* lowFreqParam;
    juce::AudioParameterFloat* highFreqParam;
    juce::AudioParameterFloat* lowGainParam;
    juce::AudioParameterFloat* highGainParam;

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

    //==============================================================================
    void timerCallback() override
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

        if (preDist.getReadableSpace() >= sampleDataPreDistortion.size() && postDist.getReadableSpace() >= sampleDataPostDistortion.size()) {
            preDist.pop(sampleDataPreDistortion.data(), sampleDataPreDistortion.size());
            postDist.pop(sampleDataPostDistortion.data(), sampleDataPostDistortion.size());
        }



        repaint(getLocalBounds());
    }

    //==============================================================================
    static void plotStraightLine(const SampleType *data,
                     size_t numSamples,
                     juce::Graphics &g,
                     juce::Rectangle<SampleType> rect,
                     SampleType scaler = SampleType(1),
                     SampleType offset = SampleType(0))
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
};