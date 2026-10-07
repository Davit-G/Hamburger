#pragma once

#include "MacroParam.h"
#include "../utils/Params.h"

#include "juce_dsp/juce_dsp.h"

// a slot's own in gain, dry/wet and out gain around whatever it runs. the out gain still applies while the slot is off
class InOutMix
{
public:
    InOutMix (juce::AudioProcessorValueTreeState& state, SlotId slot)
    {
        inGainParam = &MacroParam::fetch (state, paramIdFor (slot, ParamIDs::slotInGain).getParamID());
        mixParam = &MacroParam::fetch (state, paramIdFor (slot, ParamIDs::slotMix).getParamID());
        outGainParam = &MacroParam::fetch (state, paramIdFor (slot, ParamIDs::slotOutGain).getParamID());

        gainLink = dynamic_cast<juce::AudioParameterBool*> (state.getParameter (paramIdFor (slot, ParamIDs::gainLink).getParamID()));
        enabled = dynamic_cast<juce::AudioParameterBool*> (state.getParameter (slot.enabled().getParamID()));
        jassert (gainLink != nullptr && enabled != nullptr);
    }

    bool isEnabled() const { return enabled == nullptr || enabled->get(); }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        for (auto* gain : { &inGain, &outGain })
        {
            gain->prepare (spec);
            gain->setRampDurationSeconds (0.02);
        }

        inGain.setGainDecibels (inGainParam->get());
        outGain.setGainDecibels (getOutGain());
        inGain.reset();
        outGain.reset();

        dryWet.prepare (spec);
        dryWet.setWetMixProportion (mixParam->get() * 0.01f);
        dryWet.reset();
    }

    // the slot's own processing runs between the in gain and the dry/wet mix, and is skipped while the slot is off
    template <typename Process>
    void process (juce::dsp::AudioBlock<float>& block, Process&& processSlot)
    {
        juce::dsp::ProcessContextReplacing<float> context (block);

        if (isEnabled())
        {

            inGain.setGainDecibels (inGainParam->get());
            inGain.process (context);

            processSlot();

            dryWet.setWetMixProportion (mixParam->get() * 0.01f);
            dryWet.mixWetSamples (block);
        }
        else if (latency > 0)
        {
            // switched off, the slot still delays everything it would have, which the dry already is
            dryWet.pushDrySamples (block);
            dryWet.setWetMixProportion (0.0f);
            dryWet.mixWetSamples (block);
        }

        
        outGain.setGainDecibels (getOutGain());

        outGain.process (context);
    }

private:
    inline const float getOutGain() {
        const auto linked = gainLink != nullptr && gainLink->get() && isEnabled();
        return outGainParam->get() - (linked ? inGainParam->get() : 0.0f);
    }

    MacroParam* inGainParam = nullptr;
    MacroParam* mixParam = nullptr;
    MacroParam* outGainParam = nullptr;
    juce::AudioParameterBool* gainLink = nullptr;
    juce::AudioParameterBool* enabled = nullptr;

    juce::dsp::Gain<float> inGain, outGain;
    juce::dsp::DryWetMixer<float> dryWet { 16384 }; // only set higher if necessary

    int latency = 0;
};
