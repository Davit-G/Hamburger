#pragma once

#include <JuceHeader.h>
#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TUBE") {
        auto laf = new KnobLAF(juce::Colours::violet);
        setLookAndFeel(laf); // will cascade to all children knobs
        // addAndMakeVisible(knob);
        // addAndMakeVisible(knob2);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        // knob.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        // knob2.setBounds(bounds);
    }

    // ParamKnob knob;
    // ParamKnob knob2;
};