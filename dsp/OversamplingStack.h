
#include <JuceHeader.h>

class OversamplingStack { // TODO: implement this into the program
public:
    OversamplingStack() {
        for (int i = 1; i < 4; i++) {
            oversamplingStack.add(new dsp::Oversampling<float>(2, i, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
        }
    };

    ~OversamplingStack();

    dsp::AudioBlock<float>& getOversampledBlock(dsp::AudioBlock<float>& block) {
        if (oversamplingFactor == 0) { // if there is no oversampling
            return block;
        } else if (oversamplingFactor > oversamplingStack.size()) { // if the oversampling factor is higher than the max stacks we have
            return oversamplingStack.getLast()->processSamplesUp(block);
        } else {
            return oversamplingStack[oversamplingFactor - 1]->processSamplesUp(block);
        }
    }

    dsp::AudioBlock<float>& getOversampledBlockUnchecked(dsp::AudioBlock<float>& block) {
        return oversamplingStack[oversamplingFactor - 1]->processSamplesUp(block);
    }

    void prepare(dsp::ProcessSpec& spec) {
        for (auto& oversampler : oversamplingStack) {
            oversampler->initProcessing(spec.maximumBlockSize);
        }
    };

    void setOversamplingFactor(int factor) {
        oversamplingFactor = factor;
    }
private:
    juce::OwnedArray<dsp::Oversampling<float>> oversamplingStack;

    int maxOversamplingFactor = 4;
    int oversamplingFactor = 1;
};