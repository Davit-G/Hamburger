#pragma once

#include "../Panel.h"



class AllPassPanel : public Panel
{
public:
    AllPassPanel(AudioPluginAudioProcessor &p) : Panel(p, "ALLPASS", Palette::colours[2]),
        amount(p, "AMOUNT", ParamIDs::allPassAmount),
        freq(p, "FREQ", ParamIDs::allPassFreq, ParamUnits::hz),
        q(p, "Q", ParamIDs::allPassQ)
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