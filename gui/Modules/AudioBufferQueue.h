#pragma once

#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "ScopeConstants.h"

//==============================================================================
template <typename SampleType>
class AudioBufferQueue
{
public:
    AudioBufferQueue();

    void resize(size_t size);

    // obtains the currently consumed space (the amount that can be currently read from the buffer)
    int getReadableSpace();

    //==============================================================================
    // happens on audio thread, we dont need write locks here.
    // if we go over the size limit this could be dodgy?
    // returns the number of samples written
    float push(const SampleType *dataToPush, size_t numSamples);

    //==============================================================================
    // happens on ui thread
    float pop(SampleType *outputBuffer, size_t numSamples);

    // reset fifo structure to start from the start without reallocating
    void reset();

private:
    //==============================================================================
    juce::AbstractFifo abstractFifo {scope_constants::defaultQueueSize};
    juce::ReadWriteLock lock; // using a lock to block the ui thread when a resize occurs
    std::vector<SampleType> buffer;
};
