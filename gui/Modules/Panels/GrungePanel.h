#pragma once

#include "../Panel.h"


#include "../../Knob.h"

class GrungePanel : public Panel
{
public:
    GrungePanel(AudioPluginAudioProcessor &p) : Panel(p, "GRUNGE"),
    amount(p, "AMT", ParamIDs::grungeAmt),
    tone(p, "TONE", ParamIDs::grungeTone) {

        addAndMakeVisible(amount);
        addAndMakeVisible(tone);
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