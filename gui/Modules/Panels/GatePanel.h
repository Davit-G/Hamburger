#pragma once

#include "../Panel.h"

class GatePanel : public Panel
{
public:
    GatePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "GATE", Palette::colours[1]),
        gate(p, "GATE", slot, ParamIDs::gateAmt, ValueType::none, ScopeContextType::NOISE)
    {
        addKnobs({&gate});
    }

    void resized() override { layoutGrid({&gate}); }

private:
    ParamKnob gate;
};
