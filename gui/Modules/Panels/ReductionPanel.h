#pragma once

 

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"



class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "BIT", Palette::colours[1]),
                                                   downSample(p, "RATE", slot, ParamIDs::downsampleFreq, ScopeContextType::NOISE),
                                                   bitReduction(p, "BITS", slot, ParamIDs::bitReduction, ScopeContextType::NOISE),
                                                   downsampleMix(p, "MIX", slot, ParamIDs::downsampleMix, ScopeContextType::NOISE)
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