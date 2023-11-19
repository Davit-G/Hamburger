#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p, juce::String name) : Panel(p, name),
                                                   downSample(p, "RATE", "downsampleFreq"),
                                                   bitReduction(p, "BITS", "bitReduction"),
                                                   jitter(p, "GRIT", "downsampleJitter")
    {
        auto laf = new KnobLAF(juce::Colours::yellowgreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);
        addAndMakeVisible(jitter);
    }

    // void paint(juce::Graphics &g) override
    // {
    //     // g.fillAll(juce::Colours::green);
    // }

    void resized() override {
        auto bounds = getLocalBounds();
        auto width = bounds.getWidth() / 3;
        downSample.setBounds(bounds.removeFromLeft(width));
        jitter.setBounds(bounds.removeFromLeft(width));
        bitReduction.setBounds(bounds);
    }

    ParamKnob downSample;
    ParamKnob jitter;
    ParamKnob bitReduction;
};