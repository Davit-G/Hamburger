#pragma once

#include "../Panel.h"

class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "BIT", Palette::colours[1]),
        downSample(p, "RATE", slot, ParamIDs::downsampleFreq, ValueType::hz, ScopeContextType::NOISE),
        bitReduction(p, "BITS", slot, ParamIDs::bitReduction, ValueType::none, ScopeContextType::NOISE)
    {
        addKnobs({&downSample, &bitReduction});
    }

    void resized() override { layoutGrid({&downSample, &bitReduction}); }

private:
    ParamKnob downSample, bitReduction;
};
