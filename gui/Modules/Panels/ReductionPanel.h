#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "BIT", &Theme::noise),
        downSample(p, "RATE", slot, ParamIDs::downsampleFreq, ScopeContextType::NOISE),
        bitReduction(p, "BITS", slot, ParamIDs::bitReduction, ScopeContextType::NOISE)
    {
        addKnobs({&downSample, &bitReduction});
    }

    void resized() override { layoutGrid({&downSample, &bitReduction}); }

private:
    ParamKnob downSample, bitReduction;
};
