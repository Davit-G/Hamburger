#include "ScopeDataCollector.h"

//==============================================================================
template <typename SampleType>
void ScopeDataCollector<SampleType>::prepare(juce::dsp::ProcessSpec& spec)
{
    currentSampleRate.store(spec.sampleRate);

    audioBufferQueueL.resize(spec.sampleRate / 30);
    audioBufferQueueR.resize(spec.sampleRate / 30);

    audioBufferQueuePreDistortion.resize(spec.sampleRate / 30);
    audioBufferQueuePostDistortion.resize(spec.sampleRate / 30);

    const int maxScopeSamples = juce::jmax<int>((int) spec.maximumBlockSize, 1) * 16;
    preDistScratchBuffer.setSize(1, maxScopeSamples, false, false);
    postDistScratchBuffer.setSize(1, maxScopeSamples, false, false);
}

//==============================================================================
template <typename SampleType>
void ScopeDataCollector<SampleType>::process(const SampleType *dataL, const SampleType *dataR, size_t numSamples)
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

template <typename SampleType>
void ScopeDataCollector<SampleType>::decimate(juce::AudioBuffer<SampleType>& scratch, const SampleType* dataL, size_t numSamples, int oversamplingFactor)
{
    const size_t step = (size_t) 1 << oversamplingFactor; // 2 ^ oversamplingFactor
    const size_t decimatedCount = numSamples / step;

    if (scratch.getNumSamples() < (int) decimatedCount)
        scratch.setSize(1, (int) decimatedCount, false, false);

    auto* out = scratch.getWritePointer(0);

    for (size_t i = 0; i < decimatedCount; ++i)
        out[i] = dataL[i * step];
}

template <typename SampleType>
void ScopeDataCollector<SampleType>::capturePreDistortion(const SampleType *dataL, size_t numSamples, int oversamplingFactor)
{
    if (oversamplingFactor <= 0)
    {
        samplesReadPre = audioBufferQueuePreDistortion.push(dataL, numSamples);
    }
    else
    {
        decimate(preDistScratchBuffer, dataL, numSamples, oversamplingFactor);

        const auto decimatedCount = numSamples >> oversamplingFactor;
        samplesReadPre = audioBufferQueuePreDistortion.push(preDistScratchBuffer.getReadPointer(0), decimatedCount);
    }

    if (audioBufferQueuePreDistortion.consumeOverflowFlag())
        resyncPrePostQueues();
}

template <typename SampleType>
void ScopeDataCollector<SampleType>::capturePostDistortion(const SampleType *dataL, size_t numSamples, int oversamplingFactor)
{
    if (oversamplingFactor <= 0)
    {
        samplesReadPost = audioBufferQueuePostDistortion.push(dataL, numSamples);
    }
    else
    {
        decimate(postDistScratchBuffer, dataL, numSamples, oversamplingFactor);

        const auto decimatedCount = numSamples >> oversamplingFactor;
        samplesReadPost = audioBufferQueuePostDistortion.push(postDistScratchBuffer.getReadPointer(0), decimatedCount);
    }

    if (audioBufferQueuePostDistortion.consumeOverflowFlag() || samplesReadPre != samplesReadPost)
        resyncPrePostQueues();
}

template <typename SampleType>
void ScopeDataCollector<SampleType>::resyncPrePostQueues()
{
    audioBufferQueuePreDistortion.reset();
    audioBufferQueuePostDistortion.reset();
}

//==============================================================================
template class ScopeDataCollector<float>;
