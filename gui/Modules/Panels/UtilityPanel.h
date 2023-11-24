#pragma once

 

#include "../Panel.h"
#include "PanelNames.h"

class UtilityPanel : public Panel
{
public:
    UtilityPanel(AudioPluginAudioProcessor &p) : Panel(p, "UTILITY"),
    inGain(p, "IN", "inputGain", ParamUnits::db),
    mix(p, "MIX", "mix", ParamUnits::percent),
    outGain(p, "OUT", "outputGain", ParamUnits::db)
    {
        
        addAndMakeVisible(inGain);
        addAndMakeVisible(mix);
        addAndMakeVisible(outGain);
    }

    void resized() {
        auto bounds = getLocalBounds();
        auto top = bounds.removeFromLeft(bounds.getWidth() / 3);
        auto mid = bounds.removeFromLeft(bounds.getWidth() / 2);
        auto bot = bounds;

        inGain.setBounds(top);
        mix.setBounds(mid);
        outGain.setBounds(bot);
    }
private:
    Grid grid;
    ParamKnob inGain;
    ParamKnob mix;
    ParamKnob outGain;
};