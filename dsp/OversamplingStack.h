#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"

class OversamplingStack {
public:
    OversamplingStack() {
        oversamplingArray.clear();

        auto oversamplingDevice = new juce::dsp::Oversampling<float>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

        oversamplingArray.add(oversamplingDevice);
        oversamplingArray[0]->clearOversamplingStages();
        oversamplingArray[0]->addDummyOversamplingStage();

        for (int i = 1; i <= 3; i++) {
            // DBG("Adding oversampling stack " << i);
            oversamplingArray.add(new juce::dsp::Oversampling<float>(2, i, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true));
        }
    }

    ~OversamplingStack() {
        // oversamplingArray.clear();
    }

    juce::dsp::AudioBlock<float> processSamplesUp(juce::dsp::AudioBlock<float>& block) {
        if (oversamplingFactor == 0) { // if there is no oversampling
            return block;
        } else if (oversamplingFactor > oversamplingArray.size()) { // if the oversampling factor is higher than the max stacks we have
            return oversamplingArray.getLast()->processSamplesUp(block);
        } else {
            return oversamplingArray[oversamplingFactor]->processSamplesUp(block);
        }
    }

    juce::dsp::AudioBlock<float> processSamplesUpUnchecked(juce::dsp::AudioBlock<float>& block) {
        return oversamplingArray[oversamplingFactor]->processSamplesUp(block);
    }

    void processSamplesDown(juce::dsp::AudioBlock<float>& block) {
        if (oversamplingFactor > oversamplingArray.size()) { // if the oversampling factor is higher than the max stacks we have
            oversamplingArray.getLast()->processSamplesDown(block);
        } else if (oversamplingFactor != 0) {
            oversamplingArray[oversamplingFactor]->processSamplesDown(block);
        }

        // and if it's 0 then do nothing
    }

    float getLatencySamples() {
        if (oversamplingFactor == 0)
            return 0.0f;
        return oversamplingArray[oversamplingFactor]->getLatencyInSamples();
    }

    void processSamplesDownUnchecked(juce::dsp::AudioBlock<float>& block) {
        oversamplingArray[oversamplingFactor - 1]->processSamplesDown(block);
    }

    void prepare(juce::dsp::ProcessSpec& spec) {
        for (auto& oversampler : oversamplingArray) {
            oversampler->initProcessing(spec.maximumBlockSize);
        }
    }

    void setOversamplingFactor(int factor) {
        oversamplingFactor = factor;
    }

    int getOversamplingFactor() {
        return oversamplingFactor;
    }
private:
    juce::OwnedArray<juce::dsp::Oversampling<float>> oversamplingArray;

    // int maxOversamplingFactor = 4;
    int oversamplingFactor = 0;
};