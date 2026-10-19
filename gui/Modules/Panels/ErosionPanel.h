#pragma once

#include "../Panel.h"

class ErosionPanel : public Panel
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "EROSION", Palette::colours[1]),
        erosionAmt(p, "AMOUNT", slot, ParamIDs::erosionAmount, ValueType::none, ScopeContextType::NOISE),
        erosionFreq(p, "FREQ", slot, ParamIDs::erosionFrequency, ValueType::hz, ScopeContextType::NOISE),
        erosionQ(p, "Q", slot, ParamIDs::erosionQ, ValueType::none, ScopeContextType::NOISE)
    {
        addKnobs({&erosionAmt, &erosionFreq, &erosionQ});
    }

    void resized() override { layoutGrid({&erosionAmt, &erosionFreq, &erosionQ}); }

private:
    ParamKnob erosionAmt, erosionFreq, erosionQ;
};
