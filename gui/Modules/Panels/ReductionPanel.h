#pragma once

 

#include "../Panel.h"
#include "PanelNames.h"

class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p) : Panel(p, "BIT"),
                                                   downSample(p, "RATE", "downsampleFreq", ParamUnits::hz),
                                                   bitReduction(p, "BITS", "bitReduction"),
                                                   jitter(p, "GRIT", "downsampleJitter")
    {
        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);
        addAndMakeVisible(jitter);
    }

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