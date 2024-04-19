#pragma once

 

#include "../Panel.h"


class UtilityPanel : public Panel
{
public:
    UtilityPanel(AudioPluginAudioProcessor &p) : Panel(p, "UTILITY"),
    inGain(p, "IN", ParamIDs::inputGain, ParamUnits::db),
    mix(p, "MIX", ParamIDs::mix, ParamUnits::percent),
    outGain(p, "OUT", ParamIDs::outputGain, ParamUnits::db)
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