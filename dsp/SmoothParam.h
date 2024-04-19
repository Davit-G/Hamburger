#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"

/* Only for use with floats / knobs, not categorical parameters */
class SmoothParam
{
public:
    SmoothParam(juce::AudioProcessorValueTreeState &treeState, juce::ParameterID attachmentID) {
        param = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(attachmentID.getParamID()));
        DBG(juce::String("SmoothParam: ") + attachmentID.getParamID());
        jassert(param); // heads up if the parameter doesn't exist
    }

    void prepare(juce::dsp::ProcessSpec& spec) {
        smoothedParam.reset(spec.sampleRate, 0.01);
        smoothedParam.setCurrentAndTargetValue(param->get());
    }

    void update() {
        oldRawVal = newRawVal;
        newRawVal = param->get();
        smoothedParam.setTargetValue(newRawVal);
    }

    void setTo(float value) {
        smoothedParam.setValue(value);
        oldRawVal = newRawVal = value;
    }

    void updateFloored() {
        oldRawVal = newRawVal;
        newRawVal = param->get();
        smoothedParam.setTargetValue(floor(newRawVal));
    }
    
    float getNextValue() {
        return get();
    }

    float get() {
        oldRawVal = newRawVal;
        newRawVal = smoothedParam.getNextValue();

        return newRawVal;
    }
    
    float getRaw() {
        oldRawVal = newRawVal;
        newRawVal = param->get();

        return newRawVal;
    }

    bool isChanged() {
        return oldRawVal != newRawVal;
    }

private:
    juce::AudioParameterFloat *param = nullptr;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedParam; // dont use multiply it leads to audio glitches

    float oldRawVal = 0.0f;
    float newRawVal = 0.0f;
};