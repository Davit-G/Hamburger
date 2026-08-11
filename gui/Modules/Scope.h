#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "../../utils/Params.h"

namespace scope_constants
{
    static constexpr int defaultQueueSize = 2048;
    static constexpr int fftSize = 2048;
    static constexpr int fftInputSize = fftSize * 2;
    static constexpr int fftBins = fftSize / 2 + 1;
    static constexpr int defaultFrameRate = 60;
    static constexpr int defaultHopSize = 44100 / defaultFrameRate;
    static constexpr float spectrumSmoothing = 0.6f;
}

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

//==============================================================================
template <typename SampleType>
class AudioBufferQueue
{
public:
    AudioBufferQueue() {
        resize(scope_constants::defaultQueueSize);
    }

    void resize(size_t size) {
        lock.enterWrite();

        const auto capacity = juce::jmax<int>((int) size, 1);
        buffer.resize(capacity * 3); // give ourselves leeway if the ui thread is not consuming fast enough;
        abstractFifo.setTotalSize((int) buffer.size());
        abstractFifo.reset();

        lock.exitWrite();
    }

    // obtains the currently consumed space (the amount that can be currently read from the buffer)
    int getReadableSpace() {
        lock.enterRead();

        int start1, size1, start2, size2;
        abstractFifo.prepareToRead((int) buffer.size(), start1, size1, start2, size2);

        lock.exitRead();

        return size1 + size2;
    }

    //==============================================================================
    // happens on audio thread, we dont need write locks here.
    // if we go over the size limit this could be dodgy?
    // returns the number of samples written
    float push(const SampleType *dataToPush, size_t numSamples)
    {
        jassert(numSamples <= buffer.size());

        lock.enterWrite();

        const auto requiredSamples = juce::jmin<int>((int) numSamples, (int) buffer.size());

        int start1, size1, start2, size2;
        abstractFifo.prepareToWrite(requiredSamples, start1, size1, start2, size2);

        if (size1 + size2 < requiredSamples)
        {
            abstractFifo.reset();
            abstractFifo.prepareToWrite(requiredSamples, start1, size1, start2, size2);
        }

        const auto totalSpace = juce::jmin<int>(requiredSamples, size1 + size2);
        int samplesWritten = 0;

        if (size1 > 0 && samplesWritten < totalSpace)
        {
            const auto writeNow = juce::jmin<int>(totalSpace - samplesWritten, size1);
            juce::FloatVectorOperations::copy(buffer.data() + start1, dataToPush + samplesWritten, writeNow);
            samplesWritten += writeNow;
        }

        if (size2 > 0 && samplesWritten < totalSpace)
        {
            const auto writeNow = juce::jmin<int>(totalSpace - samplesWritten, size2);
            juce::FloatVectorOperations::copy(buffer.data() + start2, dataToPush + samplesWritten, writeNow);
            samplesWritten += writeNow;
        }

        abstractFifo.finishedWrite(samplesWritten);

        lock.exitWrite();

        return samplesWritten;
    }

    //==============================================================================
    // happens on ui thread
    float pop(SampleType *outputBuffer, size_t numSamples)
    {
        lock.enterWrite();

        int start1, size1, start2, size2;
        abstractFifo.prepareToRead((int) numSamples, start1, size1, start2, size2);
        
        const auto totalSpace = juce::jmin<int>((int) numSamples, size1 + size2);
        int samplesRead = 0;

        if (size1 > 0 && samplesRead < totalSpace)
        {
            const auto readNow = juce::jmin<int>(totalSpace - samplesRead, size1);
            juce::FloatVectorOperations::copy(outputBuffer + samplesRead, buffer.data() + start1, readNow);
            samplesRead += readNow;
        }

        if (size2 > 0 && samplesRead < totalSpace)
        {
            const auto readNow = juce::jmin<int>(totalSpace - samplesRead, size2);
            juce::FloatVectorOperations::copy(outputBuffer + samplesRead, buffer.data() + start2, readNow);
            samplesRead += readNow;
        }

        abstractFifo.finishedRead(samplesRead);

        lock.exitWrite();
        return samplesRead;
    }

    // reset fifo structure to start from the start without reallocating
    void reset() {
        abstractFifo.reset();
    }

private:
    //==============================================================================
    juce::AbstractFifo abstractFifo {scope_constants::defaultQueueSize};
    juce::ReadWriteLock lock; // using a lock to block the ui thread when a resize occurs
    std::vector<SampleType> buffer;
};

//==============================================================================
template <typename SampleType>
class ScopeDataCollector
{
public:
    //==============================================================================
    ScopeDataCollector()
    {}

    void prepare(juce::dsp::ProcessSpec& spec) {
        audioBufferQueueL.resize(spec.sampleRate / 30);
        audioBufferQueueR.resize(spec.sampleRate / 30);

        audioBufferQueuePreDistortion.resize(spec.sampleRate / 30);
        audioBufferQueuePostDistortion.resize(spec.sampleRate / 30);

        const int maxScopeSamples = juce::jmax<int>((int) spec.maximumBlockSize, 1) * 16;
        preDistScratchBuffer.setSize(1, maxScopeSamples, false, false);
        postDistScratchBuffer.setSize(1, maxScopeSamples, false, false);
    }

    //==============================================================================
    void process(const SampleType *dataL, const SampleType *dataR, size_t numSamples)
    {
        // size_t index = 0;

        audioBufferQueueL.push(dataL, numSamples);
        audioBufferQueueR.push(dataR, numSamples);

        // if (state == State::waitingForTrigger)
        // {
        //     while (index++ < numSamples)
        //     {
        //         auto currentSampleL = *dataL++;
        //         auto currentSampleR = *dataR++;

        //         auto currentSample = currentSampleL + currentSampleR;

        //         if (currentSample >= triggerLevel && prevSample < triggerLevel)
        //         {
        //             numCollected = 0;
        //             state = State::collecting;
        //             break;
        //         }

        //         prevSample = currentSample;
        //     }
        // }

        // if (state == State::collecting)
        // {

            // while (index++ < numSamples)
            // {
            //     bufferL[numCollected] = *dataL++;
            //     bufferR[numCollected++] = *dataR++;

            //     if (numCollected == bufferL.size())
            //     {
            //         audioBufferQueueL.push(bufferL.data(), bufferL.size());
            //         audioBufferQueueR.push(bufferR.data(), bufferR.size());

            //         state = State::waitingForTrigger;
            //         prevSample = SampleType(100);
            //         numCollected = 0;
            //         break;
            //     }
            // }

        
    }

    void capturePreDistortion(const SampleType *dataL, size_t numSamples, int oversamplingFactor) {
        if (oversamplingFactor <= 1)
        {
            audioBufferQueuePreDistortion.push(dataL, numSamples);
            return;
        }

        prepareOversampler(preDistOversampling, oversamplingFactor, numSamples);

        if (preDistScratchBuffer.getNumSamples() < (int) numSamples)
            preDistScratchBuffer.setSize(1, (int) numSamples, false, false);

        preDistScratchBuffer.clear();
        preDistScratchBuffer.copyFrom(0, 0, dataL, (int) numSamples);

        juce::dsp::AudioBlock<SampleType> inputBlock(preDistScratchBuffer.getArrayOfWritePointers(), 1, (int) numSamples);
        preDistOversampling->processSamplesDown(inputBlock);

        samplesReadPre = audioBufferQueuePreDistortion.push(inputBlock.getChannelPointer(0), inputBlock.getNumSamples());
    }

    void capturePostDistortion(const SampleType *dataL, size_t numSamples, int oversamplingFactor) {
        if (oversamplingFactor <= 1)
        {
            audioBufferQueuePostDistortion.push(dataL, numSamples);
            return;
        }

        prepareOversampler(postDistOversampling, oversamplingFactor, numSamples);

        if (postDistScratchBuffer.getNumSamples() < (int) numSamples)
            postDistScratchBuffer.setSize(1, (int) numSamples, false, false);

        postDistScratchBuffer.clear();
        postDistScratchBuffer.copyFrom(0, 0, dataL, (int) numSamples);

        juce::dsp::AudioBlock<SampleType> inputBlock(postDistScratchBuffer.getArrayOfWritePointers(), 1, (int) numSamples);
        postDistOversampling->processSamplesDown(inputBlock);

        samplesReadPost = audioBufferQueuePostDistortion.push(inputBlock.getChannelPointer(0), inputBlock.getNumSamples());

        if (samplesReadPre != samplesReadPost) {
            // mismatch, we have to re-align both queues without reallocating
            audioBufferQueuePreDistortion.reset();
            audioBufferQueuePostDistortion.reset();
        }
    }

    //==============================================================================
    AudioBufferQueue<SampleType> audioBufferQueueL;
    AudioBufferQueue<SampleType> audioBufferQueueR;

    AudioBufferQueue<SampleType> audioBufferQueuePreDistortion;
    AudioBufferQueue<SampleType> audioBufferQueuePostDistortion;
private:
    void prepareOversampler(std::unique_ptr<juce::dsp::Oversampling<SampleType>>& oversampler,
                            int oversamplingFactor,
                            size_t numSamples)
    {
        if (oversampler == nullptr || oversampler->getOversamplingFactor() != oversamplingFactor)
        {
            oversampler = std::make_unique<juce::dsp::Oversampling<SampleType>>(
                1,
                oversamplingFactor,
                juce::dsp::Oversampling<SampleType>::filterHalfBandPolyphaseIIR,
                true);
        }

        oversampler->initProcessing((size_t) juce::jmax<int>((int) numSamples, 1));
    }

    std::unique_ptr<juce::dsp::Oversampling<SampleType>> preDistOversampling;
    std::unique_ptr<juce::dsp::Oversampling<SampleType>> postDistOversampling;
    juce::AudioBuffer<SampleType> preDistScratchBuffer;
    juce::AudioBuffer<SampleType> postDistScratchBuffer;

    float samplesReadPre, samplesReadPost;

    // size_t numCollected;
    // SampleType prevSample = SampleType(100);

    // static constexpr auto triggerLevel = SampleType(0.001);

    // enum class State
    // {
    //     waitingForTrigger,
    //     collecting
    // } state{State::waitingForTrigger};
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
                    const auto logMax = std::log10(binCount);
                    constexpr auto tiltDbPerOctave = SampleType(4.5);
                    constexpr auto mindB = SampleType(-70);
                    constexpr auto maxdB = SampleType(18);

                    juce::Path spectrumPath;
                    for (size_t i = 0; i < spectrumTransformed.size(); ++i)
                    {
                        const auto x = (binCount > 1.0)
                            ? static_cast<SampleType>((std::log10(static_cast<double>(i + 1)) / logMax) * static_cast<double>(w))
                            : SampleType(0);

                        const auto dbValue = juce::jmap(spectrumTransformed[i], SampleType(0), SampleType(1), mindB, maxdB);
                        const auto octaveIndex = std::max(SampleType(0), std::log2(static_cast<SampleType>(binCount) / static_cast<SampleType>(i + 1)));
                        const auto tiltDb = -tiltDbPerOctave * octaveIndex;
                        const auto adjustedDb = dbValue + tiltDb;
                        const auto magnitude = juce::jlimit(SampleType(0), SampleType(1), juce::jmap(
                            juce::jlimit(mindB, maxdB, adjustedDb),
                            mindB,
                            maxdB,
                            SampleType(0),
                            SampleType(1)));
                        const auto y = centerY - magnitude * maxHeight;

                        if (i == 0)
                            spectrumPath.startNewSubPath(x, y);
                        else
                            spectrumPath.lineTo(x, y);
                    }

                    g.strokePath(spectrumPath, juce::PathStrokeType(2.f));

                    
                    drawResponseCurve(g, w, centerY, maxHeight, juce::Colours::grey);

                    // drawResponseCurve(juce::Colours::grey, true);
                    // drawResponseCurve(juce::Colours::white, false); // drawing white over the top of this
                }

                break;
            }
            case ScopeContextType::CLIPPER: {
                

                break;
            }
        }
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

    void drawResponseCurve(juce::Graphics &g, const float w, const float centerY, const float maxHeight, const juce::Colour &colour)
    {
        const auto lowFreq = lowFreqParam->get();
        const auto highFreq = highFreqParam->get();
        const auto lowGainDb = lowGainParam->get();
        const auto highGainDb = highGainParam->get();
                        
        juce::Path eqPath, eqInversePath;
        const auto minFreq = 20.0;
        const auto maxFreq = 20000.0;
        const auto leftAnchorFreq = 20.0;
        const auto bellXScale = std::log10(maxFreq / minFreq);
        const auto spectrumLeftEdgeFreq = 20.0;

        for (int i = 0; i <= 220; ++i)
        {
            const auto logT = static_cast<double>(i) / 220.0;
            const auto logFreq = std::log10(leftAnchorFreq) + logT * bellXScale;
            const auto freq = std::pow(10.0, logFreq);
            const auto lowResponse = getBandResponse(freq, lowFreq, lowGainDb);
            const auto highResponse = getBandResponse(freq, highFreq, highGainDb);
            const auto responseToDraw = juce::jlimit(0.0, 1.0, 0.5 + (lowResponse - 0.5) + (highResponse - 0.5));

            const auto x = juce::jmap(logFreq, std::log10(spectrumLeftEdgeFreq), std::log10(maxFreq), 0.0, static_cast<double>(w));
            const auto clampedX = juce::jlimit(0.0, static_cast<double>(w), x);
            const auto y = centerY - responseToDraw * maxHeight;
            const auto inverseY = centerY + responseToDraw * maxHeight;

            if (!std::isfinite(x) || !std::isfinite(y))
                continue;

            if (i == 0) {
                eqPath.startNewSubPath(static_cast<float>(clampedX), static_cast<float>(y));
                eqInversePath.startNewSubPath(static_cast<float>(clampedX), static_cast<float>(inverseY));
            }
            else {
                eqPath.lineTo(static_cast<float>(clampedX), static_cast<float>(y));
                eqInversePath.startNewSubPath(static_cast<float>(clampedX), static_cast<float>(inverseY));
            }
        }

        if (eqPath.isEmpty())
            return;

        g.setColour(colour);
        g.strokePath(eqPath, juce::PathStrokeType(2.0f));
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