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

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        smoothedParam.reset(sampleRate, 0.1);
        smoothedParam.setCurrentAndTargetValue(param->get());
    }

    void update() {
        smoothedParam.setTargetValue(param->get());
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
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedParam;
};