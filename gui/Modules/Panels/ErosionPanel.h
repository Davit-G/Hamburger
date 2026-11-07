#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class ErosionPanel : public Panel
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "EROSION", &Theme::noise),
        erosionAmt(p, "AMOUNT", slot, ParamIDs::erosionAmount, ScopeContextType::NOISE),
        erosionFreq(p, "FREQ", slot, ParamIDs::erosionFrequency, ScopeContextType::NOISE),
        erosionQ(p, "Q", slot, ParamIDs::erosionQ, ScopeContextType::NOISE)
    {
        addKnobs({&erosionAmt, &erosionFreq, &erosionQ});
    }

    void resized() override { layoutGrid({&erosionAmt, &erosionFreq, &erosionQ}); }

private:
    ParamKnob erosionAmt, erosionFreq, erosionQ;
};
