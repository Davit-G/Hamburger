#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"


class GatePanel : public Panel
{
public:
    GatePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "GATE", &Theme::gate),
    gate(p, "GATE", slot, ParamIDs::gateAmt, ScopeContextType::NOISE),
    gateMix(p, "MIX", slot, ParamIDs::gateMix, ScopeContextType::NOISE)
    {
        addAndMakeVisible(gate);
        addAndMakeVisible(gateMix);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        gate.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        gateMix.setBounds(bounds);
    }

private:
    // ParamKnob knob;
    ParamKnob gate;
    ParamKnob gateMix;
};