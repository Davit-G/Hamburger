#pragma once

#include "../Panel.h"
#include "PanelNames.h"

class JeffPanel : public Panel
{
public:
    JeffPanel(AudioPluginAudioProcessor &p) : Panel(p, "GATE"),
    // knob(p, "JEFF", "noiseAmount"),
    gate(p, "GATE", "gateAmt")
    {
        // addAndMakeVisible(knob);
        addAndMakeVisible(gate);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        // knob.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        gate.setBounds(bounds);
    }

private:
    // ParamKnob knob;
    ParamKnob gate;
};