#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class AllPassPanel : public Panel
{
public:
    AllPassPanel(AudioPluginAudioProcessor &p) : Panel(p, "ALLPASS"),
        amount(p, "AMOUNT", "allPassAmount", 0.0f, 50.0f),
        freq(p, "FREQ", "allPassFreq", 20.0f, 20000.0f),
        q(p, "Q", "allPassQ", 0.01f, 1.41f)
    {
        auto laf = new KnobLAF(juce::Colours::yellowgreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(amount);
        addAndMakeVisible(freq);
        addAndMakeVisible(q);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() {
        // three, in a row
        auto bounds = getLocalBounds();
        amount.setBounds(bounds.removeFromLeft(bounds.getWidth() / 3));
        freq.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        q.setBounds(bounds);
    }

    ParamKnob amount;
    ParamKnob freq;
    ParamKnob q;
};