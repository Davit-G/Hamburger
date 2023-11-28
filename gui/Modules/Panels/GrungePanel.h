#pragma once

#include "../Panel.h"
#include "PanelNames.h"
#include "../../Knob.h"

class GrungePanel : public Panel
{
public:
    GrungePanel(AudioPluginAudioProcessor &p) : Panel(p, "GRUNGE"),
    amount(p, "AMT", "grungeAmt"),
    tone(p, "TONE", "grungeTone") {

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