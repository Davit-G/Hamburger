#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p) : Panel(p, "REDUCTION"),
                                                   downSample(p, "DOWNSAMPLE", "downsampleAmount", 0.0f, 20.0f),
                                                   bitReduction(p, "BITS", "bitReduction", 1.0f, 32.0f)
    {
        auto laf = new KnobLAF(juce::Colours::yellowgreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() {
        auto bounds = getLocalBounds();
        downSample.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        bitReduction.setBounds(bounds);
    }

    ParamKnob downSample;
    ParamKnob bitReduction;
};