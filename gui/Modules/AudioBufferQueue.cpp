#include "AudioBufferQueue.h"

//==============================================================================
template <typename SampleType>
AudioBufferQueue<SampleType>::AudioBufferQueue()
{
    resize(scope_constants::defaultQueueSize);
}

template <typename SampleType>
void AudioBufferQueue<SampleType>::resize(size_t size)
{
    lock.enterWrite();

    const auto capacity = juce::jmax<int>((int) size, 1);
    buffer.resize(capacity * 3); // give ourselves leeway if the ui thread is not consuming fast enough;
    abstractFifo.setTotalSize((int) buffer.size());
    abstractFifo.reset();
    overflowed = false;

    lock.exitWrite();
}

template <typename SampleType>
int AudioBufferQueue<SampleType>::getReadableSpace()
{
    lock.enterRead();

    int start1, size1, start2, size2;
    abstractFifo.prepareToRead((int) buffer.size(), start1, size1, start2, size2);

    lock.exitRead();

    return size1 + size2;
}

//==============================================================================
template <typename SampleType>
float AudioBufferQueue<SampleType>::push(const SampleType *dataToPush, size_t numSamples)
{
    jassert(numSamples <= buffer.size());

    lock.enterRead();

    const auto requiredSamples = juce::jmin<int>((int) numSamples, (int) buffer.size());

    int start1, size1, start2, size2;
    abstractFifo.prepareToWrite(requiredSamples, start1, size1, start2, size2);

    if (size1 + size2 < requiredSamples)
    {
        abstractFifo.reset();
        abstractFifo.prepareToWrite(requiredSamples, start1, size1, start2, size2);
        overflowed = true;
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

    lock.exitRead();

    return samplesWritten;
}

//==============================================================================
template <typename SampleType>
float AudioBufferQueue<SampleType>::pop(SampleType *outputBuffer, size_t numSamples)
{
    lock.enterRead();

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

    lock.exitRead();
    return samplesRead;
}

template <typename SampleType>
void AudioBufferQueue<SampleType>::reset()
{
    abstractFifo.reset();
}

template <typename SampleType>
bool AudioBufferQueue<SampleType>::consumeOverflowFlag() noexcept
{
    const bool result = overflowed;
    overflowed = false;
    return result;
}

//==============================================================================
template class AudioBufferQueue<float>;
