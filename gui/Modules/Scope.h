#pragma once

//==============================================================================
template <typename SampleType>
class AudioBufferQueue
{
public:
    //==============================================================================
    static constexpr size_t order = 10;
    static constexpr size_t bufferSize = 1U << order;
    static constexpr size_t numBuffers = 5;

    //==============================================================================
    void push(const SampleType *dataToPush, size_t numSamples)
    {
        jassert(numSamples <= bufferSize);

        int start1, size1, start2, size2;
        abstractFifo.prepareToWrite(1, start1, size1, start2, size2);

        jassert(size1 <= 1);
        jassert(size2 == 0);

        if (size1 > 0)
            juce::FloatVectorOperations::copy(buffers[(size_t)start1].data(), dataToPush, (int)juce::jmin(bufferSize, numSamples));

        abstractFifo.finishedWrite(size1);
    }

    //==============================================================================
    void pop(SampleType *outputBuffer)
    {
        int start1, size1, start2, size2;
        abstractFifo.prepareToRead(1, start1, size1, start2, size2);

        jassert(size1 <= 1);
        jassert(size2 == 0);

        if (size1 > 0)
            juce::FloatVectorOperations::copy(outputBuffer, buffers[(size_t)start1].data(), (int)bufferSize);

        abstractFifo.finishedRead(size1);
    }

private:
    //==============================================================================
    juce::AbstractFifo abstractFifo{numBuffers};
    std::array<std::array<SampleType, bufferSize>, numBuffers> buffers;
};

//==============================================================================
template <typename SampleType>
class ScopeDataCollector
{
public:
    //==============================================================================
    ScopeDataCollector(AudioBufferQueue<SampleType> &queueToUseL, AudioBufferQueue<SampleType> &queueToUseR)
        : audioBufferQueueL(queueToUseL)
        , audioBufferQueueR(queueToUseR)
    {
    }

    //==============================================================================
    void process(const SampleType *dataL, const SampleType *dataR, size_t numSamples)
    {
        size_t index = 0;

        if (state == State::waitingForTrigger)
        {
            while (index++ < numSamples)
            {
                auto currentSampleL = *dataL++;
                auto currentSampleR = *dataR++;

                auto currentSample = currentSampleL + currentSampleR;

                if (currentSample >= triggerLevel && prevSample < triggerLevel)
                {
                    numCollected = 0;
                    state = State::collecting;
                    break;
                }

                prevSample = currentSample;
            }
        }

        if (state == State::collecting)
        {
            while (index++ < numSamples)
            {
                bufferL[numCollected] = *dataL++;
                bufferR[numCollected++] = *dataR++;

                if (numCollected == bufferL.size())
                {
                    audioBufferQueueL.push(bufferL.data(), bufferL.size());
                    audioBufferQueueR.push(bufferR.data(), bufferR.size());

                    state = State::waitingForTrigger;
                    prevSample = SampleType(100);
                    break;
                }
            }
        }
    }

private:
    //==============================================================================
    AudioBufferQueue<SampleType> &audioBufferQueueL;
    AudioBufferQueue<SampleType> &audioBufferQueueR;
    std::array<SampleType, AudioBufferQueue<SampleType>::bufferSize> bufferL;
    std::array<SampleType, AudioBufferQueue<SampleType>::bufferSize> bufferR;
    size_t numCollected;
    SampleType prevSample = SampleType(100);

    static constexpr auto triggerLevel = SampleType(0.005);

    enum class State
    {
        waitingForTrigger,
        collecting
    } state{State::waitingForTrigger};
};

template <typename SampleType>
class Scope : public juce::Component,
              private juce::Timer
{
public:
    using Queue = AudioBufferQueue<SampleType>;

    //==============================================================================
    Scope(Queue &queueToUseL, Queue &queueToUseR)
        : audioBufferQueueL(queueToUseL),
          audioBufferQueueR(queueToUseR)
    {
        sampleDataL.fill(SampleType(0));
        sampleDataR.fill(SampleType(0));
        setFramesPerSecond(60);
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

        g.setColour(juce::Colours::grey);
        plot(originLineData.data(), 2, g, scopeRect, SampleType(0.4), h / 2);
        plot(originLineData.data(), 2, g, scopeRect, SampleType(-0.4), h / 2);


        g.setColour(juce::Colours::yellow);
        plot(sampleDataL.data(), sampleDataL.size(), g, scopeRect, SampleType(0.4), h / 2);
        g.setColour(juce::Colours::lime);
        plot(sampleDataR.data(), sampleDataR.size(), g, scopeRect, SampleType(0.4), h / 2);

    
    }

    //==============================================================================
    void resized() override {}

    bool viewSpectrum = false;

private:
    Queue &audioBufferQueueL;
    Queue &audioBufferQueueR;
    std::array<SampleType, Queue::bufferSize> sampleDataL;
    std::array<SampleType, Queue::bufferSize> sampleDataR;

    std::array<SampleType, 2> originLineData = {SampleType(1), SampleType(1)};

    juce::dsp::FFT fft{Queue::order};
    using WindowFun = juce::dsp::WindowingFunction<SampleType>;
    WindowFun windowFun{(size_t)fft.getSize(), WindowFun::hann};

    std::array<SampleType, 2 * Queue::bufferSize> spectrumData;
    std::array<SampleType, 2 * Queue::bufferSize> scopeData;

    //==============================================================================
    void timerCallback() override
    {
        audioBufferQueueL.pop(sampleDataL.data());
        audioBufferQueueR.pop(sampleDataR.data());

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
    static void plot(const SampleType *data,
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