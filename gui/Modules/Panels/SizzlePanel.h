#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"
#include "../../LookAndFeel/Palette.h" 

class SizzlePanel : public Panel
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p) : Panel(p, "SIZZLE"),
    sizzleKnob(p, "SIZZLE", "noiseAmount") {

        addAndMakeVisible(sizzleKnob);
    }

    void resized() override
    {
        sizzleKnob.setBounds(getLocalBounds());
    }

private:
    ParamKnob sizzleKnob;
};