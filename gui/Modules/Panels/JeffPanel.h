#pragma once

 

#include "../Panel.h"
#include "PanelNames.h"
#include "../../LookAndFeel/Palette.h" 

class JeffPanel : public Panel
{
public:
    JeffPanel(AudioPluginAudioProcessor &p) : Panel(p, "JEFF"),
    knob(p, "JEFF", "noiseAmount") {

        addAndMakeVisible(knob);
    }

    void resized() override
    {
        knob.setBounds(getLocalBounds());
    }

private:
    ParamKnob knob;
};