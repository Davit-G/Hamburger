#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"


#include "../../Knob.h"

class GrungePanel : public Panel
{
public:
    GrungePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module2, 0}) : Panel(p, "GRUNGE", Palette::colours[2]),
    amount(p, "AMT", slot, ParamIDs::grungeAmt),
    tone(p, "TONE", slot, ParamIDs::grungeTone) {

        addAndMakeVisible(amount);
        addAndMakeVisible(tone);

        Palette::setKnobColoursOfComponent(&amount, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&tone, Palette::colours[2]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto width = bounds.getWidth() / 2;
        amount.setBounds(bounds.removeFromLeft(width));
        tone.setBounds(bounds);
    }

private:
    ParamKnob amount;
    ParamKnob tone;
};