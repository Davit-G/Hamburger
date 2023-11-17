#pragma once

#include <JuceHeader.h>
#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TUBE"), knob(p, "BIAS", "tubeTone", -1.0f, 1.0f) {
        auto laf = new KnobLAF(juce::Colours::yellowgreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(knob);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        knob.setBounds(bounds);
    }

    ParamKnob knob;
};