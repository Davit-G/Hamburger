#pragma once

#include "../Panel.h"


class GatePanel : public Panel
{
public:
    GatePanel(AudioPluginAudioProcessor &p) : Panel(p, "GATE", Palette::colours[1]),
    gate(p, "GATE", ParamIDs::gateAmt),
    gateMix(p, "MIX", ParamIDs::gateMix)
    {
        addAndMakeVisible(gate);
        addAndMakeVisible(gateMix);

        Palette::setKnobColoursOfComponent(&gate, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&gateMix, Palette::colours[1]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        gate.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        gateMix.setBounds(bounds);
    }

private:
    // ParamKnob knob;
    ParamKnob gate;
    ParamKnob gateMix;
};