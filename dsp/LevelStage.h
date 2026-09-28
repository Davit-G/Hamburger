#pragma once

#include "MacroParam.h"
#include "Crossover.h"
#include "../utils/Params.h"

#include "juce_dsp/juce_dsp.h"

// a slot's own in gain, dry/wet and out gain around whatever it runs. the out gain still applies while the slot is off
class LevelStage
{
public:
    // enough for the linear phase crossovers at 4x oversampling of 192kHz
    static constexpr int maxLatency = 16384;

    LevelStage (juce::AudioProcessorValueTreeState& state, SlotId slot)
    {
        auto level = [&state, slot] (const ParamIDs::ParameterInfo& descriptor)
        {
            return &MacroParam::fetch (state, paramIdFor (slot, descriptor).getParamID());
        };

        inGainParam = level (ParamIDs::slotInGain);
        mixParam = level (ParamIDs::slotMix);
        outGainParam = level (ParamIDs::slotOutGain);

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
        outGain.setGainDecibels (outGainDb());
        inGain.reset();
        outGain.reset();

        dryWet.prepare (spec);
        dryWet.setWetMixProportion (mixParam->get() * 0.01f);
        dryWet.reset();

        dryPhase.prepare (spec);
    }

    // the slot's crossovers and latency, which the dry is put through as well to mix back in phase
    void matchDry (const Crossovers& crossovers, int latencySamples)
    {
        dryPhase.clear();
        dryPhase.add (crossovers);

        if (latencySamples != latency)
        {
            latency = juce::jmin (latencySamples, maxLatency);
            dryWet.setWetLatency ((float) latency);
        }
    }

    // the slot's own processing runs between the in gain and the dry/wet mix, and is skipped while the slot is off
    template <typename Process>
    void process (juce::dsp::AudioBlock<float>& block, Process&& processSlot)
    {
        juce::dsp::ProcessContextReplacing<float> context (block);

        if (isEnabled())
        {
            dryPhase.pushDry (dryWet, block);

            inGain.setGainDecibels (inGainParam->getModulated());
            inGain.process (context);

            processSlot();

            dryWet.setWetMixProportion (mixParam->getModulated() * 0.01f);
            dryWet.mixWetSamples (block);
        }
        else if (latency > 0)
        {
            // switched off, the slot still delays everything it would have, which the dry already is
            dryWet.pushDrySamples (block);
            dryWet.setWetMixProportion (0.0f);
            dryWet.mixWetSamples (block);
        }

        outGain.setGainDecibels (outGainDb());
        outGain.process (context);
    }

private:
    // linked, the out gain takes back whatever the in gain added
    float outGainDb() const
    {
        const auto linked = gainLink != nullptr && gainLink->get() && isEnabled();
        return outGainParam->getModulated() - (linked ? inGainParam->getModulated() : 0.0f);
    }

    MacroParam* inGainParam = nullptr;
    MacroParam* mixParam = nullptr;
    MacroParam* outGainParam = nullptr;
    juce::AudioParameterBool* gainLink = nullptr;
    juce::AudioParameterBool* enabled = nullptr;

    juce::dsp::Gain<float> inGain, outGain;
    juce::dsp::DryWetMixer<float> dryWet { maxLatency };
    DryPhase dryPhase;
    int latency = 0;
};
