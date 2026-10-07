#pragma once

 

#include "../Panel.h"


class UtilityPanel : public Panel
{
public:
    UtilityPanel(AudioPluginAudioProcessor &p) : Panel(p, "UTILITY"),
    inGain(p, "IN", ParamIDs::inputGain, ValueType::db),
    mix(p, "MIX", ParamIDs::mix, ValueType::percent),
    outGain(p, "OUT", ParamIDs::outputGain, ValueType::db)
    {
        
        addAndMakeVisible(inGain);
        addAndMakeVisible(mix);
        addAndMakeVisible(outGain);

        Palette::setKnobColoursOfComponent(&inGain, juce::Colours::whitesmoke);
        Palette::setKnobColoursOfComponent(&mix, juce::Colours::whitesmoke);
        Palette::setKnobColoursOfComponent(&outGain, juce::Colours::whitesmoke);
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