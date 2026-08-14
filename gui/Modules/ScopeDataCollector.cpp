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
void ScopeDataCollector<SampleType>::capturePreDistortion(const SampleType *dataL, size_t numSamples, int oversamplingFactor)
{
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

template <typename SampleType>
void ScopeDataCollector<SampleType>::capturePostDistortion(const SampleType *dataL, size_t numSamples, int oversamplingFactor)
{
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

template <typename SampleType>
void ScopeDataCollector<SampleType>::prepareOversampler(std::unique_ptr<juce::dsp::Oversampling<SampleType>>& oversampler,
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

//==============================================================================
template class ScopeDataCollector<float>;
