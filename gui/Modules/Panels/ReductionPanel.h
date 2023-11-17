#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p) : Panel(p, "BIT"),
                                                   downSample(p, "DOWNSMPL", "downsampleAmount", 0.0f, 20.0f),
                                                   bitReduction(p, "BITS", "bitReduction", 1.0f, 32.0f),
                                                   jitter(p, "GRIT", "downsampleJitter", 0.0f, 1.0f)
    {
        auto laf = new KnobLAF(juce::Colours::yellowgreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);
        addAndMakeVisible(jitter);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() {
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