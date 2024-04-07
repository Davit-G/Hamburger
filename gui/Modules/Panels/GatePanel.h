#pragma once

#include "../Panel.h"
#include "PanelNames.h"

class GatePanel : public Panel
{
public:
    GatePanel(AudioPluginAudioProcessor &p) : Panel(p, "GATE"),
    gate(p, "GATE", "gateAmt"),
    gateMix(p, "MIX", "gateMix")
    {
        addAndMakeVisible(gate);
        addAndMakeVisible(gateMix);
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