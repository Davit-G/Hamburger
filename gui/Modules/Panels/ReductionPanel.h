#pragma once

 

#include "../Panel.h"
#include "PanelNames.h"

class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p) : Panel(p, "BIT"),
                                                   downSample(p, "RATE", "downsampleFreq", ParamUnits::hz),
                                                   bitReduction(p, "BITS", "bitReduction"),
                                                   downsampleMix(p, "MIX", "downsampleMix")
    {
        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);
        addAndMakeVisible(downsampleMix);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        auto width = bounds.getWidth() / 3;
        downSample.setBounds(bounds.removeFromLeft(width));
        bitReduction.setBounds(bounds.removeFromLeft(width));
        downsampleMix.setBounds(bounds);
    }

    ParamKnob downSample;
    ParamKnob downsampleMix;
    ParamKnob bitReduction;
};