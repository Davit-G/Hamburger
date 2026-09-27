#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "../utils/Params.h"

class MacroParam : public juce::AudioParameterFloat
{
public:
    // a slot-scoped effect parameter, e.g. "main_rubidiumAmount"
    MacroParam(SlotId slot, const ParamIDs::ParameterInfo& descriptor)
    : juce::AudioParameterFloat(paramIdFor(slot, descriptor),
                                nameFor(slot, descriptor),
                                descriptor.range,
                                descriptor.defaultValue),
      info(&descriptor) {}

    // a fixed parameter that belongs to no slot, e.g. inputGain
    explicit MacroParam(const ParamIDs::ParameterInfo& descriptor)
    : juce::AudioParameterFloat(descriptor.getParameterID(),
                                descriptor.displayName,
                                descriptor.range,
                                descriptor.defaultValue),
      info(&descriptor) {}

    const ParamIDs::ParameterInfo& getDescriptor() const noexcept { return *info; }

    // the start page amount scaling this, like global drive, and the value it scales from. set once before any audio runs
    void setScaler (juce::AudioParameterFloat* newScaler, float neutralValue)
    {
        scaler = newScaler;
        neutral = neutralValue;
    }

    juce::AudioParameterFloat* getScaler() const noexcept { return scaler; }

    /*  What the audio gets. The knob's distance from neutral times the scaler's percentage, kept within the knob's own
        range. The knob itself stays where it's set. */
    float getScaled() const
    {
        if (scaler == nullptr)
            return get();

        return juce::jlimit (range.start, range.end, neutral + (get() - neutral) * scaler->get() * 0.01f);
    }

    // rename the parameter the DAW sees
    // E.G: "LOW BAND DRIVE" rather than "DISTORTION SLOT 1 Drive".
    // message thread only, and use updateHostDisplay (ChangeDetails{}.withParameterInfoChanged (true)) afterwards
    void setNameOverride (juce::String newName) { nameOverride = std::move (newName); }
    void clearNameOverride()                    { nameOverride.clear(); }


    static MacroParam& fetch(juce::AudioProcessorValueTreeState& state, const juce::String& paramID)
    {
        auto* param = dynamic_cast<MacroParam*>(state.getParameter(paramID));
        jassert(param != nullptr); // not registered, or not registered as a MacroParam
        return *param;
    }

private:
    juce::String getName (int maximumStringLength) const override
    {
        return (nameOverride.isNotEmpty() ? nameOverride : name).substring (0, maximumStringLength);
    }

    static juce::String nameFor(SlotId slot, const ParamIDs::ParameterInfo& descriptor)
    {
        return slot.displayNamePrefix() + " " + descriptor.displayName;
    }

    const ParamIDs::ParameterInfo* info = nullptr;
    juce::AudioParameterFloat* scaler = nullptr;
    float neutral = 0.0f;
    juce::String nameOverride;
};
