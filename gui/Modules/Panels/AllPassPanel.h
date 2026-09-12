#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"



class AllPassPanel : public Panel
{
public:
    AllPassPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module2, 0}) : Panel(p, "ALLPASS", Palette::colours[2]),
        amount(p, "AMOUNT", slot, ParamIDs::allPassAmount),
        freq(p, "FREQ", slot, ParamIDs::allPassFreq),
        q(p, "Q", slot, ParamIDs::allPassQ)
    {
        addAndMakeVisible(amount);
        addAndMakeVisible(freq);
        addAndMakeVisible(q);

        Palette::setKnobColoursOfComponent(&amount, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&freq, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&q, Palette::colours[2]);
    }

    void resized() override {
        // three, in a row
        auto bounds = getLocalBounds();
        amount.setBounds(bounds.removeFromLeft(bounds.getWidth() / 3));
        freq.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        q.setBounds(bounds);
    }

    ParamKnob amount;
    ParamKnob freq;
    ParamKnob q;
};