#pragma once

 

#include "../Panel.h"



class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "BIT", Palette::colours[1]),
                                                   downSample(p, "RATE", slot, ParamIDs::downsampleFreq, ValueType::hz, ScopeContextType::NOISE),
                                                   bitReduction(p, "BITS", slot, ParamIDs::bitReduction, ValueType::none, ScopeContextType::NOISE)
    {
        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);

        Palette::setKnobColoursOfComponent(&downSample, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&bitReduction, Palette::colours[1]);
        
    }

    void resized() override {
        auto bounds = getLocalBounds();
        downSample.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        bitReduction.setBounds(bounds);
    }

    ParamKnob downSample;
    ParamKnob bitReduction;
};