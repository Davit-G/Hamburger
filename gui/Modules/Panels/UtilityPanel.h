#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

/*
params.add(std::make_unique<AudioParameterFloat>("inputGain", "Input Gain", -24.0f, 24.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("outputGain", "Out Gain", -24.0f, 24.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("mix", "Mix", 0.0f, 100.0f, 100.f));
*/

class UtilityPanel : public Panel
{
public:
    UtilityPanel(AudioPluginAudioProcessor &p) : Panel(p, "UTILITY"),
    inGain(p, "IN", "inputGain"),
    mix(p, "MIX", "mix"),
    outGain(p, "OUT", "outputGain")
    {
        auto laf = new KnobLAF(juce::Colours::whitesmoke);
        setLookAndFeel(laf); // will cascade to all children knobs
        
        addAndMakeVisible(inGain);
        addAndMakeVisible(mix);
        addAndMakeVisible(outGain);
    }

    // void paint(juce::Graphics &g) override
    // {
    //     // g.fillAll(juce::Colours::cyan);
    // }

    void resized() {
        auto bounds = getLocalBounds().reduced(20);
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