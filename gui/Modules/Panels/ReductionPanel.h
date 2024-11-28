#pragma once

 

#include "../Panel.h"



class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p) : Panel(p, "BIT", Palette::colours[1]),
                                                   downSample(p, "RATE", ParamIDs::downsampleFreq, ParamUnits::hz),
                                                   bitReduction(p, "BITS", ParamIDs::bitReduction),
                                                   downsampleMix(p, "MIX", ParamIDs::downsampleMix)
    {
        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);
        addAndMakeVisible(downsampleMix);

        Palette::setKnobColoursOfComponent(&downSample, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&bitReduction, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&downsampleMix, Palette::colours[1]);
        
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