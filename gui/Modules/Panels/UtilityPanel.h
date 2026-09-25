#pragma once

 

#include "../Panel.h"


class UtilityPanel : public Panel
{
public:
    UtilityPanel(AudioPluginAudioProcessor &p) : Panel(p, "UTILITY"),
    inGain(p, "IN", ParamIDs::inputGain),
    mix(p, "MIX", ParamIDs::mix),
    outGain(p, "OUT", ParamIDs::outputGain)
    {
        
        addAndMakeVisible(inGain);
        addAndMakeVisible(mix);
        addAndMakeVisible(outGain);

        auto* link = p.treeState.getParameter(ParamIDs::gainLink.getParamID());
        inGain.setGainLink(link, false);
        outGain.setGainLink(link, true);
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
    juce::Grid grid;
    ParamKnob inGain;
    ParamKnob mix;
    ParamKnob outGain;
};