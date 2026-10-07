#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"

#include "../utils/Params.h"
#include "MacroParam.h"

using SmoothedValue = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

/* Only for use with floats / knobs, not categorical parameters */
class SmoothParam
{
public:
    SmoothParam(juce::AudioProcessorValueTreeState& state, const ParamIDs::ParameterInfo& info)
        : param(dynamic_cast<MacroParam*>(state.getParameter(info.getParamID())))
    {
        if (param == nullptr)
            jassertfalse; // not registered, maybe your paramid hasnt been registered yet?
    }

    explicit SmoothParam(MacroParam& macroParam) : param(&macroParam) {}

    // from one place, disable / enable knob smoothing
    static inline std::atomic<bool> smoothingEnabled { true };

    void prepare(juce::dsp::ProcessSpec& spec)
    {
        if (param == nullptr)
            return;

        smoothedParamPerChannel.assign(static_cast<size_t>(spec.numChannels), SmoothedValue{});
        positions.assign(static_cast<size_t>(spec.numChannels), 0);

        for (auto& smoother : smoothedParamPerChannel)
        {
            smoother.reset(spec.sampleRate, 0.01);
            smoother.setCurrentAndTargetValue(param->get());
        }
    }

    void update()
    {
        if (param == nullptr)
            return;

        const auto target = param->get();
        const auto smoothing = smoothingEnabled.load(std::memory_order_relaxed);

        for (auto& smoother : smoothedParamPerChannel)
        {
            if (smoothing)
                smoother.setTargetValue(target);
            else
                smoother.setCurrentAndTargetValue(target);
        }

        std::fill(positions.begin(), positions.end(), 0);
    }

    float getNextValue(int channel = 0)
    {
        const auto ch = static_cast<size_t>(channel);

        if (ch >= smoothedParamPerChannel.size())
            return 0.0f;

        return smoothedParamPerChannel[ch].getNextValue();
    }

    // straight off the parameter, for values only read once per block
    float getRaw(int = 0) const { return param != nullptr ? param->get() : 0.0f; }

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
    std::vector<int> positions {};
};
