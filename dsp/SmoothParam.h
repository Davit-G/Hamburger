#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"
#include "chowdsp_simd/chowdsp_simd.h"

#include "../utils/Params.h"

using SmoothedValue = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

/* Only for use with floats / knobs, not categorical parameters */
class SmoothParam
{
public:
    explicit SmoothParam(juce::AudioProcessorValueTreeState& treeState, ParamIDs::ParameterInfo paramInfo)
        : param (dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(paramInfo.getParamID())))
    {
        if (param == nullptr)
            jassertfalse; // heads up if the parameter doesn't exist
    }

    void prepare(juce::dsp::ProcessSpec& spec)
    {
        smoothedParamPerChannel.resize(static_cast<size_t>(spec.numChannels));

        if (param == nullptr)
            return;

        const auto target = param->get();
        old = target;
        current = target;

        for (size_t i = 0; i < smoothedParamPerChannel.size(); ++i)
        {
            smoothedParamPerChannel[i] = std::make_unique<SmoothedValue>();
            smoothedParamPerChannel[i]->reset(spec.sampleRate, 0.01);
            smoothedParamPerChannel[i]->setCurrentAndTargetValue(target);
        }
    }

    void update()
    {
        if (param == nullptr)
            return;

        const auto target = param->get();

        for (auto& smoother : smoothedParamPerChannel)
        {
            if (smoother != nullptr)
                smoother->setTargetValue(target);
        }

        old = current;
        current = target;
    }

    float getNextValue(int channel = 0)
    {
        if (param == nullptr)
            return 0.0f;

        const auto ch = static_cast<size_t>(channel);
        if (ch >= smoothedParamPerChannel.size())
            return 0.0f;

        return smoothedParamPerChannel[ch]->getNextValue();
    }

    float getCurrent(int channel = 0) {
        if (param == nullptr)
            return 0.0f;

        const auto ch = static_cast<size_t>(channel);
        if (ch >= smoothedParamPerChannel.size())
            return 0.0f;

        return smoothedParamPerChannel[ch]->getCurrentValue();
    }

    float getRaw(int channel = 0)
    {
        if (param == nullptr)
            return 0.0f;

        const auto target = param->get();
        old = current;
        current = target;
        return target;
    }

    // use only if you actually use smoothing or not
    bool isSmoothing(int channel = 0) const
    {
        if (param == nullptr)
            return false;

        const auto ch = static_cast<size_t>(channel);
        if (ch >= smoothedParamPerChannel.size())
            return false;

        return smoothedParamPerChannel[ch]->isSmoothing();
    }

    bool isChanged() const
    {
        return !juce::approximatelyEqual(current, old);
    }

private:
    juce::AudioParameterFloat* param = nullptr;
    std::vector<std::unique_ptr<SmoothedValue>> smoothedParamPerChannel {}; // dont use multiply it leads to audio glitches

    float current = 0.0f;
    float old = 0.0f;
};