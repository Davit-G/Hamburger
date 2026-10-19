#pragma once

#include "../Panel.h"

class AllPassPanel : public Panel
{
public:
    AllPassPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module2, 0}) : Panel(p, "ALLPASS", Palette::colours[2]),
        amount(p, "AMOUNT", slot, ParamIDs::allPassAmount),
        freq(p, "FREQ", slot, ParamIDs::allPassFreq, ValueType::hz),
        q(p, "Q", slot, ParamIDs::allPassQ)
    {
        addKnobs({&amount, &freq, &q});
    }

    void resized() override { layoutGrid({&amount, &freq, &q}); }

private:
    ParamKnob amount, freq, q;
};
