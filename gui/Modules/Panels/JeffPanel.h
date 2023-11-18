#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class JeffPanel : public Panel
{
public:
    JeffPanel(AudioPluginAudioProcessor &p) : Panel(p, "JEFF"),
    knob(p, "JEFF", "noiseAmount") {
        auto laf = new KnobLAF(juce::Colours::limegreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(knob);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() override
    {
        knob.setBounds(getLocalBounds());
    }

private:
    ParamKnob knob;
};