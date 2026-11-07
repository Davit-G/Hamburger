#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class AllPassPanel : public Panel
{
public:
    AllPassPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module2, 0}) : Panel(p, "ALLPASS", &Theme::preFx),
        amount(p, "AMOUNT", slot, ParamIDs::allPassAmount),
        freq(p, "FREQ", slot, ParamIDs::allPassFreq),
        q(p, "Q", slot, ParamIDs::allPassQ)
    {
        addKnobs({&amount, &freq, &q});
    }

    void resized() override { layoutGrid({&amount, &freq, &q}); }

private:
    ParamKnob amount, freq, q;
};
