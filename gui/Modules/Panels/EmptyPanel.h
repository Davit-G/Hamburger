#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class EmptyPanel : public Panel
{
public:
    EmptyPanel(AudioPluginAudioProcessor &p) : Panel(p, "EMPTY") {
        auto laf = new KnobLAF(juce::Colours::white);
        setLookAndFeel(laf); // will cascade to all children knobs
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }
};