#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"

#include "../utils/Params.h"
#include "MacroParam.h"

using SmoothedValue = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

/* Only for use with floats / knobs, not categorical parameters. Reads the parameter as scaled by the start page, see
   MacroParam::getScaled */
class SmoothParam
{
public:
    // bind by descriptor, for fixed parameters that aren't part of a macro slot
    SmoothParam(juce::AudioProcessorValueTreeState& state, const ParamIDs::ParameterInfo& info)
        : param(dynamic_cast<MacroParam*>(state.getParameter(info.getParamID())))
    {
        if (param == nullptr)
            jassertfalse; // not registered, or not registered as a MacroParam
    }

    explicit SmoothParam(MacroParam& macroParam) : param(&macroParam) {}

    void prepare(juce::dsp::ProcessSpec& spec)
    {
        if (param == nullptr)
            return;

        smoothedParamPerChannel.assign(static_cast<size_t>(spec.numChannels), SmoothedValue{});

        for (auto& smoother : smoothedParamPerChannel)
        {
            smoother.reset(spec.sampleRate, 0.01);
            smoother.setCurrentAndTargetValue(param->getScaled());
        }
    }

    void update()
    {
        if (param == nullptr)
            return;

        for (auto& smoother : smoothedParamPerChannel)
            smoother.setTargetValue(param->getScaled());
    }

    float getNextValue(int channel = 0)
    {
        const auto ch = static_cast<size_t>(channel);

        if (ch >= smoothedParamPerChannel.size())
            return 0.0f;

        return smoothedParamPerChannel[ch].getNextValue();
    }

    // straight off the parameter, for values only read once per block
    float getRaw(int = 0) const { return param != nullptr ? param->getScaled() : 0.0f; }

    // use only if you actually use smoothing or not
    bool isSmoothing(int channel = 0) const
    {
        const auto ch = static_cast<size_t>(channel);

        if (ch >= smoothedParamPerChannel.size())
            return false;

        return smoothedParamPerChannel[ch].isSmoothing();
    }

private:
    MacroParam* param = nullptr;
    std::vector<SmoothedValue> smoothedParamPerChannel {};
};
