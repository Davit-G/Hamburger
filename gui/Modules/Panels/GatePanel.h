#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class GatePanel : public Panel
{
public:
    GatePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "GATE", &Theme::noise),
        gate(p, "GATE", slot, ParamIDs::gateAmt, ScopeContextType::NOISE),
        smooth(p, "SMOOTH", slot, ParamIDs::gateSmooth, ScopeContextType::NOISE)
    {
        addKnobs({&gate, &smooth});
    }

    void resized() override { layoutGrid({&gate, &smooth}); }

private:
    ParamKnob gate, smooth;
};
