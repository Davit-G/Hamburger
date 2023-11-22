#pragma once
#include <JuceHeader.h>

/* Only for use with floats / knobs, not categorical parameters */
class SmoothParam
{
public:
    SmoothParam(juce::AudioProcessorValueTreeState &treeState, juce::String attachmentID) {
        param = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(attachmentID));
        DBG(juce::String("SmoothParam: ") + attachmentID);
        jassert(param); // heads up if the parameter doesn't exist
    }

    void prepare(dsp::ProcessSpec& spec) {
        smoothedParam.reset(spec.sampleRate, 0.05);
        smoothedParam.setCurrentAndTargetValue(param->get());
    }

    void update() {
        smoothedParam.setTargetValue(param->get());
    }

    void updateFloored() {
        smoothedParam.setTargetValue(floor(param->get()));
    }
    
    float getNextValue() {
        return smoothedParam.getNextValue();
    }

    float get() {
        return smoothedParam.getNextValue();
    }
    
    float getRaw() {
        return param->get();
    }

private:
    juce::AudioParameterFloat *param = nullptr;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedParam; // dont use multiply it leads to audio glitches
};