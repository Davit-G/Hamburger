
#include <JuceHeader.h>

class OversamplingStack { // TODO: implement this into the program
public:
    OversamplingStack() {
        oversamplingArray.clear();

        auto oversamplingDevice = new dsp::Oversampling<float>(2, 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);

        oversamplingArray.add(oversamplingDevice);
        oversamplingArray[0]->clearOversamplingStages();
        oversamplingArray[0]->addDummyOversamplingStage();

        for (int i = 1; i <= 4; i++) {
            DBG("Adding oversampling stack " << i);
            oversamplingArray.add(new dsp::Oversampling<float>(2, i, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
        }
    };

    ~OversamplingStack() {
        // oversamplingArray.clear();
    };

    dsp::AudioBlock<float> processSamplesUp(dsp::AudioBlock<float>& block) {
        if (oversamplingFactor == 0) { // if there is no oversampling
            return block;
        } else if (oversamplingFactor > oversamplingArray.size()) { // if the oversampling factor is higher than the max stacks we have
            return oversamplingArray.getLast()->processSamplesUp(block);
        } else {
            return oversamplingArray[oversamplingFactor]->processSamplesUp(block);
        }
    }

    dsp::AudioBlock<float> processSamplesUpUnchecked(dsp::AudioBlock<float>& block) {
        return oversamplingArray[oversamplingFactor]->processSamplesUp(block);
    }

    void processSamplesDown(dsp::AudioBlock<float>& block) {
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

    void processSamplesDownUnchecked(dsp::AudioBlock<float>& block) {
        oversamplingArray[oversamplingFactor - 1]->processSamplesDown(block);
    }

    void prepare(dsp::ProcessSpec& spec) {
        for (auto& oversampler : oversamplingArray) {
            oversampler->initProcessing(spec.maximumBlockSize);
        }
    };

    void setOversamplingFactor(int factor) {
        oversamplingFactor = factor;
    }
private:
    juce::OwnedArray<dsp::Oversampling<float>> oversamplingArray;

    int maxOversamplingFactor = 4;
    int oversamplingFactor = 1;
};