#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"


class GatePanel : public Panel
{
public:
    GatePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "GATE", &Theme::gate),
    gate(p, "GATE", slot, ParamIDs::gateAmt, ScopeContextType::NOISE),
    smooth(p, "SMOOTH", slot, ParamIDs::gateSmooth, ScopeContextType::NOISE)
    {
        addAndMakeVisible(gate);
        addAndMakeVisible(smooth);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        gate.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        smooth.setBounds(bounds);
    }

private:
    ParamKnob gate;
    ParamKnob smooth;
};