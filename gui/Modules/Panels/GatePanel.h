#pragma once

#include "../Panel.h"


class GatePanel : public Panel
{
public:
    GatePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "GATE", Palette::colours[1]),
    gate(p, "GATE", slot, ParamIDs::gateAmt, ValueType::none, ScopeContextType::NOISE)
    {
        addAndMakeVisible(gate);

        Palette::setKnobColoursOfComponent(&gate, Palette::colours[1]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        gate.setBounds(bounds);
    }

private:
    // ParamKnob knob;
    ParamKnob gate;
};