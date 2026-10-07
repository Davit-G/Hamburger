#pragma once

#include "MacroParam.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_gui_basics/juce_gui_basics.h"

#include "../utils/Params.h"

class EffectBase {
public:
    EffectBase() {}
    virtual ~EffectBase() = default;

    virtual void prepare(juce::dsp::ProcessSpec& spec) = 0;

    // call before processBlock to update any smoothed parameters and any processing needed (while actively running and not smoothing)
    virtual void updateParamsEveryBlock() {}
    virtual void processBlock(juce::dsp::AudioBlock<float>& block) = 0;
    virtual int getLatencySamples() const { return 0; }

    // // what a dry mixed back with this effect has to go through to stay in phase with it
    // virtual Crossovers getCrossovers() const { return {}; }

private:
    // juce::ValueTree paramStorage;
};

// effectbase but it basically holds info on what slot it is, to pass down to DSP stuff
class MacroEffect : public EffectBase
{
public:
    MacroEffect (juce::AudioProcessorValueTreeState& state, const SlotId slotID)
        : slot(slotID), treeState(state) {}

    // resolve a generic ParameterInfo to the actual generated parameter for this slot.
    MacroParam& getParam (const ParamIDs::ParameterInfo& which) const
    {
        return MacroParam::fetch (treeState, paramIdFor (slot, which).getParamID());
    }

protected:
    const SlotId slot;

private:
    juce::AudioProcessorValueTreeState& treeState;
};
