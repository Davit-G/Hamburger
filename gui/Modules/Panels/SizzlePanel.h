#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class SizzlePanel : public Panel
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p) : Panel(p, "SIZZLE"),
    sizzleKnob(p, "SIZZLE", "noiseAmount") {
        auto laf = new KnobLAF(juce::Colours::limegreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(sizzleKnob);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() override
    {
        sizzleKnob.setBounds(getLocalBounds());
    }

private:
    ParamKnob sizzleKnob;
};