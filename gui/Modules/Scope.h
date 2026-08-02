#pragma once

#include <memory>
#include <juce_dsp/juce_dsp.h>

enum ScopeContextType {
    LR_SCOPE, // by default
    IN_OUT, // input against output
    // SPECTRUM, // once button press happens?
    // SPECTRUM_EMPHASIS, // draw curves for emphasis eq
    
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
        resize(1024);
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
    float getReadableSpace() {
        lock.enterRead();

        int start1, size1, start2, size2;
        abstractFifo.prepareToRead((int) buffer.size(), start1, size1, start2, size2);

        lock.exitRead();

        return (float) (size1 + size2);
    }

    //==============================================================================
    // happens on audio thread, we dont need write locks here.
    // if we go over the size limit this could be dodgy?
    void push(const SampleType *dataToPush, size_t numSamples)
    {
        jassert(numSamples <= buffer.size());

        lock.enterWrite();

        int start1, size1, start2, size2;
        abstractFifo.prepareToWrite((int) numSamples, start1, size1, start2, size2);

        const auto totalSpace = juce::jmin<int>((int) numSamples, size1 + size2);
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
    }

    //==============================================================================
    // happens on ui thread
    void pop(SampleType *outputBuffer, size_t numSamples)
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
    }

private:
    //==============================================================================
    juce::AbstractFifo abstractFifo {1024};
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
        audioBufferQueueL.resize(spec.sampleRate / 60);
        audioBufferQueueR.resize(spec.sampleRate / 60);

        audioBufferQueuePreDistortion.resize(spec.sampleRate / 60);
        audioBufferQueuePostDistortion.resize(spec.sampleRate / 60);

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

        audioBufferQueuePreDistortion.push(inputBlock.getChannelPointer(0), inputBlock.getNumSamples());
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

        audioBufferQueuePostDistortion.push(inputBlock.getChannelPointer(0), inputBlock.getNumSamples());
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
    Scope(ScopeDataCollector<SampleType>& scopeDataCollector, ScopeContext &newScopeContext)
        : dataCollector(scopeDataCollector),
          scopeContext(newScopeContext)
    {   
        int fps = 60;
        sampleDataL.resize(44100 / fps);
        sampleDataR.resize(44100 / fps);
        sampleDataPreDistortion.resize(44100 / fps);
        sampleDataPostDistortion.resize(44100 / fps);
        setFramesPerSecond(fps);
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

                    g.setColour(juce::Colours::lime);
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
        }
    }

    //==============================================================================
    void resized() override {}

    bool viewSpectrum = false;

private:
    ScopeContext& scopeContext;
    ScopeDataCollector<SampleType>& dataCollector;

    std::vector<SampleType> sampleDataL;
    std::vector<SampleType> sampleDataR;
    std::vector<SampleType> sampleDataPreDistortion;
    std::vector<SampleType> sampleDataPostDistortion;

    std::array<SampleType, 2> originLineData = {SampleType(1), SampleType(1)};

    // juce::dsp::FFT fft{1024};
    // using WindowFun = juce::dsp::WindowingFunction<SampleType>;
    // WindowFun windowFun{(size_t)fft.getSize(), WindowFun::hann};

    // std::array<SampleType, 2 * Queue::bufferSize> spectrumData;
    // std::array<SampleType, 2 * Queue::bufferSize> scopeData;

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
        }

        if (preDist.getReadableSpace() >= sampleDataPreDistortion.size() && postDist.getReadableSpace() >= sampleDataPostDistortion.size()) {
            preDist.pop(sampleDataPreDistortion.data(), sampleDataPreDistortion.size());
            postDist.pop(sampleDataPostDistortion.data(), sampleDataPostDistortion.size());
        }

        // juce::FloatVectorOperations::copy(spectrumData.data(), sampleDataL.data(), (int)sampleDataL.size());

        // auto fftSize = (size_t)fft.getSize();

        // jassert(spectrumData.size() == 2 * fftSize);
        // windowFun.multiplyWithWindowingTable(spectrumData.data(), fftSize);
        // fft.performFrequencyOnlyForwardTransform(spectrumData.data());

        // static constexpr auto mindB = SampleType(-156);
        // static constexpr auto maxdB = SampleType(6);

        // for (int i = 0; i < spectrumData.size(); ++i) {
        //     auto skewedProportionX = 1.0f - std::exp (std::log (1.0f - (float) i / (float) spectrumData.size()) * 0.05f);
            
        //     auto newTing = skewedProportionX * spectrumData.size() * 0.5f;

        //     const float prev = floor(newTing);
        //     const float next = ceil(newTing);
        //     const float interp = newTing - prev;
        //     const float val = spectrumData[prev] * (1 - interp) + spectrumData[next] * interp;

        //     auto ting = juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(val) - juce::Decibels::gainToDecibels((SampleType)fftSize));
        //     scopeData[i] = juce::jmap(
        //         ting, 
        //         mindB, 
        //         maxdB, 
        //         SampleType(0), 
        //         SampleType(1)
        //         );
            
        // }

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