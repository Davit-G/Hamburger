#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "../utils/Params.h"

/*  Everything the mod matrix routes to one parameter over one chunk, summed per channel and base rate sample: how far to
    move it across its range, and for keytracking a frequency, how many octaves to shift it by. */
struct ModDest
{
    static constexpr int size = 64;

    float add[2][size] {};
    float octaves[2][size] {};
    bool pitched = false;

    // a stage modulator's move at each stage of the stack, and the stage running, which the matrix owns
    static constexpr int maxStages = 4;
    float stageAdd[maxStages] {};
    const int* stage = nullptr;
    int length = size;
    int shift = 0; // log2 of the oversampling, an oversampled sample index >> shift is the chunk's sample
};

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
    void setScaler (MacroParam* newScaler, float neutralValue)
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

    // audio thread from here down. the mod matrix's sums for this parameter this chunk, nullptr while nothing's routed here
    ModDest* mod = nullptr;

    // getScaled, with the scaler as modulated at the start of the chunk
    float getBase() const
    {
        if (scaler == nullptr)
            return get();

        return juce::jlimit (range.start, range.end, neutral + (get() - neutral) * scaler->getModulated() * 0.01f);
    }

    float getModulated (int channel = 0, int sample = 0) const
    {
        return mod != nullptr ? modulate (getBase(), channel, sample) : getBase();
    }

    // a base value, smoothed or not, moved by the modulation at a sample of this chunk and kept within the range
    float modulate (float base, int channel, int sample) const
    {
        const auto moved = range.convertTo0to1 (base) + mod->add[channel][sample] + mod->stageAdd[*mod->stage];
        const auto value = range.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, moved));
        return mod->pitched ? juce::jlimit (range.start, range.end, value * std::exp2 (mod->octaves[channel][sample])) : value;
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
    MacroParam* scaler = nullptr;
    float neutral = 0.0f;
    juce::String nameOverride;
};
