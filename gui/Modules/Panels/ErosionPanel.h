#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class ErosionPanel : public Panel
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p) : Panel(p, "EROSION"),
        erosionAmt(p, "AMOUNT", "noiseAmount", 0.0f, 100.0f),
        erosionFreq(p, "FREQ", "noiseFrequency", 20.0f, 20000.0f),
        erosionQ(p, "Q", "noiseQ", 0.1f, 3.0f)
    {
        auto laf = new KnobLAF(juce::Colours::yellowgreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(erosionAmt);
        addAndMakeVisible(erosionFreq);
        addAndMakeVisible(erosionQ);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() {
        // three, in a row
        auto bounds = getLocalBounds();
        erosionAmt.setBounds(bounds.removeFromLeft(bounds.getWidth() / 3));
        erosionFreq.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        erosionQ.setBounds(bounds);
    }

    ParamKnob erosionAmt;
    ParamKnob erosionFreq;
    ParamKnob erosionQ;
};