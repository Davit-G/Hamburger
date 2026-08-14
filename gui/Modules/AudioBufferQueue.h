#pragma once

#include "ScopeConstants.h"

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