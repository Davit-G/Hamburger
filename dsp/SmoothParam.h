#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"

#include "../utils/Params.h"
#include "MacroParam.h"

using SmoothedValue = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

/* Only for use with floats / knobs, not categorical parameters. Reads the parameter as scaled by the start page, see
   MacroParam::getScaled. Only the knob's own value is smoothed, the mod matrix's modulation goes on top of it unsmoothed,
   sample by sample through getNextValue, or as it is at the start of the chunk through getRaw */
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

    // off in the settings, every change lands at the start of the chunk it comes in. one app setting, so every instance shares it
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
            smoother.setCurrentAndTargetValue(param->getBase());
        }
    }

    void update()
    {
        if (param == nullptr)
            return;

        const auto target = param->getBase();
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

        const auto value = smoothedParamPerChannel[ch].getNextValue();
        const auto* mod = param->mod;

        if (mod == nullptr)
            return value;

        // where this channel is in the chunk, wrapping for a module that runs the same chunk more than once, like the stack
        auto& position = positions[ch];
        const auto sample = position >> mod->shift;

        if (++position >= mod->length << mod->shift)
            position = 0;

        return param->modulate(value, juce::jmin(channel, 1), sample);
    }

    // straight off the parameter, for values only read once per block
    float getRaw(int = 0) const { return param != nullptr ? param->getModulated() : 0.0f; }

    // use only if you actually use smoothing or not
    bool isSmoothing(int channel = 0) const
    {
        const auto ch = static_cast<size_t>(channel);

        if (ch >= smoothedParamPerChannel.size())
            return false;

        return smoothedParamPerChannel[ch].isSmoothing() || param->mod != nullptr;
    }

private:
    MacroParam* param = nullptr;
    std::vector<SmoothedValue> smoothedParamPerChannel {};
    std::vector<int> positions {};
};
