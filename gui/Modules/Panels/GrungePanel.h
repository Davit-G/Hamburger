#pragma once

#include "../Panel.h"

class GrungePanel : public Panel
{
public:
    GrungePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module2, 0}) : Panel(p, "GRUNGE", Palette::colours[2]),
        amount(p, "AMT", slot, ParamIDs::grungeAmt),
        tone(p, "TONE", slot, ParamIDs::grungeTone)
    {
        addKnobs({&amount, &tone});
    }

    void resized() override { layoutGrid({&amount, &tone}); }

private:
    ParamKnob amount, tone;
};
